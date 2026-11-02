#ifndef SCENE_GRAPH_H
#define SCENE_GRAPH_H

#include <stdint.h>
#include "vector3d.h"

typedef struct {
    uint32_t parent;
    uint32_t first_child;
    uint32_t next_sibling;
    uint32_t prev_sibling;
    Vec3 local_position;
    Quat local_rotation;
    Vec3 local_scale;
    Mat4 local_transform;
    Mat4 world_transform;
    uint32_t dirty;
    int valid; /* NEW FIELD: Tracks active state of node */
} SceneNode;

typedef struct {
    SceneNode* nodes;
    uint32_t capacity;
    uint32_t count;
    uint32_t* root_indices;
    uint32_t root_count;
} SceneGraph;

#ifdef __cplusplus
extern "C" {
#endif

SceneGraph* scene_graph_create(uint32_t initial_capacity);
void scene_graph_destroy(SceneGraph* graph);
uint32_t scene_graph_add_node(SceneGraph* graph);
void scene_graph_set_parent(SceneGraph* graph, uint32_t node_idx, uint32_t parent_idx);
void scene_graph_update_world_transforms(SceneGraph* graph);
void scene_graph_remove_node(SceneGraph* graph, uint32_t node_idx);

#ifdef __cplusplus
}
#endif

#endif /* SCENE_GRAPH_H */
