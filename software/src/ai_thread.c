#define _POSIX_C_SOURCE 200809L
#include "ai_thread.h"
#include "http_client.h"
#include "json_parser.h"
#include "rwkv.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#define RING_BUFFER_SIZE 8

/* REMOVED: typedef enum { BACKEND_OLLAMA, BACKEND_RWKV } BackendType; */
/* BackendType is now defined in include/ai_thread.h */

typedef struct {
    AIPose poses[RING_BUFFER_SIZE];
    volatile int head, tail, count;
    pthread_mutex_t mutex;
} PoseBuffer;

struct AIThread {
    pthread_t thread;
    volatile int running;
    char* api_url;
    char* api_key;
    char* model;
    char* prompt;
    
    BackendType backend;
    RWKVContext* rwkv_ctx;
    
    HTTPClient* http;
    PoseBuffer buffer;
    pthread_mutex_t prompt_mutex;
};

static void* ai_thread_func(void* arg);

static int pose_buffer_push(PoseBuffer* buf, const AIPose* pose) {
    pthread_mutex_lock(&buf->mutex);
    if (buf->count >= RING_BUFFER_SIZE) {
        buf->tail = (buf->tail + 1) % RING_BUFFER_SIZE;
        buf->count--;
    }
    buf->poses[buf->head] = *pose;
    buf->head = (buf->head + 1) % RING_BUFFER_SIZE;
    buf->count++;
    pthread_mutex_unlock(&buf->mutex);
    return 0;
}

static int pose_buffer_pop(PoseBuffer* buf, AIPose* out) {
    pthread_mutex_lock(&buf->mutex);
    if (buf->count == 0) {
        pthread_mutex_unlock(&buf->mutex);
        return -1;
    }
    *out = buf->poses[buf->tail];
    buf->tail = (buf->tail + 1) % RING_BUFFER_SIZE;
    buf->count--;
    pthread_mutex_unlock(&buf->mutex);
    return 0;
}

AIThread* ai_thread_start(const char* api_url, const char* api_key, const char* model, BackendType backend) {
    if (!model) return NULL;
    
    AIThread* t = (AIThread*)calloc(1, sizeof(AIThread));
    if (!t) return NULL;
    
    t->api_url = api_url ? strdup(api_url) : NULL;
    t->api_key = api_key ? strdup(api_key) : NULL;
    t->model = strdup(model);
    t->prompt = strdup("Output joint positions as JSON: {\"joints\":[{\"x\":0,\"y\":0,\"z\":0},...]}. Only output JSON.");
    t->running = 1;
    t->backend = backend;
    
    pthread_mutex_init(&t->buffer.mutex, NULL);
    pthread_mutex_init(&t->prompt_mutex, NULL);
    
    if (backend == BACKEND_OLLAMA) {
        char host[256] = "localhost";
        int port = 11434;
        if (api_url && strncmp(api_url, "http://", 7) == 0) {
            const char* p = api_url + 7;
            const char* slash = strchr(p, '/');
            if (slash) {
                size_t len = slash - p;
                if (len < sizeof(host)) { strncpy(host, p, len); host[len] = '\0'; }
                char* colon = strchr(host, ':');
                if (colon) { *colon = '\0'; port = atoi(colon + 1); }
            }
        }
        t->http = http_client_create(host, port);
        if (!t->http) {
            free(t->api_url); free(t->api_key); free(t->model); free(t->prompt);
            pthread_mutex_destroy(&t->buffer.mutex);
            pthread_mutex_destroy(&t->prompt_mutex);
            free(t);
            return NULL;
        }
    } else if (backend == BACKEND_RWKV) {
        t->rwkv_ctx = rwkv_init(model);
        if (!t->rwkv_ctx) {
            fprintf(stderr, "WARN: Failed to init RWKV model '%s'\n", model);
        }
    }
    
    if (pthread_create(&t->thread, NULL, ai_thread_func, t) != 0) {
        if (t->http) http_client_destroy(t->http);
        if (t->rwkv_ctx) rwkv_free(t->rwkv_ctx);
        free(t->api_url); free(t->api_key); free(t->model); free(t->prompt);
        pthread_mutex_destroy(&t->buffer.mutex);
        pthread_mutex_destroy(&t->prompt_mutex);
        free(t);
        return NULL;
    }
    
    return t;
}

void ai_thread_stop(AIThread* thread) {
    if (!thread) return;
    thread->running = 0;
    pthread_join(thread->thread, NULL);
    if (thread->http) http_client_destroy(thread->http);
    if (thread->rwkv_ctx) rwkv_free(thread->rwkv_ctx);
    free(thread->api_url);
    free(thread->api_key);
    free(thread->model);
    free(thread->prompt);
    pthread_mutex_destroy(&thread->buffer.mutex);
    pthread_mutex_destroy(&thread->prompt_mutex);
    free(thread);
}

