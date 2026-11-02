#define _POSIX_C_SOURCE 200809L
#include "bridge_server.h"
#include "engine_abi.h"
#include "json_parser.h"
#include "editor.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define SOCK_PATH "/tmp/3dai_oracle.sock"

extern struct RWorker* g_stat_worker;
extern Editor* g_editor; /* Explicit declaration for global defined in engine.c */

static void handle_client_logic(int fd) {
    char buf[4096];
    FILE* fp = fdopen(fd, "r+");
    if (!fp) {
        close(fd);
        return;
    }

    while (fgets(buf, sizeof(buf), fp)) {
        char* nl = strchr(buf, '\n');
        if (nl) *nl = '\0';

        JSONObject* req = json_parse(buf);
        if (!req) {
            fputs("{\"status\":\"error\",\"message\":\"invalid json\"}\n", fp);
            fflush(fp);
            continue;
        }

        const char* action = json_get_string(req, "action");
        JSONObject* payload = json_get_object(req, "payload");
        char resp[2048];

        if (action && strcmp(action, "execute") == 0) {
            const char* cmd = json_get_string(payload, "command");
            if (strcmp(cmd, "stats") == 0) {
                const char* test = json_get_string(payload, "test");
                const char* data = json_get_string(payload, "data");
                if (g_stat_worker) {
                    r_stat_send_test(g_stat_worker, test, data);
                    char* res = r_stat_recv_result(g_stat_worker);
                    snprintf(resp, sizeof(resp), "{\"status\":\"ok\",\"data\":%s}", res ? res : "null");
                } else {
                    snprintf(resp, sizeof(resp), "{\"status\":\"error\",\"message\":\"stat worker offline\"}");
                }
            } else if (strcmp(cmd, "scene") == 0) {
                snprintf(resp, sizeof(resp), "{\"status\":\"ok\",\"data\":{\"objects\": %u}}", g_editor ? g_editor->object_count : 0);
            } else {
                snprintf(resp, sizeof(resp), "{\"status\":\"ok\",\"data\":\"command_queued\"}");
            }
        } else {
            snprintf(resp, sizeof(resp), "{\"status\":\"error\",\"message\":\"unknown action\"}");
        }

        fputs(resp, fp);
        fputc('\n', fp);
        fflush(fp);
        json_free(req);
    }
    fclose(fp); /* safely closes underlying fd */
}

static void* handle_client_thread(void* arg) {
    int fd = (int)(intptr_t)arg;
    handle_client_logic(fd);
    return NULL;
}

static void* bridge_listener(void* arg) {
    (void)arg;
    int server_fd, client_fd;
    struct sockaddr_un addr;

    unlink(SOCK_PATH);
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) return NULL;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(server_fd);
        return NULL;
    }

    if (listen(server_fd, 5) < 0) {
        close(server_fd);
        return NULL;
    }

    printf("🔌 Bridge server listening on %s\n", SOCK_PATH);

    while (1) {
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd >= 0) {
            pthread_t tid;
            pthread_create(&tid, NULL, handle_client_thread, (void*)(intptr_t)client_fd);
            pthread_detach(tid);
        }
    }
    return NULL;
}

void bridge_server_start(void) {
    pthread_t tid;
    pthread_create(&tid, NULL, bridge_listener, NULL);
    pthread_detach(tid);
}
