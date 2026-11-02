#ifndef KDTREE_H
#define KDTREE_H
#pragma once

#include "vector3d.h"
#include <stdint.h>

typedef struct {
    Vec3 point;
    uint32_t axis;
    uint32_t left;
    uint32_t right;
    uint32_t data_id;
} KDNode;

typedef struct {
    KDNode* nodes;
    uint32_t capacity;
    uint32_t count;
    uint32_t root;
} KDTree;

#ifdef __cplusplus
extern "C" {
#endif

KDTree* kdtree_create(uint32_t capacity);
void kdtree_destroy(KDTree* tree);
void kdtree_insert(KDTree* tree, Vec3 point, uint32_t data_id);
uint32_t kdtree_nearest(KDTree* tree, Vec3 query);
void kdtree_build(KDTree* tree);

#ifdef __cplusplus
}
#endif

#endif /* KDTREE_H */
