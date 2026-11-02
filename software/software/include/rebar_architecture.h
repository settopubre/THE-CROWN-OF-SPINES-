#ifndef REBAR_ARCHITECTURE_H
#define REBAR_ARCHITECTURE_H
#pragma once

#include "gpu_handle.h"
#include "vector3d.h"
#include <stdint.h>

typedef enum { REBAR_3 = 0, REBAR_4 = 1, REBAR_5 = 2 } RebarType;

typedef struct {
    Vec3 start, end;
    float diameter;
    char grade[8];
} Rebar;

typedef struct {
    Rebar* bars;
    uint32_t count, capacity;
    float spacing, cover, chair_spacing;
    GPUHandle vao, vbo;
    int uploaded;
} RebarLayout;

#ifdef __cplusplus
extern "C" {
#endif

RebarLayout* rebar_layout_generate(const Vec3* perimeter, uint32_t count, float slab_thickness, float cover_inches, float spacing_inches);
void rebar_layout_destroy(RebarLayout* rl);
void rebar_layout_render(RebarLayout* rl, Mat4 view, Mat4 proj);
void rebar_layout_print_report(const RebarLayout* rl);

#ifdef __cplusplus
}
#endif

#endif /* REBAR_ARCHITECTURE_H */
