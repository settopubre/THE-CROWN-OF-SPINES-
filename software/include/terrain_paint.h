#ifndef TERRAIN_PAINT_H
#define TERRAIN_PAINT_H
#pragma once

#include "vector3d.h"
#include "gpu_handle.h"
#include <stdint.h>

typedef struct {
    float* weights;
    uint32_t width, depth;
    GPUHandle texture;
    
    /* GPU resources for painting visualization */
    GPUHandle vao, vbo;
    int uploaded;
} TerrainLayer;

#ifdef __cplusplus
extern "C" {
#endif

TerrainLayer* terrain_paint_create(uint32_t w, uint32_t d);
void terrain_paint_destroy(TerrainLayer* tl);
void terrain_paint_apply(TerrainLayer* tl, const float* heightmap, float slope_threshold, float height_threshold);
void terrain_paint_brush(TerrainLayer* tl, int center_x, int center_z, float radius, float strength, int add_mode);
void terrain_paint_smooth(TerrainLayer* tl, int x, int z, float radius);
void terrain_paint_render(TerrainLayer* tl, Mat4 view, Mat4 proj);
float terrain_paint_get_weight(const TerrainLayer* tl, uint32_t x, uint32_t z);

#ifdef __cplusplus
}
#endif

#endif /* TERRAIN_PAINT_H */
