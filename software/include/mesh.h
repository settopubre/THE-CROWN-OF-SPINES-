#ifndef MESH_H
#define MESH_H

#include <stdint.h>
#include "gpu_handle.h"

typedef struct Mesh {
    uint32_t vertex_count;
    uint32_t index_count;
    float* positions;
    float* normals;
    uint32_t* indices;
    GPUHandle vao, vbo_pos, vbo_nrm, ebo;
    int uploaded;
} Mesh;

#ifdef __cplusplus
extern "C" {
#endif

Mesh* mesh_load_obj(const char* filename);
void mesh_destroy(Mesh* mesh);
void mesh_render(Mesh* mesh);
uint32_t mesh_get_triangle_count(const Mesh* mesh);

#ifdef __cplusplus
}
#endif

#endif /* MESH_H */
