#ifndef FORMWORK_H
#define FORMWORK_H
#pragma once

#include "vector3d.h"
#include <stdint.h>
#include <GL/gl.h>

typedef struct {
    float width, thickness, cost_per_ft;
    float length;  /* unused but kept for struct compatibility */
} Lumber;

typedef struct {
    Vec3 position, size;
    Quat rotation;
    int is_stake;
} FormPanel;

typedef struct {
    FormPanel* panels;
    uint32_t count, capacity;
    float total_bf, total_cost;
    
    /* GPU resources for rendering */
    GLuint vao, vbo;
    uint32_t vertex_count;
    int uploaded;
} Formwork;

#ifdef __cplusplus
extern "C" {
#endif

Formwork* formwork_generate(const Vec3* perimeter, uint32_t count, float slab_thickness);
void formwork_destroy(Formwork* fw);
void formwork_render(Formwork* fw, Mat4 view, Mat4 proj);
void formwork_print_report(const Formwork* fw);

#ifdef __cplusplus
}
#endif

#endif /* FORMWORK_H */
