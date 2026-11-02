#ifndef NODE_H
#define NODE_H

#include "vector3d.h"
#include <stdint.h>

typedef enum {
    NODE_BASE,
    NODE_MESH,
    NODE_CAMERA,
    NODE_LIGHT,
    NODE_SHAPE,
    NODE_GRID,
    NODE_GIZMO
} NodeType;

typedef struct Node {
    char name[64];
    NodeType type;
    Vec3 position;
    Quat rotation;
    Vec3 scale;
    uint32_t id;
    struct Node* parent;
    struct Node** children;
    uint32_t child_count;
    uint32_t child_capacity;
    int visible;
    void* user_data;
} Node;

#ifdef __cplusplus
extern "C" {
#endif

Node* node_create(NodeType type, const char* name);
void node_destroy(Node* node);
void node_add_child(Node* parent, Node* child);
void node_remove_child(Node* parent, Node* child);
Node* node_get_child(Node* parent, uint32_t index);
uint32_t node_get_child_count(Node* parent);
Node* node_find_child(Node* parent, const char* name);
void node_set_position(Node* node, Vec3 pos);
void node_set_rotation(Node* node, Quat rot);
void node_set_scale(Node* node, Vec3 scale);
Mat4 node_get_world_transform(Node* node);
void node_process(Node* node, float delta_time);
void node_render(Node* node, Mat4 view, Mat4 proj);

#ifdef __cplusplus
}
#endif

#endif /* NODE_H */
