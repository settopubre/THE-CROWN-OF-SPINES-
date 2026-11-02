#ifndef GRAVEL_H
#define GRAVEL_H
#pragma once

#include <GL/gl.h>
#include "vector3d.h"
#include <stdint.h>

typedef struct {
    Vec3* perimeter;
    uint32_t perimeter_count;
    float thickness;
    float area_sqft;
    float volume_yards;
    GLuint vao, vbo;
    int uploaded;
} GravelLayer;

#ifdef __cplusplus
extern "C" {
#endif

GravelLayer* gravel_generate(const Vec3* perimeter, uint32_t count, float thickness);
void gravel_destroy(GravelLayer* gl);
void gravel_render(GravelLayer* gl, Mat4 view, Mat4 proj);
void gravel_print_report(const GravelLayer* gl);

#ifdef __cplusplus
}
#endif

#endif /* GRAVEL_H */
