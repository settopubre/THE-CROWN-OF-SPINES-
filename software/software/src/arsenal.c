#include "arsenal.h"
#include "gl_functions.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Arsenal* arsenal_create(float radius, int segments) {
    if (radius <= 0 || segments < 3) return NULL;
    
    Arsenal* a = (Arsenal*)calloc(1, sizeof(Arsenal));
    if (!a) return NULL;
    
    a->radius = radius;
    a->position = vec3_create(0.0, 0.0, 0.0);
    
    uint32_t vertex_count = 1 + segments;
    uint32_t index_count = segments * 3;
    
    a->platform = (Mesh*)calloc(1, sizeof(Mesh));
    if (!a->platform) { free(a); return NULL; }
    
    a->platform->vertex_count = vertex_count;
    a->platform->index_count = index_count;
    a->platform->positions = (float*)malloc(vertex_count * 3 * sizeof(float));
    a->platform->normals = (float*)malloc(vertex_count * 3 * sizeof(float));
    a->platform->indices = (uint32_t*)malloc(index_count * sizeof(uint32_t));
    
    if (!a->platform->positions || !a->platform->normals || !a->platform->indices) {
        free(a->platform->positions); free(a->platform->normals); free(a->platform->indices);
        free(a->platform); free(a);
        return NULL;
    }
    
    a->platform->positions[0] = 0; a->platform->positions[1] = 0; a->platform->positions[2] = 0;
    a->platform->normals[0] = 0; a->platform->normals[1] = 1; a->platform->normals[2] = 0;
    
    for (int i = 0; i < segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        uint32_t idx = i + 1;
        a->platform->positions[idx*3] = radius * cosf(angle);
        a->platform->positions[idx*3+1] = 0;
        a->platform->positions[idx*3+2] = radius * sinf(angle);
        a->platform->normals[idx*3] = 0;
        a->platform->normals[idx*3+1] = 1;
        a->platform->normals[idx*3+2] = 0;
    }
    
    for (int i = 0; i < segments; i++) {
        a->platform->indices[i*3] = 0;
        a->platform->indices[i*3+1] = (i + 1) % segments + 1;
        a->platform->indices[i*3+2] = i + 1;
    }
    
    return a;
}

void arsenal_destroy(Arsenal* a) {
    if (!a) return;
    if (a->platform) { mesh_destroy(a->platform); }
    free(a);
}

void arsenal_render(Arsenal* a, Mat4 view, Mat4 projection) {
    (void)view; (void)projection;
    if (!a || !a->platform) return;
    mesh_render(a->platform);
}

Vec3 arsenal_get_position(const Arsenal* a) {
    return a ? a->position : vec3_create(0.0, 0.0, 0.0);
}
