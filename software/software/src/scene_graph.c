#include "scene_graph.h"
#include <stdlib.h>
#include <string.h>

#define INVALID_IDX UINT32_MAX
#define INITIAL_CAPACITY 64

/* Removed <stdbool.h> as requested; using int 1/0 for valid field */

static void _compute_local_transform(SceneNode* node) {
    Mat4 T = mat4_translate(node->local_position);
    Mat4 R = mat4_from_quat(node->local_rotation);
    Mat4 S = mat4_scale(node->local_scale);
    Mat4 RS = mat4_mul(R, S);
    node->local_transform = mat4_mul(T, RS);
}

static void _update_children(SceneGraph* graph, uint32_t parent_idx, Mat4 parent_world) {
    uint32_t child_idx = graph->nodes[parent_idx].first_child;
    while (child_idx != INVALID_IDX) {
        /* Check bounds and validity */
        if (child_idx < graph->count && graph->nodes[child_idx].valid) {
            SceneNode* child = &graph->nodes[child_idx];
            child->world_transform = mat4_mul(parent_world, child->local_transform);
            child->dirty = 0;

            if (child->first_child != INVALID_IDX) {
                _update_children(graph, child_idx, child->world_transform);
            }
            child_idx = child->next_sibling;
        } else {
            break;
        }
    }
}

SceneGraph* scene_graph_create(uint32_t initial_capacity) {
    if (initial_capacity == 0) initial_capacity = INITIAL_CAPACITY;
    
    SceneGraph* g = (SceneGraph*)malloc(sizeof(SceneGraph));
    if (!g) return NULL;
    
    g->nodes = (SceneNode*)calloc(initial_capacity, sizeof(SceneNode));
    g->root_indices = (uint32_t*)calloc(initial_capacity, sizeof(uint32_t));
    if (!g->nodes || !g->root_indices) {
        free(g->nodes); free(g->root_indices); free(g); return NULL;
    }
    
    g->capacity = initial_capacity;
    g->count = 0;
    g->root_count = 0;
    return g;
}

void scene_graph_destroy(SceneGraph* graph) {
    if (!graph) return;
    free(graph->root_indices);
    free(graph->nodes);
    free(graph);
}

uint32_t scene_graph_add_node(SceneGraph* graph) {
    if (!graph) return INVALID_IDX;
    
    if (graph->count >= graph->capacity) {
        uint32_t new_cap = graph->capacity * 2;
        SceneNode* new_nodes = (SceneNode*)realloc(graph->nodes, new_cap * sizeof(SceneNode));
        uint32_t* new_roots = (uint32_t*)realloc(graph->root_indices, new_cap * sizeof(uint32_t));
        if (!new_nodes || !new_roots) return INVALID_IDX;
        
        graph->nodes = new_nodes;
        graph->root_indices = new_roots;
        graph->capacity = new_cap;
    }
    
    uint32_t idx = graph->count++;
    SceneNode* n = &graph->nodes[idx];
    n->parent = INVALID_IDX;
    n->first_child = INVALID_IDX;
    n->next_sibling = INVALID_IDX;
    n->prev_sibling = INVALID_IDX;
    n->local_position = (Vec3){0, 0.0, 0.0, 0.0};
    n->local_rotation = quat_identity();
    n->local_scale = (Vec3){0, 1.0, 1.0, 1.0};
    n->dirty = 1;
    n->valid = 1; /* Set as valid active node */
    
    _compute_local_transform(n);
    n->world_transform = n->local_transform;
    
    graph->root_indices[graph->root_count++] = idx;
    return idx;
}

