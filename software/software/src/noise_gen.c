#include "noise_gen.h"
#include <stdlib.h>
#include <math.h>

static const int PERLIN_P[256] = {
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,
    69,142,8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,
    94,252,219,203,117,35,11,32,57,177,33,88,237,149,56,87,174,20,125,
    136,171,168,68,175,74,165,71,134,139,48,27,166,77,146,158,231,83,111,
    229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54,
    65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,200,196,
    135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,250,
    124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,
    17,182,189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,
    101,155,167,43,172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,
    185,112,104,218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,
    241,81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,157,184,
    84,204,176,115,121,50,45,127,4,150,254,138,236,205,93,222,114,67,29,
    24,72,243,141,128,195,78,66,215,61,156,180
};

static int PERM[512];
static int perm_init = 0;

static void _perm_init(void) {
    if (!perm_init) {
        for (int i = 0; i < 512; i++) PERM[i] = PERLIN_P[i & 255];
        perm_init = 1;
    }
}

static float _fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }
static float _lerp(float t, float a, float b) { return a + t * (b - a); }

/* 2D Gradient */
static float _grad2d(int hash, float x, float y) {
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : (h == 12 || h == 14 ? x : 0.0f);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

/* Phase 14: 3D Gradient */
static float _grad3d(int hash, float x, float y, float z) {
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v, w;
    if (h < 4) {
        v = y;
        w = (h == 1 || h == 2) ? 0.0f : z;
    } else if (h < 12) {
        v = (h == 8 || h == 9 || h == 10 || h == 11) ? x : z;
        w = (h == 10 || h == 11) ? x : (h == 8 || h == 9) ? y : z; 
        /* Correction for standard Perlin 3D gradient logic */
        /* Simplified standard implementation: */
        v = (h==8||h==9) ? y : z;
        w = (h==8||h==10) ? x : (h==9||h==11) ? y : z;
    } else {
        v = x; w = z; 
        /* Fallback to simple dot products with 12 edge directions */
    }
    /* Standard Ken Perlin 3D Grad: */
    switch(h & 0x0F) {
        case 0x0: return  x + y;
        case 0x1: return -x + y;
        case 0x2: return  x - y;
        case 0x3: return -x - y;
        case 0x4: return  x + z;
        case 0x5: return -x + z;
        case 0x6: return  x - z;
        case 0x7: return -x - z;
        case 0x8: return  y + z;
        case 0x9: return -y + z;
        case 0xA: return  y - z;
        case 0xB: return -y - z;
        case 0xC: return  y + x;
        case 0xD: return -y + z; /* Note: edge vectors vary, this is an approximation */
        case 0xE: return  y - x;
        case 0xF: return -y + z;
    }
    return 0;
}

float noise_perlin_2d(float x, float y, float scale, int octaves, float persistence) {
    if (!perm_init) _perm_init();
    float total = 0.0f;
    float frequency = scale;
    float amplitude = 1.0f;
    float max_value = 0.0f;
    
    for (int i = 0; i < octaves; i++) {
        float sx = x * frequency;
        float sy = y * frequency;
        int ix0 = (int)floorf(sx);
        int iy0 = (int)floorf(sy);
        float fx = sx - ix0;
        float fy = sy - iy0;
        
        float u = _fade(fx);
        float v = _fade(fy);
        
        int aa = PERM[(ix0 & 255) + PERM[(iy0 & 255)]];
        int ab = PERM[(ix0 & 255) + PERM[((iy0 + 1) & 255)]];
        int ba = PERM[((ix0 + 1) & 255) + PERM[(iy0 & 255)]];
        int bb = PERM[((ix0 + 1) & 255) + PERM[((iy0 + 1) & 255)]];
        
        float x1 = _lerp(u, _grad2d(aa, fx, fy), _grad2d(ba, fx - 1, fy));
        float x2 = _lerp(u, _grad2d(ab, fx, fy - 1), _grad2d(bb, fx - 1, fy - 1));
        total += _lerp(v, x1, x2) * amplitude;
        
        max_value += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }
    return total / max_value;
}

/* Phase 14: 3D Perlin Noise */
float noise_perlin_3d(float x, float y, float z, float scale, int octaves, float persistence) {
    if (!perm_init) _perm_init();
    float total = 0.0f;
    float frequency = scale;
    float amplitude = 1.0f;
    float max_value = 0.0f;
    
    for (int i = 0; i < octaves; i++) {
        float sx = x * frequency;
        float sy = y * frequency;
        float sz = z * frequency;
        int ix0 = (int)floorf(sx);
        int iy0 = (int)floorf(sy);
        int iz0 = (int)floorf(sz);
        
        float fx = sx - ix0;
        float fy = sy - iy0;
        float fz = sz - iz0;
        
        float u = _fade(fx);
        float v = _fade(fy);
        float w = _fade(fz);
        
        /* Hashing */
        int aaa = PERM[(ix0 & 255) + PERM[(iy0 & 255) + PERM[(iz0 & 255)]]];
        int aba = PERM[(ix0 & 255) + PERM[((iy0 + 1) & 255) + PERM[(iz0 & 255)]]];
        int aab = PERM[(ix0 & 255) + PERM[(iy0 & 255) + PERM[((iz0 + 1) & 255)]]];
        int abb = PERM[(ix0 & 255) + PERM[((iy0 + 1) & 255) + PERM[((iz0 + 1) & 255)]]];
        int baa = PERM[((ix0 + 1) & 255) + PERM[(iy0 & 255) + PERM[(iz0 & 255)]]];
        int bba = PERM[((ix0 + 1) & 255) + PERM[((iy0 + 1) & 255) + PERM[(iz0 & 255)]]];
        int bab = PERM[((ix0 + 1) & 255) + PERM[(iy0 & 255) + PERM[((iz0 + 1) & 255)]]];
        int bbb = PERM[((ix0 + 1) & 255) + PERM[((iy0 + 1) & 255) + PERM[((iz0 + 1) & 255)]]];
        
        /* Interpolate */
        float x1 = _lerp(u, _grad3d(aaa, fx, fy, fz), _grad3d(baa, fx - 1, fy, fz));
        float x2 = _lerp(u, _grad3d(aba, fx, fy - 1, fz), _grad3d(bba, fx - 1, fy - 1, fz));
        float x3 = _lerp(u, _grad3d(aab, fx, fy, fz - 1), _grad3d(bab, fx - 1, fy, fz - 1));
        float x4 = _lerp(u, _grad3d(abb, fx, fy - 1, fz - 1), _grad3d(bbb, fx - 1, fy - 1, fz - 1));
        
        float y1 = _lerp(v, x1, x2);
        float y2 = _lerp(v, x3, x4);
        float val = _lerp(w, y1, y2);
        
        total += val * amplitude;
        
        max_value += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }
    return total / max_value;
}

float noise_simplex_2d(float x, float y) {
    return noise_perlin_2d(x * 1.2f, y * 1.2f, 1.0f, 3, 0.5f);
}

float* noise_generate_heightmap(uint32_t width, uint32_t depth, float scale, int octaves, float persistence) {
    float* data = (float*)malloc(width * depth * sizeof(float));
    if (!data) return NULL;
    for (uint32_t z = 0; z < depth; z++) {
        for (uint32_t x = 0; x < width; x++) {
            float n = noise_perlin_2d((float)x, (float)z, scale, octaves, persistence);
            data[z * width + x] = (n + 1.0f) * 0.5f;
        }
    }
    return data;
}

/* Phase 14: Volumetric Noise */
float* noise_generate_3d_volume(uint32_t w, uint32_t h, uint32_t d, float scale, int octaves, float persistence) {
    uint32_t total = w * h * d;
    float* data = (float*)malloc(total * sizeof(float));
    if (!data) return NULL;
    for (uint32_t z = 0; z < d; z++) {
        for (uint32_t y = 0; y < h; y++) {
            for (uint32_t x = 0; x < w; x++) {
                float n = noise_perlin_3d((float)x, (float)y, (float)z, scale, octaves, persistence);
                data[z * w * h + y * w + x] = (n + 1.0f) * 0.5f;
            }
        }
    }
    return data;
}

void noise_free_heightmap(float* data) {
    free(data);
}
