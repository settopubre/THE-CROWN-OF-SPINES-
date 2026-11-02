#include "bvh.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

#define MAX_TRI_PER_LEAF 8

static AABB aabb_union(AABB a, AABB b) {
    AABB r;
    r.min.x = fminf(a.min.x, b.min.x);
    r.min.y = fminf(a.min.y, b.min.y);
    r.min.z = fminf(a.min.z, b.min.z);
    r.max.x = fmaxf(a.max.x, b.max.x);
    r.max.y = fmaxf(a.max.y, b.max.y);
    r.max.z = fmaxf(a.max.z, b.max.z);
    return r;
}

static AABB compute_tri_aabb(const float* verts, uint32_t i0, uint32_t i1, uint32_t i2) {
    AABB bbox;
    bbox.min.x = fminf(verts[i0*3], fminf(verts[i1*3], verts[i2*3]));
    bbox.min.y = fminf(verts[i0*3+1], fminf(verts[i1*3+1], verts[i2*3+1]));
    bbox.min.z = fminf(verts[i0*3+2], fminf(verts[i1*3+2], verts[i2*3+2]));
    bbox.max.x = fmaxf(verts[i0*3], fmaxf(verts[i1*3], verts[i2*3]));
    bbox.max.y = fmaxf(verts[i0*3+1], fmaxf(verts[i1*3+1], verts[i2*3+1]));
    bbox.max.z = fmaxf(verts[i0*3+2], fmaxf(verts[i1*3+2], verts[i2*3+2]));
    return bbox;
}

static int ray_aabb_intersect(const AABB* bbox, Vec3 origin, Vec3 dir, float* out_t) {
    float tmin = -FLT_MAX, tmax = FLT_MAX;
    for (int i = 0; i < 3; i++) {
        float o = (i==0)?origin.x:(i==1)?origin.y:origin.z;
        float d = (i==0)?dir.x:(i==1)?dir.y:dir.z;
        float bmin = (i==0)?bbox->min.x:(i==1)?bbox->min.y:bbox->min.z;
        float bmax = (i==0)?bbox->max.x:(i==1)?bbox->max.y:bbox->max.z;
        if (fabsf(d) < 1e-8f) {
            if (o < bmin || o > bmax) return 0;
        } else {
            float t1 = (bmin - o) / d;
            float t2 = (bmax - o) / d;
            if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
            if (t1 > tmin) tmin = t1;
            if (t2 < tmax) tmax = t2;
            if (tmin > tmax) return 0;
        }
    }
    if (tmax < 0) return 0;
    *out_t = (tmin > 0) ? tmin : tmax;
    return 1;
}

static int ray_triangle_intersect(const float* v0, const float* v1, const float* v2, 
                                   Vec3 orig, Vec3 dir, float* out_t) {
    float e1[3] = {v1[0]-v0[0], v1[1]-v0[1], v1[2]-v0[2]};
    float e2[3] = {v2[0]-v0[0], v2[1]-v0[1], v2[2]-v0[2]};
    float h[3] = {dir.y*e2[2]-dir.z*e2[1], dir.z*e2[0]-dir.x*e2[2], dir.x*e2[1]-dir.y*e2[0]};
    float a = e1[0]*h[0] + e1[1]*h[1] + e1[2]*h[2];
    if (fabsf(a) < 1e-8f) return 0;
    float f = 1.0f / a;
    float s[3] = {orig.x-v0[0], orig.y-v0[1], orig.z-v0[2]};
    float u = f * (s[0]*h[0] + s[1]*h[1] + s[2]*h[2]);
    if (u < 0 || u > 1) return 0;
    float q[3] = {s[1]*e1[2]-s[2]*e1[1], s[2]*e1[0]-s[0]*e1[2], s[0]*e1[1]-s[1]*e1[0]};
    float v = f * (dir.x*q[0] + dir.y*q[1] + dir.z*q[2]);
    if (v < 0 || u+v > 1) return 0;
    float t = f * (e2[0]*q[0] + e2[1]*q[1] + e2[2]*q[2]);
    if (t > 1e-4f) { *out_t = t; return 1; }
    return 0;
}

BVH* bvh_create(uint32_t capacity) {
    if (capacity == 0) capacity = 256;
    BVH* bvh = (BVH*)calloc(1, sizeof(BVH));
    if (!bvh) return NULL;
    bvh->nodes = (BVHNode*)calloc(capacity, sizeof(BVHNode));
    if (!bvh->nodes) { free(bvh); return NULL; }
    bvh->capacity = capacity;
    bvh->count = 0;
    bvh->root = UINT32_MAX;
    bvh->is_dirty = 0;
    bvh->src_verts = NULL;
    bvh->src_inds = NULL;
    bvh->src_tri_count = 0;
    return bvh;
}

