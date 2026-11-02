#ifndef AI_THREAD_H
#define AI_THREAD_H
#pragma once

#include <stdint.h>
#include "vector3d.h"

#define MAX_JOINTS 64

/* Backend selection for AI thread */
typedef enum { BACKEND_OLLAMA, BACKEND_RWKV } BackendType;

typedef struct {
    Vec3 joint_positions[MAX_JOINTS];
    Quat joint_rotations[MAX_JOINTS];
    uint32_t joint_count;
    double timestamp;
} AIPose;

typedef struct AIThread AIThread;

#ifdef __cplusplus
extern "C" {
#endif

AIThread* ai_thread_start(const char* api_url, const char* api_key, const char* model, BackendType backend);
void ai_thread_stop(AIThread* thread);
int ai_thread_get_latest_pose(AIThread* thread, AIPose* out_pose);
void ai_thread_set_prompt(AIThread* thread, const char* prompt);
int ai_thread_is_running(AIThread* thread);

#ifdef __cplusplus
}
#endif

#endif /* AI_THREAD_H */