int ai_thread_get_latest_pose(AIThread* thread, AIPose* out_pose) {
    if (!thread || !out_pose) return -1;
    return pose_buffer_pop(&thread->buffer, out_pose);
}

void ai_thread_set_prompt(AIThread* thread, const char* prompt) {
    if (!thread || !prompt) return;
    pthread_mutex_lock(&thread->prompt_mutex);
    free(thread->prompt);
    thread->prompt = strdup(prompt);
    pthread_mutex_unlock(&thread->prompt_mutex);
}

int ai_thread_is_running(AIThread* thread) {
    return thread && thread->running;
}

void ai_thread_switch_backend(AIThread* thread, BackendType backend) {
    if (!thread) return;
    thread->backend = backend;
}

static void* ai_thread_func(void* arg) {
    AIThread* t = (AIThread*)arg;
    char response[65536];
    
    while (t->running) {
        if (t->backend == BACKEND_OLLAMA && !t->http) {
            // Re-init if needed
        } else if (t->backend == BACKEND_RWKV && !t->rwkv_ctx) {
            // Re-init if needed
        }

        pthread_mutex_lock(&t->prompt_mutex);
        char* prompt_copy = strdup(t->prompt);
        pthread_mutex_unlock(&t->prompt_mutex);
        
        AIPose pose = {0};
        
        if (t->backend == BACKEND_OLLAMA) {
            char body[4096];
            snprintf(body, sizeof(body),
                "{\"model\":\"%s\",\"prompt\":\"%s\",\"stream\":false}",
                t->model, prompt_copy);
            
            if (http_client_post(t->http, "/api/generate", body, response, sizeof(response)) == 0) {
                JSONObject* root = json_parse(response);
                if (root) {
                    const char* resp_text = json_get_string(root, "response");
                    if (resp_text && strlen(resp_text) > 0) {
                        JSONObject* joints_obj = json_parse(resp_text);
                        if (joints_obj) {
                            JSONObject* joints_arr = json_get_object(joints_obj, "joints");
                            if (joints_arr && json_array_length(joints_arr) > 0) {
                                int arr_len = json_array_length(joints_arr);
                                pose.joint_count = (arr_len < MAX_JOINTS) ? arr_len : MAX_JOINTS;
                                for (int i = 0; i < pose.joint_count; i++) {
                                    JSONObject* joint = json_array_get(joints_arr, i);
                                    if (joint) {
                                        pose.joint_positions[i].x = (float)json_get_number(joint, "x");
                                        pose.joint_positions[i].y = (float)json_get_number(joint, "y");
                                        pose.joint_positions[i].z = (float)json_get_number(joint, "z");
                                        pose.joint_rotations[i] = quat_identity();
                                    }
                                }
                            }
                            json_free(joints_obj);
                        }
                    }
                    json_free(root);
                }
            }
        } else if (t->backend == BACKEND_RWKV) {
            char rwkv_output[65536] = {0};
            if (t->rwkv_ctx) {
                if (rwkv_eval(t->rwkv_ctx, prompt_copy, rwkv_output, sizeof(rwkv_output)) == 0) {
                    JSONObject* joints_obj = json_parse(rwkv_output);
                    if (joints_obj) {
                        JSONObject* joints_arr = json_get_object(joints_obj, "joints");
                        if (joints_arr && json_array_length(joints_arr) > 0) {
                            int arr_len = json_array_length(joints_arr);
                            pose.joint_count = (arr_len < MAX_JOINTS) ? arr_len : MAX_JOINTS;
                            for (int i = 0; i < pose.joint_count; i++) {
                                JSONObject* joint = json_array_get(joints_arr, i);
                                if (joint) {
                                    pose.joint_positions[i].x = (float)json_get_number(joint, "x");
                                    pose.joint_positions[i].y = (float)json_get_number(joint, "y");
                                    pose.joint_positions[i].z = (float)json_get_number(joint, "z");
                                    pose.joint_rotations[i] = quat_identity();
                                }
                            }
                        }
                        json_free(joints_obj);
                    }
                }
            }
        }

        if (pose.joint_count > 0) {
            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            pose.timestamp = ts.tv_sec + ts.tv_nsec * 1e-9;
            pose_buffer_push(&t->buffer, &pose);
        }
        
        free(prompt_copy);
        
        struct timespec ts_sleep = {0, 100000000};
        nanosleep(&ts_sleep, NULL);
    }
    return NULL;
}