void bvh_destroy(BVH* bvh) {
    if (bvh) { free(bvh->nodes); free(bvh); }
}

void bvh_mark_dirty(BVH* bvh) {
    if (bvh) bvh->is_dirty = 1;
}

void bvh_store_geometry_refs(BVH* bvh, const float* vertices, const uint32_t* indices, uint32_t tri_count) {
    if (bvh) {
        bvh->src_verts = vertices;
        bvh->src_inds = indices;
        bvh->src_tri_count = tri_count;
    }
}

static uint32_t _bvh_build_recursive(BVH* bvh, const float* verts, const uint32_t* indices, 
                                      uint32_t* tri_indices, uint32_t tri_start, uint32_t tri_end) {
    if (tri_end - tri_start <= MAX_TRI_PER_LEAF) {
        uint32_t node_idx = bvh->count++;
        BVHNode* node = &bvh->nodes[node_idx];
        node->bbox = compute_tri_aabb(verts, indices[tri_indices[tri_start]*3], 
                                       indices[tri_indices[tri_start]*3+1], indices[tri_indices[tri_start]*3+2]);
        for (uint32_t i = tri_start+1; i < tri_end; i++) {
            AABB tri_bbox = compute_tri_aabb(verts, indices[tri_indices[i]*3], 
                                              indices[tri_indices[i]*3+1], indices[tri_indices[i]*3+2]);
            node->bbox = aabb_union(node->bbox, tri_bbox);
        }
        node->left = node->right = UINT32_MAX;
        node->first_tri = tri_start;
        node->tri_count = tri_end - tri_start;
        return node_idx;
    }
    
    AABB centroid_bbox;
    centroid_bbox.min.x = centroid_bbox.min.y = centroid_bbox.min.z = FLT_MAX;
    centroid_bbox.max.x = centroid_bbox.max.y = centroid_bbox.max.z = -FLT_MAX;
    for (uint32_t i = tri_start; i < tri_end; i++) {
        uint32_t i0 = indices[tri_indices[i]*3], i1 = indices[tri_indices[i]*3+1], i2 = indices[tri_indices[i]*3+2];
        Vec3 c;
        c.x = (verts[i0*3]+verts[i1*3]+verts[i2*3])/3;
        c.y = (verts[i0*3+1]+verts[i1*3+1]+verts[i2*3+1])/3;
        c.z = (verts[i0*3+2]+verts[i1*3+2]+verts[i2*3+2])/3;
        if (c.x < centroid_bbox.min.x) centroid_bbox.min.x = c.x;
        if (c.y < centroid_bbox.min.y) centroid_bbox.min.y = c.y;
        if (c.z < centroid_bbox.min.z) centroid_bbox.min.z = c.z;
        if (c.x > centroid_bbox.max.x) centroid_bbox.max.x = c.x;
        if (c.y > centroid_bbox.max.y) centroid_bbox.max.y = c.y;
        if (c.z > centroid_bbox.max.z) centroid_bbox.max.z = c.z;
    }
    
    int axis = 0;
    float extent_x = centroid_bbox.max.x - centroid_bbox.min.x;
    float extent_y = centroid_bbox.max.y - centroid_bbox.min.y;
    float extent_z = centroid_bbox.max.z - centroid_bbox.min.z;
    if (extent_y > extent_x) axis = 1;
    if (extent_z > extent_x && extent_z > extent_y) axis = 2;
    
    uint32_t mid = (tri_start + tri_end) / 2;
    for (uint32_t i = tri_start; i < tri_end; i++) {
        uint32_t i0 = indices[tri_indices[i]*3], i1 = indices[tri_indices[i]*3+1], i2 = indices[tri_indices[i]*3+2];
        float cx = (verts[i0*3]+verts[i1*3]+verts[i2*3])/3;
        float cy = (verts[i0*3+1]+verts[i1*3+1]+verts[i2*3+1])/3;
        float cz = (verts[i0*3+2]+verts[i1*3+2]+verts[i2*3+2])/3;
        float cval = (axis==0)?cx:(axis==1)?cy:cz;
        float split = (centroid_bbox.min.x + centroid_bbox.max.x) * 0.5f;
        if (axis==1) split = (centroid_bbox.min.y + centroid_bbox.max.y) * 0.5f;
        if (axis==2) split = (centroid_bbox.min.z + centroid_bbox.max.z) * 0.5f;
        if (cval < split && i >= mid) {
            for (uint32_t j = i-1; j >= tri_start; j--) {
                float jx = (verts[indices[tri_indices[j]*3]*3]+verts[indices[tri_indices[j]*3+1]*3]+verts[indices[tri_indices[j]*3+2]*3])/3;
                float jy = (verts[indices[tri_indices[j]*3]*3+1]+verts[indices[tri_indices[j]*3+1]*3+1]+verts[indices[tri_indices[j]*3+2]*3+1])/3;
                float jz = (verts[indices[tri_indices[j]*3]*3+2]+verts[indices[tri_indices[j]*3+1]*3+2]+verts[indices[tri_indices[j]*3+2]*3+2])/3;
                float jval = (axis==0)?jx:(axis==1)?jy:jz;
                if (jval < split) {
                    uint32_t tmp = tri_indices[i]; tri_indices[i] = tri_indices[j]; tri_indices[j] = tmp;
                    break;
                }
            }
        }
    }
    
    uint32_t node_idx = bvh->count++;
    BVHNode* node = &bvh->nodes[node_idx];
    uint32_t left_child = _bvh_build_recursive(bvh, verts, indices, tri_indices, tri_start, mid);
    uint32_t right_child = _bvh_build_recursive(bvh, verts, indices, tri_indices, mid, tri_end);
    node->left = left_child;
    node->right = right_child;
    node->bbox = aabb_union(bvh->nodes[left_child].bbox, bvh->nodes[right_child].bbox);
    node->first_tri = 0;
    node->tri_count = 0;
    return node_idx;
}

