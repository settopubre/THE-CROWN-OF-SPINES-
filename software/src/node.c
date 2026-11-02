#include "node.h"
#include <stdlib.h>
#include <string.h>

static uint32_t g_node_id_counter = 0;

Node* node_create(NodeType type, const char* name) {
    Node* n = (Node*)calloc(1, sizeof(Node));
    if (!n) return NULL;
    strncpy(n->name, name, sizeof(n->name) - 1);
    n->type = type;
    n->position = vec3_create(0.0, 0.0, 0.0);
    n->rotation = quat_identity();
    n->scale = vec3_create(1.0, 1.0, 1.0);
    n->id = ++g_node_id_counter;
    n->visible = 1;
    n->child_capacity = 4;
    n->children = (Node**)calloc(n->child_capacity, sizeof(Node*));
    return n;
}

void node_destroy(Node* node) {
    if (!node) return;
    for (uint32_t i = 0; i < node->child_count; i++) {
        node_destroy(node->children[i]);
    }
    free(node->children);
    free(node);
}

void node_add_child(Node* parent, Node* child) {
    if (!parent || !child) return;
    if (parent->child_count >= parent->child_capacity) {
        parent->child_capacity *= 2;
        parent->children = realloc(parent->children, parent->child_capacity * sizeof(Node*));
    }
    parent->children[parent->child_count++] = child;
    child->parent = parent;
}

void node_remove_child(Node* parent, Node* child) {
    if (!parent || !child) return;
    for (uint32_t i = 0; i < parent->child_count; i++) {
        if (parent->children[i] == child) {
            for (uint32_t j = i; j < parent->child_count - 1; j++) {
                parent->children[j] = parent->children[j + 1];
            }
            parent->child_count--;
            child->parent = NULL;
            break;
        }
    }
}

Node* node_get_child(Node* parent, uint32_t index) {
    return (parent && index < parent->child_count) ? parent->children[index] : NULL;
}

uint32_t node_get_child_count(Node* parent) {
    return parent ? parent->child_count : 0;
}

Node* node_find_child(Node* parent, const char* name) {
    if (!parent || !name) return NULL;
    if (strcmp(parent->name, name) == 0) return parent;
    for (uint32_t i = 0; i < parent->child_count; i++) {
        Node* found = node_find_child(parent->children[i], name);
        if (found) return found;
    }
    return NULL;
}

void node_set_position(Node* node, Vec3 pos) {
    if (node) node->position = pos;
}

void node_set_rotation(Node* node, Quat rot) {
    if (node) node->rotation = rot;
}

void node_set_scale(Node* node, Vec3 scale) {
    if (node) node->scale = scale;
}

Mat4 node_get_world_transform(Node* node) {
    if (!node) return mat4_identity();
    Mat4 local = mat4_mul(mat4_translate(node->position), mat4_mul(mat4_from_quat(node->rotation), mat4_scale(node->scale)));
    if (node->parent) return mat4_mul(node_get_world_transform(node->parent), local);
    return local;
}

void node_process(Node* node, float delta_time) {
    (void)node;
    (void)delta_time;
}

void node_render(Node* node, Mat4 view, Mat4 proj) {
    (void)node;
    (void)view;
    (void)proj;
}
