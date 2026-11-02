#ifndef SPLAT_H
#define SPLAT_H

#include "vector3d.h"
#include "gpu_handle.h"
#include <stdint.h>

typedef struct {
    float min_x, min_y, min_z;
    float max_x, max_y, max_z;
} SplatBBox;

typedef struct {
    float* positions;
    float* colors;
    uint32_t count;
    SplatBBox bbox;
    int has_covariance;
    int uploaded;
    GPUHandle vao, vbo;
} Splat;

#ifdef __cplusplus
extern "C" {
#endif

Splat* splat_load(const char* filename);
void splat_destroy(Splat* s);
void splat_render(Splat* s, Mat4 view, Mat4 proj, float point_size);
uint32_t splat_get_count(const Splat* s);

#ifdef __cplusplus
}
#endif

#endif /* SPLAT_H */
