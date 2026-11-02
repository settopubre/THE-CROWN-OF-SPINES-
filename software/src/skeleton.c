#include "skeleton.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define INITIAL_BONE_CAPACITY 32

/* Global frame counter definition */
static uint32_t g_engine_frame = 0;

void skeleton_tick_frame(void) {
    g_engine_frame++;
}

Skeleton* skeleton_create(void) {
    Skeleton* skel = (Skeleton*)calloc(1, sizeof(Skeleton));
    if (!skel) return NULL;
    
    skel->graph = scene_graph_create(64);
    if (!skel->graph) {
        free(skel);
        return NULL;
    }
    
    skel->bone_capacity = INITIAL_BONE_CAPACITY;
    skel->bones = (Bone*)calloc(skel->bone_capacity, sizeof(Bone));
    skel->bone_matrices = (Mat4*)calloc(skel->bone_capacity, sizeof(Mat4));
    if (!skel->bones || !skel->bone_matrices) {
        free(skel->bones);
        free(skel->bone_matrices);
        scene_graph_destroy(skel->graph);
        free(skel);
        return NULL;
    }
    
    skel->root_bone = UINT32_MAX;
    skel->last_update_frame = 0;
    return skel;
}

void skeleton_destroy(Skeleton* skel) {
    if (!skel) return;
    if (skel->graph) scene_graph_destroy(skel->graph);
    free(skel->bones);
    free(skel->bone_matrices);
    free(skel);
}

void skeleton_invalidate(Skeleton* skel) {
    if (skel) skel->last_update_frame = 0;
}

uint32_t skeleton_add_bone(Skeleton* skel, uint32_t parent_bone, Vec3 head_pos, Vec3 tail_pos) {
    if (!skel) return UINT32_MAX;
    
    if (skel->bone_count >= skel->bone_capacity) {
        uint32_t new_cap = skel->bone_capacity * 2;
        Bone* new_bones = (Bone*)realloc(skel->bones, new_cap * sizeof(Bone));
        Mat4* new_mats = (Mat4*)realloc(skel->bone_matrices, new_cap * sizeof(Mat4));
        if (!new_bones || !new_mats) return UINT32_MAX;
        skel->bones = new_bones;
        skel->bone_matrices = new_mats;
        skel->bone_capacity = new_cap;
    }
    
    uint32_t node_idx = scene_graph_add_node(skel->graph);
    if (node_idx == UINT32_MAX) return UINT32_MAX;
    
    SceneNode* node = &skel->graph->nodes[node_idx];
    node->local_position = head_pos;
    
    Vec3 delta = vec3_sub(tail_pos, head_pos);
    float length = vec3_length(delta);
    
    Bone* bone = &skel->bones[skel->bone_count];
    bone->node_idx = node_idx;
    bone->parent_bone_idx = parent_bone;
    bone->length = length;
    bone->bind_pose = mat4_identity();
    
    if (parent_bone != UINT32_MAX && parent_bone < skel->bone_count) {
        uint32_t parent_node = skel->bones[parent_bone].node_idx;
        scene_graph_set_parent(skel->graph, node_idx, parent_node);
    } else {
        skel->root_bone = skel->bone_count;
    }
    
    skeleton_invalidate(skel);
    return skel->bone_count++;
}

void skeleton_update_transforms(Skeleton* skel) {
    if (!skel || !skel->graph) return;
    
    if (skel->last_update_frame == g_engine_frame) return;
    
    scene_graph_update_world_transforms(skel->graph);
    
    for (uint32_t i = 0; i < skel->bone_count; i++) {
        Bone* bone = &skel->bones[i];
        SceneNode* node = &skel->graph->nodes[bone->node_idx];
        skel->bone_matrices[i] = node->world_transform;
    }
    
    skel->last_update_frame = g_engine_frame;
}

Mat4* skeleton_get_bone_matrices(Skeleton* skel) {
    return skel ? skel->bone_matrices : NULL;
}

uint32_t skeleton_get_bone_count(const Skeleton* skel) {
    return skel ? skel->bone_count : 0;
}

const Bone* skeleton_get_bone(const Skeleton* skel, uint32_t bone_idx) {
    if (!skel || bone_idx >= skel->bone_count) return NULL;
    return &skel->bones[bone_idx];
}
