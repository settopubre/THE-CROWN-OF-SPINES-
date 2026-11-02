#include "kdtree.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define INVALID_IDX UINT32_MAX

static double _dist_sq(Vec3 a, Vec3 b) {
    double dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return dx*dx + dy*dy + dz*dz;
}

static uint32_t _insert_recursive(KDTree* tree, uint32_t node_idx, Vec3 point, uint32_t data_id, uint32_t depth) {
    if (node_idx == INVALID_IDX) {
        if (tree->count >= tree->capacity) return INVALID_IDX;
        uint32_t new_idx = tree->count++;
        KDNode* n = &tree->nodes[new_idx];
        n->point = point;
        n->axis = depth % 3;
        n->left = INVALID_IDX;
        n->right = INVALID_IDX;
        n->data_id = data_id;
        return new_idx;
    }
    
    KDNode* node = &tree->nodes[node_idx];
    double pv = (node->axis == 0) ? point.x : (node->axis == 1) ? point.y : point.z;
    double nv = (node->axis == 0) ? node->point.x : (node->axis == 1) ? node->point.y : node->point.z;
    
    if (pv < nv) {
        node->left = _insert_recursive(tree, node->left, point, data_id, depth + 1);
    } else {
        node->right = _insert_recursive(tree, node->right, point, data_id, depth + 1);
    }
    return node_idx;
}

static uint32_t _nearest_recursive(KDTree* tree, uint32_t node_idx, Vec3 query, uint32_t best_id, double* best_dist) {
    if (node_idx == INVALID_IDX) return best_id;
    
    KDNode* node = &tree->nodes[node_idx];
    double d = _dist_sq(query, node->point);
    if (d < *best_dist) {
        *best_dist = d;
        best_id = node->data_id;
    }
    
    double axis_val = (node->axis == 0) ? query.x : (node->axis == 1) ? query.y : query.z;
    double node_val = (node->axis == 0) ? node->point.x : (node->axis == 1) ? node->point.y : node->point.z;
    
    uint32_t first = (axis_val < node_val) ? node->left : node->right;
    uint32_t second = (axis_val < node_val) ? node->right : node->left;
    
    best_id = _nearest_recursive(tree, first, query, best_id, best_dist);
    
    double diff = axis_val - node_val;
    if (diff*diff < *best_dist) {
        best_id = _nearest_recursive(tree, second, query, best_id, best_dist);
    }
    return best_id;
}

static uint32_t _build_balanced(KDTree* tree, uint32_t* indices, uint32_t start, uint32_t end, uint32_t depth) {
    if (start >= end) return INVALID_IDX;
    
    uint32_t mid = (start + end) / 2;
    uint32_t axis = depth % 3;
    
    for (uint32_t i = start; i <= mid; i++) {
        uint32_t min_idx = i;
        for (uint32_t j = i + 1; j <= end; j++) {
            double vi = (axis == 0) ? tree->nodes[indices[i]].point.x : 
                       (axis == 1) ? tree->nodes[indices[i]].point.y : tree->nodes[indices[i]].point.z;
            double vj = (axis == 0) ? tree->nodes[indices[j]].point.x : 
                       (axis == 1) ? tree->nodes[indices[j]].point.y : tree->nodes[indices[j]].point.z;
            if (vj < vi) min_idx = j;
        }
        if (min_idx != i) {
            uint32_t tmp = indices[i]; indices[i] = indices[min_idx]; indices[min_idx] = tmp;
        }
    }
    
    uint32_t node_idx = indices[mid];
    tree->nodes[node_idx].axis = axis;
    
    tree->nodes[node_idx].left = _build_balanced(tree, indices, start, mid, depth + 1);
    tree->nodes[node_idx].right = _build_balanced(tree, indices, mid + 1, end, depth + 1);
    
    return node_idx;
}

KDTree* kdtree_create(uint32_t capacity) {
    if (capacity == 0) capacity = 256;
    KDTree* t = (KDTree*)calloc(1, sizeof(KDTree));
    if (!t) return NULL;
    t->nodes = (KDNode*)calloc(capacity, sizeof(KDNode));
    if (!t->nodes) { free(t); return NULL; }
    t->capacity = capacity;
    t->count = 0;
    t->root = INVALID_IDX;
    return t;
}

void kdtree_destroy(KDTree* tree) {
    if (tree) {
        free(tree->nodes);
        free(tree);
    }
}

void kdtree_insert(KDTree* tree, Vec3 point, uint32_t data_id) {
    if (!tree || tree->count >= tree->capacity) return;
    tree->root = _insert_recursive(tree, tree->root, point, data_id, 0);
}

uint32_t kdtree_nearest(KDTree* tree, Vec3 query) {
    if (!tree || tree->count == 0) return UINT32_MAX;
    double best_dist = 1e30;
    return _nearest_recursive(tree, tree->root, query, UINT32_MAX, &best_dist);
}

void kdtree_build(KDTree* tree) {
    if (!tree || tree->count == 0) return;
    
    uint32_t* indices = (uint32_t*)malloc(tree->count * sizeof(uint32_t));
    if (!indices) return;
    
    for (uint32_t i = 0; i < tree->count; i++) indices[i] = i;
    
    tree->root = _build_balanced(tree, indices, 0, tree->count - 1, 0);
    
    free(indices);
}
