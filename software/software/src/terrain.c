#include "terrain.h"
#include "gl_functions.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#if __has_include("stb_image.h")
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#else
#define HAS_STB 0
#endif

static Vec3 _compute_normal(const float* hmap, uint32_t w, uint32_t d, uint32_t x, uint32_t z, float scale_xz, float scale_y) {
    float hl = (x > 0) ? hmap[(z * w) + (x - 1)] : hmap[z * w + x];
    float hr = (x < w - 1) ? hmap[(z * w) + (x + 1)] : hmap[z * w + x];
    float hu = (z > 0) ? hmap[((z - 1) * w) + x] : hmap[z * w + x];
    float hd = (z < d - 1) ? hmap[((z + 1) * w) + x] : hmap[z * w + x];
    float dx = (hr - hl) * scale_xz;
    float dz = (hd - hu) * scale_xz;
    Vec3 n = vec3_normalize(vec3_create(-dx, 2.0f * scale_y, -dz));
    return n;
}

Terrain* terrain_create_from_heightmap(const float* data, uint32_t w, uint32_t d, float scale_xz, float scale_y) {
    if (!data || w < 2 || d < 2) return NULL;
    
    Terrain* t = (Terrain*)calloc(1, sizeof(Terrain));
    if (!t) return NULL;
    t->width = w; t->depth = d;
    t->scale_xz = scale_xz; t->scale_y = scale_y;
    t->heightmap = (float*)malloc(w * d * sizeof(float));
    if (!t->heightmap) { free(t); return NULL; }
    memcpy(t->heightmap, data, w * d * sizeof(float));
    
    t->mesh = (Mesh*)calloc(1, sizeof(Mesh));
    if (!t->mesh) { free(t->heightmap); free(t); return NULL; }
    
    t->mesh->vertex_count = w * d;
    t->mesh->positions = (float*)malloc(t->mesh->vertex_count * 5 * sizeof(float));
    t->mesh->normals = (float*)malloc(t->mesh->vertex_count * 3 * sizeof(float));
    
    for (uint32_t z = 0; z < d; z++) {
        for (uint32_t x = 0; x < w; x++) {
            uint32_t idx = z * w + x;
            t->mesh->positions[idx * 5]     = t->origin.x + x * scale_xz;
            t->mesh->positions[idx * 5 + 1] = t->origin.y + data[idx] * scale_y;
            t->mesh->positions[idx * 5 + 2] = t->origin.z + z * scale_xz;
            t->mesh->positions[idx * 5 + 3] = (x * scale_xz) * 0.1f;
            t->mesh->positions[idx * 5 + 4] = (z * scale_xz) * 0.1f;
            
            Vec3 n = _compute_normal(data, w, d, x, z, scale_xz, scale_y);
            t->mesh->normals[idx * 3]     = n.x;
            t->mesh->normals[idx * 3 + 1] = n.y;
            t->mesh->normals[idx * 3 + 2] = n.z;
        }
    }
    
    t->mesh->index_count = (w - 1) * (d - 1) * 6;
    t->mesh->indices = (uint32_t*)malloc(t->mesh->index_count * sizeof(uint32_t));
    uint32_t ic = 0;
    for (uint32_t z = 0; z < d - 1; z++) {
        for (uint32_t x = 0; x < w - 1; x++) {
            uint32_t i0 = z * w + x;
            uint32_t i1 = i0 + 1;
            uint32_t i2 = (z + 1) * w + x;
            uint32_t i3 = i2 + 1;
            t->mesh->indices[ic++] = i0; t->mesh->indices[ic++] = i2; t->mesh->indices[ic++] = i1;
            t->mesh->indices[ic++] = i1; t->mesh->indices[ic++] = i2; t->mesh->indices[ic++] = i3;
        }
    }
    
    t->splat_texture = 0;
    t->shadow_texture = 0;
    
    return t;
}

