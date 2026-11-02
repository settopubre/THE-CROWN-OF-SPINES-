#include "editor_viewport.h"
#include "editor_gizmo.h"
#include "render_server.h"
#include "grid.h"
#include "engine_abi.h"
#include "bvh.h"
#include <stdlib.h>
#include <math.h>

extern BVH* g_bvh; /* External reference to global BVH */

static void _raycast_recursive(Node* node, Vec3 ray_origin, Vec3 ray_dir, Node** out_node, Vec3* out_point, float* out_dist) {
    if (!node || !node->visible) return;
    
    /* Simple bounding sphere raycast check */
    Mat4 wt = node_get_world_transform(node);
    Vec3 center = vec3_create(wt.m[12], wt.m[13], wt.m[14]);
    float radius = 1.0f;
    
    Vec3 oc = vec3_sub(ray_origin, center);
    float a = vec3_dot(ray_dir, ray_dir);
    float b = 2.0f * vec3_dot(oc, ray_dir);
    float c = vec3_dot(oc, oc) - radius * radius;
    float discriminant = b * b - 4 * a * c;
    
    if (discriminant > 0) {
        float t = (-b - sqrtf(discriminant)) / (2 * a);
        if (t > 0 && t < *out_dist) {
            *out_dist = t;
            *out_node = node;
            *out_point = vec3_add(ray_origin, vec3_mul(ray_dir, t));
        }
    }
    
    for (uint32_t i = 0; i < node_get_child_count(node); i++) {
        _raycast_recursive(node_get_child(node, i), ray_origin, ray_dir, out_node, out_point, out_dist);
    }
}

EditorViewport* editor_viewport_create(SceneTree* st) {
    EditorViewport* ev = (EditorViewport*)calloc(1, sizeof(EditorViewport));
    if (!ev) return NULL;
    ev->scene_tree = st;
    ev->show_grid = 1;
    ev->show_gizmo = 1;
    ev->gizmo = editor_gizmo_create();
    
    /* Initialize BVH if scene geometry is available */
    if (st && g_bvh) {
        /* In a real scenario, we would build the BVH from the SceneTree meshes here.
           We assume g_bvh is pre-built by the engine. */
    }
    
    return ev;
}

void editor_viewport_destroy(EditorViewport* ev) {
    if (!ev) return;
    if (ev->gizmo) editor_gizmo_destroy(ev->gizmo);
    free(ev);
}

void editor_viewport_render(EditorViewport* ev) {
    if (!ev) return;
    
    Mat4 view = camera_get_view_matrix(ev->camera);
    Mat4 proj = camera_get_projection_matrix(ev->camera, 800.0f / 600.0f);
    
    render_server_begin_frame(view, proj);
    
    if (ev->scene_tree) {
        scene_tree_render(ev->scene_tree, view, proj);
    }
    
    if (ev->show_grid && ev->grid) {
        grid_render(ev->grid, view, proj);
    }
    
    if (ev->show_gizmo && ev->gizmo) {
        editor_gizmo_render(ev->gizmo, view, proj);
    }
}

void editor_viewport_set_camera(EditorViewport* ev, struct Camera* cam) {
    if (ev) ev->camera = cam;
}

struct Camera* editor_viewport_get_camera(EditorViewport* ev) {
    return ev ? ev->camera : NULL;
}

void editor_viewport_toggle_grid(EditorViewport* ev) {
    if (ev) ev->show_grid = !ev->show_grid;
}

void editor_viewport_toggle_gizmo(EditorViewport* ev) {
    if (ev) ev->show_gizmo = !ev->show_gizmo;
}

void editor_viewport_raycast(EditorViewport* ev, Vec3 ray_origin, Vec3 ray_dir, Node** out_node, Vec3* out_point) {
    if (!ev) return;
    *out_node = NULL;
    *out_point = vec3_create(0.0, 0.0, 0.0);
    float dist = 1e30f;
    
    /* Integration: Use BVH if available for the scene */
    if (g_bvh && g_bvh->root != UINT32_MAX) {
        /* We need mesh vertices to perform BVH raycast. 
           Assuming the BVH is built from the terrain or a specific mesh.
           If the BVH represents the whole scene, we map the hit triangle to a Node.
           For this implementation, we use BVH to get the hit point, then find nearest node. */
        
        float t_hit;
        uint32_t tri_idx;
        /* Note: bvh_raycast requires vertices/indices. We assume g_bvh holds pointers or we use a helper.
           Since the structure is decoupled, we fall back to sphere check if we can't access geometry here. */
           
        /* Placeholder for BVH hit test logic:
           if (bvh_raycast(g_bvh, scene_vertices, scene_indices, ray_origin, ray_dir, &t_hit, &tri_idx)) {
               *out_point = vec3_add(ray_origin, vec3_mul(ray_dir, t_hit));
               *out_node = find_node_containing_point(ev->scene_tree, *out_point);
               return;
           }
        */
    }

    /* Fallback: Recursive bounding sphere check */
    if (ev->scene_tree) {
        _raycast_recursive(scene_tree_get_root(ev->scene_tree), ray_origin, ray_dir, out_node, out_point, &dist);
    }
}
