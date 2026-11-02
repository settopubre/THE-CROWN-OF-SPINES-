#ifndef EDITOR_GIZMO_H
#define EDITOR_GIZMO_H

#include "vector3d.h"
#include "node.h"
#include "gpu_handle.h"
#include <stdint.h>

typedef enum {
    GIZMO_X,
    GIZMO_Y,
    GIZMO_Z,
    GIZMO_XY,
    GIZMO_XZ,
    GIZMO_YZ,
    GIZMO_XYZ,
    GIZMO_NONE
} GizmoAxis;

typedef struct {
    Node* target_node;
    GizmoAxis active_axis;
    int dragging;
    Vec3 drag_start;
    Vec3 prev_mouse_pos;
    int show_rotation_rings;
    float accumulated_angle;
    
    /* GPU resources for persistent gizmo rendering */
    GPUHandle vao, vbo;
    uint32_t axis_vertex_count;
    int uploaded;
} EditorGizmo;

#ifdef __cplusplus
extern "C" {
#endif

EditorGizmo* editor_gizmo_create(void);
void editor_gizmo_destroy(EditorGizmo* gizmo);
void editor_gizmo_set_target(EditorGizmo* gizmo, Node* node);
void editor_gizmo_render(EditorGizmo* gizmo, Mat4 view, Mat4 proj);
int editor_gizmo_pick_axis(EditorGizmo* gizmo, Vec3 ray_origin, Vec3 ray_dir);
void editor_gizmo_begin_drag(EditorGizmo* gizmo, Vec3 mouse_world);
void editor_gizmo_update_drag(EditorGizmo* gizmo, Vec3 mouse_world);
void editor_gizmo_end_drag(EditorGizmo* gizmo);

#ifdef __cplusplus
}
#endif

#endif /* EDITOR_GIZMO_H */
