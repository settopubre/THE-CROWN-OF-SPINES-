#ifndef EDITOR_VIEWPORT_H
#define EDITOR_VIEWPORT_H

#include "vector3d.h"
#include "scene_tree.h"
#include "node.h"

struct Camera;
struct Grid;
struct EditorGizmo;

typedef struct {
    SceneTree* scene_tree;
    struct Camera* camera;
    struct Grid* grid;
    struct EditorGizmo* gizmo;
    int show_grid;
    int show_gizmo;
    int show_wireframe;
} EditorViewport;

#ifdef __cplusplus
extern "C" {
#endif

EditorViewport* editor_viewport_create(SceneTree* st);
void editor_viewport_destroy(EditorViewport* ev);
void editor_viewport_render(EditorViewport* ev);
void editor_viewport_set_camera(EditorViewport* ev, struct Camera* cam);
struct Camera* editor_viewport_get_camera(EditorViewport* ev);
void editor_viewport_toggle_grid(EditorViewport* ev);
void editor_viewport_toggle_gizmo(EditorViewport* ev);
void editor_viewport_raycast(EditorViewport* ev, Vec3 ray_origin, Vec3 ray_dir, Node** out_node, Vec3* out_point);

#ifdef __cplusplus
}
#endif

#endif /* EDITOR_VIEWPORT_H */
