#include "extrude.h"
#include "gl_functions.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

Mesh* extrude_mesh(Mesh* face, float depth, int direction) {
    if (!face || face->vertex_count == 0) return NULL;
    
    Mesh* result = (Mesh*)calloc(1, sizeof(Mesh));
    if (!result) return NULL;
    
    uint32_t front_verts = face->vertex_count;
    uint32_t total_verts = front_verts * 2;
    
    result->vertex_count = total_verts;
    result->positions = (float*)malloc(total_verts * 3 * sizeof(float));
    if (!result->positions) { free(result); return NULL; }
    
    /* Copy front face */
    memcpy(result->positions, face->positions, front_verts * 3 * sizeof(float));
    
    /* Copy back face offset by depth along normal (assume Y-up for 2D shapes) */
    for (uint32_t i = 0; i < front_verts; i++) {
        result->positions[(front_verts+i)*3] = face->positions[i*3];
        result->positions[(front_verts+i)*3+1] = face->positions[i*3+1] + depth * direction;
        result->positions[(front_verts+i)*3+2] = face->positions[i*3+2];
    }
    
    /* Generate side walls - simplified: connect corresponding edges */
    result->index_count = face->index_count * 2 + front_verts * 6;
    result->indices = (uint32_t*)malloc(result->index_count * sizeof(uint32_t));
    if (!result->indices) { free(result->positions); free(result); return NULL; }
    
    uint32_t ic = 0;
    /* Front face indices (offset by 0) */
    for (uint32_t i = 0; i < face->index_count; i++) {
        result->indices[ic++] = face->indices[i];
    }
    /* Back face indices (offset by front_verts) */
    for (uint32_t i = 0; i < face->index_count; i++) {
        result->indices[ic++] = face->indices[i] + front_verts;
    }
    /* Side walls - connect each edge */
    for (uint32_t i = 0; i < face->index_count; i += 3) {
        for (int e = 0; e < 3; e++) {
            uint32_t v0 = face->indices[i+e];
            uint32_t v1 = face->indices[i+(e+1)%3];
            /* Quad: v0, v1, v1+front, v0+front */
            result->indices[ic++] = v0;
            result->indices[ic++] = v1;
            result->indices[ic++] = v1 + front_verts;
            result->indices[ic++] = v0;
            result->indices[ic++] = v1 + front_verts;
            result->indices[ic++] = v0 + front_verts;
        }
    }
    
    return result;
}

Extrude* extrude_create(Mesh* face, float depth, int direction) {
    Extrude* e = (Extrude*)calloc(1, sizeof(Extrude));
    if (!e) return NULL;
    e->original = face;
    e->depth = depth;
    e->direction = direction;
    e->extruded = extrude_mesh(face, depth, direction);
    return e;
}

void extrude_destroy(Extrude* e) {
    if (!e) return;
    /* Note: original mesh not owned by Extrude */
    if (e->extruded) mesh_destroy(e->extruded);
    free(e);
}

void extrude_render(Extrude* e) {
    if (!e || !e->extruded) return;
    mesh_render(e->extruded);
}
