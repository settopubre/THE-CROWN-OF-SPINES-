#ifndef NOISE_GEN_H
#define NOISE_GEN_H
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

float noise_perlin_2d(float x, float y, float scale, int octaves, float persistence);
float noise_simplex_2d(float x, float y);
float* noise_generate_heightmap(uint32_t width, uint32_t depth, float scale, int octaves, float persistence);
void noise_free_heightmap(float* data);

#ifdef __cplusplus
}
#endif

#endif /* NOISE_GEN_H */
