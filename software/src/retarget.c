#include "skeleton.h"
#include "ai_thread.h"
#include "ik_solver.h"
#include <string.h>

void retarget_pose(Skeleton* target_skel, const AIPose* src_pose, const uint32_t* bone_map) {
    if (!target_skel || !src_pose || !bone_map) return;
    
    for (uint32_t i = 0; i < target_skel->bone_count && i < src_pose->joint_count; i++) {
        uint32_t src_idx = bone_map[i];
        if (src_idx >= src_pose->joint_count) continue;
        
        Bone* bone = skeleton_get_bone(target_skel, i);
        if (!bone) continue;
        
        SceneNode* node = &target_skel->graph->nodes[bone->node_idx];
        node->local_position = src_pose->joint_positions[src_idx];
        node->local_rotation = src_pose->joint_rotations[src_idx];
        node->dirty = 1;
    }
    
    skeleton_update_transforms(target_skel);
    
    /* Optional: Apply FABRIK IK for end effectors */
    /* ik_solve_fabrik(target_skel, hand_bone_idx, target_pos, 10, 0.01f); */
}