void terrain_destroy(Terrain* t) {
    if (!t) return;
    if (t->mesh) { mesh_destroy(t->mesh); }
    if (t->splat_texture) glDeleteTextures(1, &t->splat_texture);
    if (t->shadow_texture) glDeleteTextures(1, &t->shadow_texture);
    free(t->heightmap);
    free(t);
}

void terrain_render_with_splatmap(Terrain* t, Mat4 view, Mat4 proj) {
    if (!t || !t->mesh || !t->splat_texture) return;
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, t->splat_texture);
    
    GLint loc_splat = glGetUniformLocation(shader_program, "uSplatMap");
    if (loc_splat >= 0) glUniform1i(loc_splat, 0);
    
    mesh_render(t->mesh);
}

void terrain_render(Terrain* t, Mat4 view, Mat4 proj) {
    if (!t || !t->mesh) return;
    
    if (t->splat_texture) {
        terrain_render_with_splatmap(t, view, proj);
    } else {
        mesh_render(t->mesh);
    }
}

void terrain_set_splatmap(Terrain* t, const char* splatmap_path) {
    if (!t || !splatmap_path) return;
#if HAS_STB == 1
    int w, h, ch;
    unsigned char* data = stbi_load(splatmap_path, &w, &h, &ch, 4);
    if (data) {
        if (t->splat_texture) glDeleteTextures(1, &t->splat_texture);
        glGenTextures(1, &t->splat_texture);
        glBindTexture(GL_TEXTURE_2D, t->splat_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        stbi_image_free(data);
    }
#else
    (void)splatmap_path;
    fprintf(stderr, "WARN: Splatmap loading requires stb_image.h\n");
#endif
}

void terrain_load_shadowmap(Terrain* t, const char* path) {
    if (!t || !path) return;
    
    FILE* f = fopen(path, "rb");
    if (!f) return;
    
    uint32_t w = terrain_get_width(t);
    uint32_t d = terrain_get_depth(t);
    uint32_t count = w * d;
    
    float* shadow_data = (float*)malloc(count * sizeof(float));
    if (!shadow_data) { fclose(f); return; }
    
    size_t read_count = fread(shadow_data, sizeof(float), count, f);
    fclose(f);
    
    if (read_count != count) {
        free(shadow_data);
        return;
    }
    
    if (t->shadow_texture) glDeleteTextures(1, &t->shadow_texture);
    glGenTextures(1, &t->shadow_texture);
    glBindTexture(GL_TEXTURE_2D, t->shadow_texture);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, w, d, 0, GL_RED, GL_FLOAT, shadow_data);
                 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    free(shadow_data);
}

float terrain_get_height(const Terrain* t, float x, float z) {
    if (!t || t->width < 2 || t->depth < 2) return 0.0f;
    
    float nx = (x - t->origin.x) / t->scale_xz;
    float nz = (z - t->origin.z) / t->scale_xz;
    
    int ix = (int)floorf(nx);
    int iz = (int)floorf(nz);
    
    if (ix < 0) ix = 0;
    if (ix >= (int)t->width - 1) ix = t->width - 2;
    if (iz < 0) iz = 0;
    if (iz >= (int)t->depth - 1) iz = t->depth - 2;
    
    float fx = nx - ix;
    float fz = nz - iz;
    
    float h00 = t->heightmap[iz * t->width + ix];
    float h10 = t->heightmap[iz * t->width + (ix + 1)];
    float h01 = t->heightmap[(iz + 1) * t->width + ix];
    float h11 = t->heightmap[(iz + 1) * t->width + (ix + 1)];
    
    float h_x0 = h00 * (1.0f - fx) + h10 * fx;
    float h_x1 = h01 * (1.0f - fx) + h11 * fx;
    
    return t->origin.y + (h_x0 * (1.0f - fz) + h_x1 * fz) * t->scale_y;
}

uint32_t terrain_get_width(const Terrain* t) { return t ? t->width : 0; }
uint32_t terrain_get_depth(const Terrain* t) { return t ? t->depth : 0; }
