#include <stdio.h>
#include "erosion.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h> /* ADDED for SIZE_MAX */

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct Erosion {
    int width, depth;
    float scale;
    float* current_heights;
    float* next_heights;
    float* water_layer;
    float* sediment_layer;
};

static int _idx(int x, int z, int w) { return z * w + x; }

Erosion* erosion_create(int width, int depth, float scale) {
    if (width <= 0 || depth <= 0) return NULL;
    Erosion* e = (Erosion*)calloc(1, sizeof(Erosion));
    if (!e) return NULL;
    e->width = width;
    e->depth = depth;
    e->scale = scale;
    e->current_heights = (float*)calloc(width * depth, sizeof(float));
    e->next_heights = (float*)calloc(width * depth, sizeof(float));
    e->water_layer = (float*)calloc(width * depth, sizeof(float));
    e->sediment_layer = (float*)calloc(width * depth, sizeof(float));
    if (!e->current_heights || !e->next_heights || !e->water_layer || !e->sediment_layer) {
        free(e->current_heights); free(e->next_heights);
        free(e->water_layer); free(e->sediment_layer); free(e);
        return NULL;
    }
    return e;
}

void erosion_destroy(Erosion* e) {
    if (e) {
        free(e->current_heights);
        free(e->next_heights);
        free(e->water_layer);
        free(e->sediment_layer);
        free(e);
    }
}

void erosion_thermal(Erosion* e, float talus_angle, float amount) {
    if (!e || amount <= 0.0f) return;
    memcpy(e->next_heights, e->current_heights, e->width * e->depth * sizeof(float));
    float tan_talus = tanf(talus_angle * (M_PI / 180.0f));
    int dx[4] = {1, -1, 0, 0};
    int dz[4] = {0, 0, 1, -1};
    
    for (int z = 1; z < e->depth - 1; z++) {
        for (int x = 1; x < e->width - 1; x++) {
            float h_center = e->current_heights[_idx(x, z, e->width)];
            for (int i = 0; i < 4; i++) {
                int nx = x + dx[i];
                int nz = z + dz[i];
                float h_neighbor = e->current_heights[_idx(nx, nz, e->width)];
                float diff = h_center - h_neighbor;
                if (diff > 0.0f) {
                    float slope = diff / e->scale;
                    if (slope > tan_talus) {
                        float transfer = diff * amount;
                        e->next_heights[_idx(x, z, e->width)] -= transfer;
                        e->next_heights[_idx(nx, nz, e->width)] += transfer;
                    }
                }
            }
        }
    }
    memcpy(e->current_heights, e->next_heights, e->width * e->depth * sizeof(float));
}

void erosion_hydraulic(Erosion* e, float rain, float evaporation, float sediment) {
    if (!e) return;
    
    int w = e->width, d = e->depth;
    
    /* CRITICAL FIX: Guard against integer overflow in grid allocation */
    if (w <= 0 || d <= 0 || (size_t)w > SIZE_MAX / (size_t)d) {
        fprintf(stderr, "[EROSION] erosion_hydraulic: invalid grid dims w=%d d=%d\n", w, d);
        return;
    }
    
    /* 1. Add Rain */
    for (int i = 0; i < w * d; i++) {
        e->water_layer[i] += rain;
        if (e->water_layer[i] < 0.0f) e->water_layer[i] = 0.0f;
    }
    
    /* 2. Flow Accumulation & Sediment Transport */
    float* new_water = (float*)calloc(w * d, sizeof(float));
    float* new_sediment = (float*)calloc(w * d, sizeof(float));
    int dx[4] = {1, -1, 0, 0};
    int dz[4] = {0, 0, 1, -1};
    
    for (int z = 1; z < d - 1; z++) {
        for (int x = 1; x < w - 1; x++) {
            if (e->water_layer[_idx(x, z, w)] < 0.001f) continue;
            
            float total_weight = 0.0f;
            float weights[4] = {0.0f};
            float h = e->current_heights[_idx(x, z, w)];
            for (int i = 0; i < 4; i++) {
                int nx = x + dx[i], nz = z + dz[i];
                float diff = h - e->current_heights[_idx(nx, nz, w)];
                if (diff > 0.0f) {
                    weights[i] = diff;
                    total_weight += diff;
                }
            }
            
            if (total_weight > 0.0f) {
                for (int i = 0; i < 4; i++) {
                    if (weights[i] > 0.0f) {
                        int nx = x + dx[i], nz = z + dz[i];
                        float flow_ratio = weights[i] / total_weight;
                        float water_flow = e->water_layer[_idx(x, z, w)] * flow_ratio * 0.5f;
                        float sed_flow = e->sediment_layer[_idx(x, z, w)] * flow_ratio * 0.5f;
                        
                        new_water[_idx(nx, nz, w)] += water_flow;
                        new_sediment[_idx(nx, nz, w)] += sed_flow;
                        
                        e->water_layer[_idx(x, z, w)] -= water_flow;
                        e->sediment_layer[_idx(x, z, w)] -= sed_flow;
                    }
                }
            }
            
            float capacity = e->water_layer[_idx(x, z, w)] * sediment;
            float excess = e->sediment_layer[_idx(x, z, w)] - capacity;
            if (excess > 0.0f) {
                e->current_heights[_idx(x, z, w)] += excess * 0.1f;
                e->sediment_layer[_idx(x, z, w)] -= excess;
            } else {
                float erode_amount = fabsf(excess) * 0.01f;
                if (e->current_heights[_idx(x, z, w)] > erode_amount) {
                    e->current_heights[_idx(x, z, w)] -= erode_amount;
                    e->sediment_layer[_idx(x, z, w)] += erode_amount * 0.5f;
                }
            }
        }
    }
    
    for (int i = 0; i < w * d; i++) {
        e->water_layer[i] += new_water[i];
        e->sediment_layer[i] += new_sediment[i];
        e->water_layer[i] *= (1.0f - evaporation);
        if (e->water_layer[i] < 0.0f) e->water_layer[i] = 0.0f;
        if (e->sediment_layer[i] < 0.0f) e->sediment_layer[i] = 0.0f;
    }
    
    free(new_water);
    free(new_sediment);
}

void erosion_apply_to_heightmap(Erosion* e, float* heightmap, int width, int depth) {
    if (!e || !heightmap || width <= 0 || depth <= 0) return;
    if (width != e->width || depth != e->depth) {
        free(e->current_heights); free(e->next_heights);
        free(e->water_layer); free(e->sediment_layer);
        e->width = width; e->depth = depth;
        e->current_heights = (float*)calloc(width * depth, sizeof(float));
        e->next_heights = (float*)calloc(width * depth, sizeof(float));
        e->water_layer = (float*)calloc(width * depth, sizeof(float));
        e->sediment_layer = (float*)calloc(width * depth, sizeof(float));
    }
    memcpy(e->current_heights, heightmap, width * depth * sizeof(float));
    erosion_thermal(e, 35.0f, 0.1f);
    erosion_hydraulic(e, 0.05f, 0.02f, 0.8f);
    memcpy(heightmap, e->current_heights, width * depth * sizeof(float));
}
