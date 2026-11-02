#ifndef FLYTHROUGH_H
#define FLYTHROUGH_H
#pragma once

#include "vector3d.h"
#include <stdint.h>

typedef struct {
    Vec3 position;
    Vec3 look_at;
    float duration;
    float fov;
} FlythroughKeyframe;

typedef struct {
    FlythroughKeyframe* keyframes;
    uint32_t count;
    uint32_t capacity;
} Flythrough;

#ifdef __cplusplus
extern "C" {
#endif

Flythrough* flythrough_create(uint32_t capacity);
void flythrough_destroy(Flythrough* fw);
void flythrough_add_keyframe(Flythrough* fw, Vec3 pos, Vec3 look, float duration, float fov);
void flythrough_evaluate(const Flythrough* fw, float t, Vec3* out_pos, Vec3* out_look);
float flythrough_get_total_duration(const Flythrough* fw);

#ifdef __cplusplus
}
#endif

#endif /* FLYTHROUGH_H */
