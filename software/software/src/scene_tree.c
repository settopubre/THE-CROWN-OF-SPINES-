#include "scene_tree.h"
#include "render_server.h"
#include "mesh.h"
#include <stdlib.h>
#include <string.h>

static uint32_t _count_nodes(Node* node) {
    if (!node) return 0;
    uint32_t count = 1;
    for (uint32_t i = 0; i < node->child_count; i++) count += _count_nodes(node->children[i]);
    return count;
}

static void _process_nodes(Node* node, float dt) {
    if (!node || !node->visible) return;
    node_process(node, dt);
    for (uint32_t i = 0; i < node->child_count; i++) _process_nodes(node->children[i], dt);
}

static void _render_nodes(Node* node, Mat4 view, Mat4 proj) {
    if (!node || !node->visible) return;
    
    /* Phase 14: Actual implementation for Mesh nodes */
    if (node->type == NODE_MESH) {
        /* Assuming user_data holds the Mesh pointer */
        Mesh* m = (Mesh*)node->user_data;
        if (m) {
            Mat4 world = node_get_world_transform(node);
            render_server_draw_mesh(m, world);
        }
    }
    
    /* Generic node render hook (if any specific node logic is needed) */
    node_render(node, view, proj);
    
    for (uint32_t i = 0; i < node->child_count; i++) _render_nodes(node->children[i], view, proj);
}

SceneTree* scene_tree_create(void) {
    SceneTree* st = (SceneTree*)calloc(1, sizeof(SceneTree));
    if (st) {
        st->running = 1;
        st->frame_count = 0;
    }
    return st;
}

void scene_tree_destroy(SceneTree* st) {
    if (!st) return;
    if (st->root) node_destroy(st->root);
    free(st);
}

void scene_tree_set_root(SceneTree* st, Node* root) {
    if (st) st->root = root;
}

Node* scene_tree_get_root(SceneTree* st) {
    return st ? st->root : NULL;
}

void scene_tree_process(SceneTree* st, float delta_time) {
    if (!st || !st->running || !st->root) return;
    st->delta_time = delta_time;
    st->frame_count++;
    _process_nodes(st->root, delta_time);
}

void scene_tree_render(SceneTree* st, Mat4 view, Mat4 proj) {
    if (!st || !st->root) return;
    _render_nodes(st->root, view, proj);
}

void scene_tree_quit(SceneTree* st) {
    if (st) st->running = 0;
}

int scene_tree_is_running(SceneTree* st) {
    return st && st->running;
}

uint32_t scene_tree_get_node_count(SceneTree* st) {
    return st ? _count_nodes(st->root) : 0;
}

Node* scene_tree_find_node(SceneTree* st, const char* name) {
    return st ? node_find_child(st->root, name) : NULL;
}
