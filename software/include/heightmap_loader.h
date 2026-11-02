#ifndef HEIGHTMAP_LOADER_H
#define HEIGHTMAP_LOADER_H
#pragma once

#include <stdint.h>

typedef struct {
    float* data;
    uint32_t width, height;
} Heightmap;

#ifdef __cplusplus
extern "C" {
#endif

Heightmap* heightmap_load_png(const char* filename);
Heightmap* heightmap_load_raw(const char* filename, uint32_t w, uint32_t h);
void heightmap_destroy(Heightmap* hm);
float heightmap_get_value(const Heightmap* hm, uint32_t x, uint32_t z);

#ifdef __cplusplus
}
#endif

#endif /* HEIGHTMAP_LOADER_H */
