#ifndef WATER_H
#define WATER_H

#include "vector3d.h"
#include "gpu_handle.h"
#include <stdint.h>

typedef struct Water Water;

#ifdef __cplusplus
extern "C" {
#endif

Water* water_create(float width, float depth, float resolution);
void water_set_wave_type(Water* w, int type); /* 0=SINE, 1=GERSTNER, 2=CALM */
void water_set_reflection_texture(Water* w, GPUHandle texture);
void water_set_refraction_texture(Water* w, GPUHandle texture);
void water_update(Water* w, float delta_time, float wind_dir, float wind_strength);
void water_render(Water* w, Mat4 view, Mat4 proj, Vec3 camera_pos);
void water_destroy(Water* w);

#ifdef __cplusplus
}
#endif

#endif /* WATER_H */

