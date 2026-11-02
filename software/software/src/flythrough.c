#include "flythrough.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

Flythrough* flythrough_create(uint32_t capacity) {
    if (capacity == 0) capacity = 16;
    Flythrough* fw = (Flythrough*)calloc(1, sizeof(Flythrough));
    if (!fw) return NULL;
    fw->keyframes = (FlythroughKeyframe*)calloc(capacity, sizeof(FlythroughKeyframe));
    if (!fw->keyframes) { free(fw); return NULL; }
    fw->capacity = capacity;
    fw->count = 0;
    return fw;
}

void flythrough_destroy(Flythrough* fw) {
    if (fw) { free(fw->keyframes); free(fw); }
}

void flythrough_add_keyframe(Flythrough* fw, Vec3 pos, Vec3 look, float duration, float fov) {
    if (!fw || fw->count >= fw->capacity) return;
    fw->keyframes[fw->count].position = pos;
    fw->keyframes[fw->count].look_at = look;
    fw->keyframes[fw->count].duration = duration;
    fw->keyframes[fw->count].fov = fov;
    fw->count++;
}

static Vec3 _catmull_rom(Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3, float t) {
    float t2 = t * t, t3 = t2 * t;
    float x = 0.5f * ((2.0f*p1.x) + (-p0.x+p2.x)*t + (2.0f*p0.x-5.0f*p1.x+4.0f*p2.x-p3.x)*t2 + (-p0.x+3.0f*p1.x-3.0f*p2.x+p3.x)*t3);
    float y = 0.5f * ((2.0f*p1.y) + (-p0.y+p2.y)*t + (2.0f*p0.y-5.0f*p1.y+4.0f*p2.y-p3.y)*t2 + (-p0.y+3.0f*p1.y-3.0f*p2.y+p3.y)*t3);
    float z = 0.5f * ((2.0f*p1.z) + (-p0.z+p2.z)*t + (2.0f*p0.z-5.0f*p1.z+4.0f*p2.z-p3.z)*t2 + (-p0.z+3.0f*p1.z-3.0f*p2.z+p3.z)*t3);
    return vec3_create(x, y, z);
}

static Vec3 _lerp_v3(Vec3 a, Vec3 b, float t) {
    return vec3_create(a.x + (b.x-a.x)*t, a.y + (b.y-a.y)*t, a.z + (b.z-a.z)*t);
}

void flythrough_evaluate(const Flythrough* fw, float t, Vec3* out_pos, Vec3* out_look) {
    if (!fw || fw->count == 0) return;
    float total = flythrough_get_total_duration(fw);
    if (total <= 0.0f) return;
    
    float current = 0.0f;
    for (uint32_t i = 0; i < fw->count - 1; i++) {
        float seg_dur = fw->keyframes[i].duration;
        if (t >= current && t <= current + seg_dur) {
            float local_t = (t - current) / seg_dur;
            
            int p0 = (i > 0) ? i - 1 : 0;
            int p1 = i;
            int p2 = i + 1;
            int p3 = (i + 2 < fw->count) ? i + 2 : fw->count - 1;
            
            if (out_pos) *out_pos = _catmull_rom(fw->keyframes[p0].position, fw->keyframes[p1].position, 
                                                  fw->keyframes[p2].position, fw->keyframes[p3].position, local_t);
            if (out_look) *out_look = _lerp_v3(fw->keyframes[p1].look_at, fw->keyframes[p2].look_at, local_t);
            return;
        }
        current += seg_dur;
    }
    
    /* Clamp to last */
    if (out_pos) *out_pos = fw->keyframes[fw->count-1].position;
    if (out_look) *out_look = fw->keyframes[fw->count-1].look_at;
}

float flythrough_get_total_duration(const Flythrough* fw) {
    if (!fw) return 0.0f;
    float total = 0.0f;
    for (uint32_t i = 0; i < fw->count; i++) total += fw->keyframes[i].duration;
    return total;
}
