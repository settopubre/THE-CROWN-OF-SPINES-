#ifndef SKELETON_H
#define SKELETON_H

#include "scene_graph.h"
#include <stdint.h>

typedef struct {
    uint32_t node_idx;
    uint32_t parent_bone_idx;
    float length;
    Mat4 bind_pose;
} Bone;

typedef struct {
    SceneGraph* graph;
    Bone* bones;
    uint32_t bone_count;
    uint32_t bone_capacity;
    uint32_t root_bone;
    Mat4* bone_matrices;
    uint32_t last_update_frame;
} Skeleton;

#ifdef __cplusplus
extern "C" {
#endif

Skeleton* skeleton_create(void);
void skeleton_destroy(Skeleton* skel);
uint32_t skeleton_add_bone(Skeleton* skel, uint32_t parent_bone, Vec3 head_pos, Vec3 tail_pos);
void skeleton_update_transforms(Skeleton* skel);
Mat4* skeleton_get_bone_matrices(Skeleton* skel);
uint32_t skeleton_get_bone_count(const Skeleton* skel);
const Bone* skeleton_get_bone(const Skeleton* skel, uint32_t bone_idx);
void skeleton_invalidate(Skeleton* skel);
void skeleton_tick_frame(void);

#ifdef __cplusplus
}
#endif

#endif /* SKELETON_H */
