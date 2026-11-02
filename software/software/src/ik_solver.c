#include "skeleton.h"
#include "vector3d.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void _fabrik_backward(Skeleton* skel, uint32_t end_idx, Vec3 target) {
    Bone* end_bone = skeleton_get_bone(skel, end_idx);
    if (!end_bone) return;
    
    SceneNode* end_node = &skel->graph->nodes[end_bone->node_idx];
    end_node->local_position = target;
    end_node->dirty = 1;
    
    uint32_t curr = end_idx;
    while (skeleton_get_bone(skel, curr)->parent_bone_idx != UINT32_MAX) {
        Bone* bone = skeleton_get_bone(skel, curr);
        Bone* parent = skeleton_get_bone(skel, bone->parent_bone_idx);
        if (!bone || !parent) break;
        
        SceneNode* curr_node = &skel->graph->nodes[bone->node_idx];
        SceneNode* parent_node = &skel->graph->nodes[parent->node_idx];
        
        Vec3 dir = vec3_sub(curr_node->local_position, parent_node->local_position);
        double len = vec3_length(dir);
        if (len > 1e-6) {
            dir = vec3_mul(dir, bone->length / len);
            parent_node->local_position = vec3_sub(curr_node->local_position, dir);
            parent_node->dirty = 1;
        }
        curr = bone->parent_bone_idx;
    }
}

static void _fabrik_forward(Skeleton* skel, uint32_t root_idx) {
    Bone* root_bone = skeleton_get_bone(skel, root_idx);
    if (!root_bone) return;
    
    SceneNode* root_node = &skel->graph->nodes[root_bone->node_idx];
    Vec3 root_pos = root_node->local_position;
    
    uint32_t curr = root_idx;
    while (1) {
        Bone* bone = skeleton_get_bone(skel, curr);
        if (!bone) break;
        
        uint32_t child_idx = UINT32_MAX;
        for (uint32_t i = 0; i < skel->bone_count; i++) {
            if (skeleton_get_bone(skel, i)->parent_bone_idx == curr) {
                child_idx = i;
                break;
            }
        }
        if (child_idx == UINT32_MAX) break;
        
        Bone* child = skeleton_get_bone(skel, child_idx);
        SceneNode* curr_node = &skel->graph->nodes[bone->node_idx];
        SceneNode* child_node = &skel->graph->nodes[child->node_idx];
        
        Vec3 dir = vec3_sub(root_pos, curr_node->local_position);
        double len = vec3_length(dir);
        if (len > 1e-6) {
            dir = vec3_mul(dir, child->length / len);
            child_node->local_position = vec3_add(curr_node->local_position, dir);
            child_node->dirty = 1;
        }
        root_pos = child_node->local_position;
        curr = child_idx;
    }
}

void ik_solve_fabrik(Skeleton* skel, uint32_t end_effector_bone, Vec3 target, int max_iterations, float tolerance) {
    if (!skel || end_effector_bone >= skel->bone_count) return;
    
    for (int iter = 0; iter < max_iterations; iter++) {
        Vec3 curr_end = skel->graph->nodes[skeleton_get_bone(skel, end_effector_bone)->node_idx].local_position;
        if (vec3_distance(&curr_end, &target) < tolerance) break;
        
        _fabrik_backward(skel, end_effector_bone, target);
        uint32_t root = skel->root_bone;
        if (root != UINT32_MAX) _fabrik_forward(skel, root);
        
        skeleton_update_transforms(skel);
    }
}
