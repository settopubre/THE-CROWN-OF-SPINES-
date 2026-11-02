#ifndef REBAR_CLASH_H
#define REBAR_CLASH_H
#pragma once

#include "rebar_architecture.h"
#include "gpu_handle.h"
#include <stdint.h>

typedef struct {
    uint32_t bar_a, bar_b;
    Vec3 location;
    float distance;
    char severity[16];
} Clash;

typedef struct {
    Clash* clashes;
    uint32_t count, capacity;
    GPUHandle vao;
    GPUHandle vbo;
    int uploaded;
} ClashReport;

#ifdef __cplusplus
extern "C" {
#endif

ClashReport* rebar_detect_clashes(const RebarLayout* rl, float min_spacing, float min_cover);
void clash_report_destroy(ClashReport* cr);
void clash_report_print(const ClashReport* cr);
void clash_report_render(ClashReport* cr, const RebarLayout* rl, Mat4 view, Mat4 proj);

#ifdef __cplusplus
}
#endif

#endif /* REBAR_CLASH_H */
