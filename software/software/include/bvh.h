#ifndef BVH_H
#define BVH_H

#include "vector3d.h"
#include <stdint.h>

typedef struct {
    Vec3 min;
    Vec3 max;
} AABB;

typedef struct {
    AABB bbox;
    uint32_t left;
    uint32_t right;
    uint32_t first_tri;
    uint32_t tri_count;
} BVHNode;

typedef struct {
    BVHNode* nodes;
    uint32_t capacity;
    uint32_t count;
    uint32_t root;
    
    /* Auto-rebuild support */
    int is_dirty;
    const float* src_verts;
    const uint32_t* src_inds;
    uint32_t src_tri_count;
} BVH;

/* Global BVH pointer - declared extern, defined in engine.c */
extern BVH* g_bvh;

#ifdef __cplusplus
extern "C" {
#endif

BVH* bvh_create(uint32_t capacity);
void bvh_destroy(BVH* bvh);
void bvh_build(BVH* bvh, const float* vertices, const uint32_t* indices, uint32_t tri_count);
void bvh_mark_dirty(BVH* bvh);
void bvh_store_geometry_refs(BVH* bvh, const float* vertices, const uint32_t* indices, uint32_t tri_count);
int bvh_raycast(const BVH* bvh, const float* vertices, const uint32_t* indices, Vec3 origin, Vec3 dir, float* out_t, uint32_t* out_tri);

#ifdef __cplusplus
}
#endif

#endif /* BVH_H */
