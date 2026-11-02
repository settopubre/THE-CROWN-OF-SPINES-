#include "heightmap_loader.h"
#include "vector3d.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if __has_include("stb_image.h")
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#else
#define HAS_STB 0
#endif

Heightmap* heightmap_load_raw(const char* filename, uint32_t w, uint32_t h) {
    DEFER_FILE(fp) = fopen(filename, "rb");
    if (!fp) return NULL;
    
    Heightmap* hm = (Heightmap*)calloc(1, sizeof(Heightmap));
    if (!hm) return NULL;
    hm->width = w; hm->height = h;
    hm->data = (float*)malloc(w * h * sizeof(float));
    if (!hm->data) { free(hm); return NULL; }
    
    unsigned char* buf = (unsigned char*)malloc(w * h);
    if (!buf) { free(hm->data); free(hm); return NULL; }
    
    fread(buf, 1, w * h, fp);
    
    for (uint32_t i = 0; i < w * h; i++) {
        hm->data[i] = (float)buf[i] / 255.0f;
    }
    free(buf);
    return hm;
}

Heightmap* heightmap_load_png(const char* filename) {
#if HAS_STB == 1
    int width, height, channels;
    unsigned char* img = stbi_load(filename, &width, &height, &channels, 1);
    if (!img) {
        fprintf(stderr, "WARN: stb_image failed to load %s: %s\n", filename, stbi_failure_reason());
        return NULL;
    }
    
    Heightmap* hm = (Heightmap*)calloc(1, sizeof(Heightmap));
    if (!hm) { stbi_image_free(img); return NULL; }
    
    hm->width = (uint32_t)width;
    hm->height = (uint32_t)height;
    hm->data = (float*)malloc(hm->width * hm->height * sizeof(float));
    if (!hm->data) { stbi_image_free(img); free(hm); return NULL; }
    
    for (uint32_t i = 0; i < hm->width * hm->height; i++) {
        hm->data[i] = (float)img[i] / 255.0f;
    }
    
    stbi_image_free(img);
    return hm;
#else
    (void)filename;
    fprintf(stderr, "WARN: PNG heightmap loading requires stb_image.h\n");
    return NULL;
#endif
}

void heightmap_destroy(Heightmap* hm) {
    if (hm) {
        free(hm->data);
        free(hm);
    }
}

float heightmap_get_value(const Heightmap* hm, uint32_t x, uint32_t z) {
    if (!hm || x >= hm->width || z >= hm->height) return 0.0f;
    return hm->data[z * hm->width + x];
}
