#ifndef SPLAT_DIFF_H
#define SPLAT_DIFF_H
#pragma once

#include "splat.h"

typedef struct {
    Splat* before;
    Splat* after;
    int* diff_values;
    uint32_t count;
    GPUHandle vao, vbo;
    int uploaded;
} SplatDiff;

#ifdef __cplusplus
extern "C" {
#endif

SplatDiff* splat_diff_create(void);
void splat_diff_destroy(SplatDiff* sd);
void splat_diff_set_before(SplatDiff* sd, Splat* s);
void splat_diff_set_after(SplatDiff* sd, Splat* s);
void splat_diff_compute(SplatDiff* sd, float tolerance);
void splat_diff_render(SplatDiff* sd, Mat4 view, Mat4 proj);

#ifdef __cplusplus
}
#endif

#endif /* SPLAT_DIFF_H */