void bvh_build(BVH* bvh, const float* vertices, const uint32_t* indices, uint32_t tri_count) {
    if (!bvh || !vertices || !indices || tri_count == 0) return;
    
    bvh_store_geometry_refs(bvh, vertices, indices, tri_count);

    bvh->count = 0;
    uint32_t* tri_indices = (uint32_t*)malloc(tri_count * sizeof(uint32_t));
    if (!tri_indices) return;
    for (uint32_t i = 0; i < tri_count; i++) tri_indices[i] = i;
    bvh->root = _bvh_build_recursive(bvh, vertices, indices, tri_indices, 0, tri_count);
    bvh->is_dirty = 0;
    free(tri_indices);
}

static int _bvh_raycast_recursive(const BVH* bvh, const float* verts, const uint32_t* indices,
                                   uint32_t node_idx, Vec3 origin, Vec3 dir, float* out_t, uint32_t* out_tri) {
    if (node_idx == UINT32_MAX) return 0;
    BVHNode* node = &bvh->nodes[node_idx];
    float t_enter;
    if (!ray_aabb_intersect(&node->bbox, origin, dir, &t_enter)) return 0;
    
    if (node->tri_count > 0) {
        float best_t = *out_t;
        uint32_t best_tri = *out_tri;
        for (uint32_t i = 0; i < node->tri_count; i++) {
            uint32_t tri_idx = node->first_tri + i;
            uint32_t i0 = indices[tri_idx*3], i1 = indices[tri_idx*3+1], i2 = indices[tri_idx*3+2];
            float t;
            if (ray_triangle_intersect(&verts[i0*3], &verts[i1*3], &verts[i2*3], origin, dir, &t)) {
                if (t < best_t) { best_t = t; best_tri = tri_idx; }
            }
        }
        if (best_tri != UINT32_MAX) { *out_t = best_t; *out_tri = best_tri; return 1; }
        return 0;
    }
    
    int hit_left = _bvh_raycast_recursive(bvh, verts, indices, node->left, origin, dir, out_t, out_tri);
    int hit_right = _bvh_raycast_recursive(bvh, verts, indices, node->right, origin, dir, out_t, out_tri);
    return hit_left || hit_right;
}

int bvh_raycast(const BVH* bvh, const float* vertices, const uint32_t* indices, 
                Vec3 origin, Vec3 dir, float* out_t, uint32_t* out_tri) {
    if (!bvh || !vertices || !indices) return 0;
    
    if (bvh->is_dirty && bvh->src_verts && bvh->src_inds) {
        BVH* mutable_bvh = (BVH*)bvh;
        bvh_build(mutable_bvh, bvh->src_verts, bvh->src_inds, bvh->src_tri_count);
    }
    
    *out_t = FLT_MAX;
    *out_tri = UINT32_MAX;
    return _bvh_raycast_recursive(bvh, vertices, indices, bvh->root, origin, dir, out_t, out_tri);
}
