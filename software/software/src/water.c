#include "water.h"
#include "gl_functions.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef enum {
    WAVE_TYPE_SINE,
    WAVE_TYPE_GERSTNER,
    WAVE_TYPE_CALM
} WaveType;

struct Water {
    float width, depth;
    int grid_x, grid_z;
    float resolution;
    
    float* heights;
    uint32_t vao, vbo, ebo;
    uint32_t index_count;
    
    GLuint reflection_tex;
    GLuint refraction_tex;

    WaveType wave_type;
    
    /*
     * P0 CRITICAL FIX: Persistent vertex buffer for per-frame updates.
     *
     * WAS: float* updated_verts allocated via malloc every frame
     *      in water_render(), then freed at the end.
     *
     * BUG: Per-frame malloc/free in the render hot path causes:
     *      1. Heap fragmentation over time
     *      2. Unnecessary CPU-GPU synchronization
     *      3. Potential frame stalls if allocator contends
     *
     * FIX: Pre-allocate a vertex buffer at creation time. Reuse it
     *      every frame via glBufferSubData. The buffer is large enough
     *      for the full grid at 6 floats per vertex (pos + normal).
     *      Freed once in water_destroy().
     */
    float* vertex_buffer;
    uint32_t vertex_buffer_size;
};

Water* water_create(float width, float depth, float resolution) {
    Water* w = (Water*)calloc(1, sizeof(Water));
    if (!w) return NULL;
    
    w->width = width; w->depth = depth;
    w->resolution = resolution > 0.5f ? resolution : 1.0f;
    
    w->grid_x = (int)(width / w->resolution);
    w->grid_z = (int)(depth / w->resolution);
    
    w->heights = (float*)calloc(w->grid_x * w->grid_z, sizeof(float));
    
    uint32_t vert_count = w->grid_x * w->grid_z;
    uint32_t idx_count = (w->grid_x - 1) * (w->grid_z - 1) * 6;
    w->index_count = idx_count;
    
    /*
     * P0 CRITICAL FIX: Pre-allocate vertex buffer once.
     */
    w->vertex_buffer_size = vert_count * 6;
    w->vertex_buffer = (float*)malloc(w->vertex_buffer_size * sizeof(float));
    
    float* vertices = (float*)malloc(vert_count * 6 * sizeof(float));
    uint32_t* indices = (uint32_t*)malloc(idx_count * sizeof(uint32_t));
    
    for (int z = 0; z < w->grid_z; z++) {
        for (int x = 0; x < w->grid_x; x++) {
            int idx = z * w->grid_x + x;
            vertices[idx * 6]     = -width * 0.5f + x * w->resolution;
            vertices[idx * 6 + 1] = 0.0f;
            vertices[idx * 6 + 2] = -depth * 0.5f + z * w->resolution;
            vertices[idx * 6 + 3] = 0.0f;
            vertices[idx * 6 + 4] = 1.0f;
            vertices[idx * 6 + 5] = 0.0f;
        }
    }
    
    uint32_t ic = 0;
    for (int z = 0; z < w->grid_z - 1; z++) {
        for (int x = 0; x < w->grid_x - 1; x++) {
            uint32_t i0 = z * w->grid_x + x;
            uint32_t i1 = i0 + 1;
            uint32_t i2 = (z + 1) * w->grid_x + x;
            uint32_t i3 = i2 + 1;
            indices[ic++] = i0; indices[ic++] = i1; indices[ic++] = i2;
            indices[ic++] = i1; indices[ic++] = i3; indices[ic++] = i2;
        }
    }
    
    glGenVertexArrays(1, &w->vao);
    glGenBuffers(1, &w->vbo);
    glGenBuffers(1, &w->ebo);
    
    glBindVertexArray(w->vao);
    glBindBuffer(GL_ARRAY_BUFFER, w->vbo);
    glBufferData(GL_ARRAY_BUFFER, vert_count * 6 * sizeof(float), vertices, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, w->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx_count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
    
    free(vertices);
    free(indices);
    
    w->reflection_tex = 0;
    w->refraction_tex = 0;
    w->wave_type = WAVE_TYPE_SINE;
    return w;
}

void water_set_wave_type(Water* w, int type) {
    if (w) w->wave_type = (WaveType)type;
}

void water_set_reflection_texture(Water* w, GLuint texture) {
    if (w) w->reflection_tex = texture;
}

void water_set_refraction_texture(Water* w, GLuint texture) {
    if (w) w->refraction_tex = texture;
}

void water_update(Water* w, float delta_time, float wind_dir, float wind_strength) {
    if (!w) return;
    
    int count = w->grid_x * w->grid_z;
    
    if (w->wave_type == WAVE_TYPE_CALM) {
        memset(w->heights, 0, count * sizeof(float));
        return;
    }
    
    if (w->wave_type == WAVE_TYPE_GERSTNER) {
        float k = 2.0f * M_PI / 12.0f;
        float w_freq = sqrtf(9.8f * k);
        float A = 0.4f * wind_strength;
        
        float dx = cosf(wind_dir);
        float dz = sinf(wind_dir);
        
        for (int z = 0; z < w->grid_z; z++) {
            for (int x = 0; x < w->grid_x; x++) {
                int idx = z * w->grid_x + x;
                float px = -w->width * 0.5f + x * w->resolution;
                float pz = -w->depth * 0.5f + z * w->resolution;
                
                float phi = k * (px * dx + pz * dz) - w_freq * delta_time;
                w->heights[idx] = A * sinf(phi);
            }
        }
    } else {
        float k = 2.0f * M_PI / 10.0f;
        float w_freq = sqrtf(9.8f * k);
        float steepness = 0.5f * wind_strength;
        
        for (int z = 0; z < w->grid_z; z++) {
            for (int x = 0; x < w->grid_x; x++) {
                int idx = z * w->grid_x + x;
                float px = -w->width * 0.5f + x * w->resolution;
                float pz = -w->depth * 0.5f + z * w->resolution;
                
                float phase = k * (px * cosf(wind_dir) + pz * sinf(wind_dir)) - w_freq * delta_time * 2.0f;
                w->heights[idx] = steepness * sinf(phase);
            }
        }
    }
}

void water_render(Water* w, Mat4 view, Mat4 proj, Vec3 camera_pos) {
    if (!w) return;
    (void)camera_pos;
    
    /*
     * P0 CRITICAL FIX: Use pre-allocated vertex buffer.
     *
     * WAS: float* updated_verts = (float*)malloc(...);
     *      ...fill...
     *      glBufferSubData(...);
     *      free(updated_verts);
     *
     * NOW: Fill pre-allocated w->vertex_buffer, then upload.
     *      Zero per-frame allocations.
     */
    if (!w->vertex_buffer) return;
    
    uint32_t verts = w->grid_x * w->grid_z;
    for (int z = 0; z < w->grid_z; z++) {
        for (int x = 0; x < w->grid_x; x++) {
            uint32_t idx = z * w->grid_x + x;
            w->vertex_buffer[idx * 6]     = -w->width * 0.5f + x * w->resolution;
            w->vertex_buffer[idx * 6 + 1] = w->heights[idx];
            w->vertex_buffer[idx * 6 + 2] = -w->depth * 0.5f + z * w->resolution;
            w->vertex_buffer[idx * 6 + 3] = 0.0f;
            w->vertex_buffer[idx * 6 + 4] = 1.0f;
            w->vertex_buffer[idx * 6 + 5] = 0.0f;
        }
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, w->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, verts * 6 * sizeof(float), w->vertex_buffer);
    
    if (w->reflection_tex || w->refraction_tex) {
        glActiveTexture(GL_TEXTURE0);
        if (w->reflection_tex) {
            glBindTexture(GL_TEXTURE_2D, w->reflection_tex);
            GLint loc = glGetUniformLocation(shader_program, "uReflectionTex");
            if (loc >= 0) glUniform1i(loc, 0);
        }
        glActiveTexture(GL_TEXTURE1);
        if (w->refraction_tex) {
            glBindTexture(GL_TEXTURE_2D, w->refraction_tex);
            GLint loc = glGetUniformLocation(shader_program, "uRefractionTex");
            if (loc >= 0) glUniform1i(loc, 1);
        }
    }
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    
    glBindVertexArray(w->vao);
    glDrawElements(GL_TRIANGLES, w->index_count, GL_UNSIGNED_INT, (void*)0);
    glBindVertexArray(0);
    
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
}

void water_destroy(Water* w) {
    if (w) {
        if (w->vao) glDeleteVertexArrays(1, &w->vao);
        if (w->vbo) glDeleteBuffers(1, &w->vbo);
        if (w->ebo) glDeleteBuffers(1, &w->ebo);
        free(w->heights);
        free(w->vertex_buffer);
        free(w);
    }
}