void scene_graph_set_parent(SceneGraph* graph, uint32_t node_idx, uint32_t parent_idx) {
    if (!graph || node_idx >= graph->count || !graph->nodes[node_idx].valid) return;
    SceneNode* node = &graph->nodes[node_idx];
    
    /* Unlink from current parent or root */
    if (node->parent != INVALID_IDX && node->parent < graph->count) {
        SceneNode* old_parent = &graph->nodes[node->parent];
        if (old_parent->first_child == node_idx) {
            old_parent->first_child = node->next_sibling;
        }
        if (node->prev_sibling != INVALID_IDX && node->prev_sibling < graph->count) {
            graph->nodes[node->prev_sibling].next_sibling = node->next_sibling;
        }
        if (node->next_sibling != INVALID_IDX && node->next_sibling < graph->count) {
            graph->nodes[node->next_sibling].prev_sibling = node->prev_sibling;
        }
    } else {
        for (uint32_t i = 0; i < graph->root_count; i++) {
            if (graph->root_indices[i] == node_idx) {
                graph->root_indices[i] = graph->root_indices[graph->root_count - 1];
                graph->root_count--;
                break;
            }
        }
    }
    
    /* Link to new parent */
    node->parent = parent_idx;
    node->prev_sibling = INVALID_IDX;
    
    if (parent_idx != INVALID_IDX && parent_idx < graph->count && graph->nodes[parent_idx].valid) {
        SceneNode* parent = &graph->nodes[parent_idx];
        node->next_sibling = parent->first_child;
        if (parent->first_child != INVALID_IDX && parent->first_child < graph->count) {
            graph->nodes[parent->first_child].prev_sibling = node_idx;
        }
        parent->first_child = node_idx;
    } else {
        node->parent = INVALID_IDX;
        node->next_sibling = INVALID_IDX;
        graph->root_indices[graph->root_count++] = node_idx;
    }
    
    node->dirty = 1;
}

void scene_graph_update_world_transforms(SceneGraph* graph) {
    if (!graph || !graph->nodes) return;
    
    for (uint32_t i = 0; i < graph->count; i++) {
        if (graph->nodes[i].valid && graph->nodes[i].dirty) {
            _compute_local_transform(&graph->nodes[i]);
        }
    }
    
    for (uint32_t i = 0; i < graph->root_count; i++) {
        uint32_t root_idx = graph->root_indices[i];
        if (root_idx < graph->count && graph->nodes[root_idx].valid) {
            graph->nodes[root_idx].world_transform = graph->nodes[root_idx].local_transform;
            if (graph->nodes[root_idx].first_child != INVALID_IDX) {
                _update_children(graph, root_idx, graph->nodes[root_idx].world_transform);
            }
        }
    }
}

void scene_graph_remove_node(SceneGraph* graph, uint32_t node_idx) {
    if (!graph || node_idx >= graph->count || !graph->nodes[node_idx].valid) return;
    
    uint32_t last_idx = graph->count - 1;
    SceneNode* target = &graph->nodes[node_idx];
    
    /* Unlink target from hierarchy */
    if (target->parent != INVALID_IDX && target->parent < graph->count) {
        SceneNode* p = &graph->nodes[target->parent];
        if (p->first_child == node_idx) p->first_child = target->next_sibling;
    } else {
        for (uint32_t i = 0; i < graph->root_count; i++) {
            if (graph->root_indices[i] == node_idx) {
                graph->root_indices[i] = graph->root_indices[graph->root_count - 1];
                graph->root_count--;
                break;
            }
        }
    }
    
    if (target->next_sibling != INVALID_IDX && target->next_sibling < graph->count) {
        graph->nodes[target->next_sibling].prev_sibling = target->prev_sibling;
    }
    if (target->prev_sibling != INVALID_IDX && target->prev_sibling < graph->count) {
        graph->nodes[target->prev_sibling].next_sibling = target->next_sibling;
    }
    
    /* If removing last node, just decrement */
    if (node_idx == last_idx) {
        target->valid = 0; /* Mark invalid */
        graph->count--;
        return;
    }
    
    /* Move last node to hole */
    SceneNode* last = &graph->nodes[last_idx];
    graph->nodes[node_idx] = *last;
    
    /* Update references to last_idx in the graph */
    for (uint32_t i = 0; i < last_idx; i++) {
        if (!graph->nodes[i].valid) continue;
        if (graph->nodes[i].parent == last_idx) graph->nodes[i].parent = node_idx;
        if (graph->nodes[i].first_child == last_idx) graph->nodes[i].first_child = node_idx;
        if (graph->nodes[i].next_sibling == last_idx) graph->nodes[i].next_sibling = node_idx;
        if (graph->nodes[i].prev_sibling == last_idx) graph->nodes[i].prev_sibling = node_idx;
    }
    for (uint32_t i = 0; i < graph->root_count; i++) {
        if (graph->root_indices[i] == last_idx) graph->root_indices[i] = node_idx;
    }
    
    /* Mark old slot as invalid */
    last->valid = 0; /* Mark invalid */
    graph->count--;
}
