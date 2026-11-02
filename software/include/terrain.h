#ifndef TERRAIN_H
#define TERRAIN_H

#include "vector3d.h"
#include "mesh.h"
#include "gpu_handle.h"
#include <stdint.h>

typedef struct {
    Mesh* mesh;
    float* heightmap;
    Vec3 origin;
    float scale_xz;
    float scale_y;
    uint32_t width;
    uint32_t depth;
    GPUHandle splat_texture;
    GPUHandle shadow_texture;
} Terrain;

#ifdef __cplusplus
extern "C" {
#endif

Terrain* terrain_create_from_heightmap(const float* data, uint32_t w, uint32_t d, float scale_xz, float scale_y);
void terrain_destroy(Terrain* t);
void terrain_render(Terrain* t, Mat4 view, Mat4 proj);
void terrain_render_with_splatmap(Terrain* t, Mat4 view, Mat4 proj);
void terrain_set_splatmap(Terrain* t, const char* splatmap_path);
void terrain_load_shadowmap(Terrain* t, const char* path);
float terrain_get_height(const Terrain* t, float x, float z);
uint32_t terrain_get_width(const Terrain* t);
uint32_t terrain_get_depth(const Terrain* t);

#ifdef __cplusplus
}
#endif

#endif /* TERRAIN_H */
