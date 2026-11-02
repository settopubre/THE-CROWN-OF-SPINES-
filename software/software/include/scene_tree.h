#ifndef SCENE_TREE_H
#define SCENE_TREE_H

#include "node.h"
#include <stdint.h>

typedef struct {
    Node* root;
    float delta_time;
    int running;
    int frame_count;
} SceneTree;

#ifdef __cplusplus
extern "C" {
#endif

SceneTree* scene_tree_create(void);
void scene_tree_destroy(SceneTree* st);
void scene_tree_set_root(SceneTree* st, Node* root);
Node* scene_tree_get_root(SceneTree* st);
void scene_tree_process(SceneTree* st, float delta_time);
void scene_tree_render(SceneTree* st, Mat4 view, Mat4 proj);
void scene_tree_quit(SceneTree* st);
int scene_tree_is_running(SceneTree* st);
uint32_t scene_tree_get_node_count(SceneTree* st);
Node* scene_tree_find_node(SceneTree* st, const char* name);

#ifdef __cplusplus
}
#endif

#endif /* SCENE_TREE_H */
