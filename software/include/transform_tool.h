#ifndef TRANSFORM_TOOL_H
#define TRANSFORM_TOOL_H
#pragma once

#include "vector3d.h"
#include "editor.h"
#include "gpu_handle.h"

typedef enum {
    AXIS_X,
    AXIS_Y,
    AXIS_Z,
    AXIS_XY,
    AXIS_XZ,
    AXIS_YZ,
    AXIS_XYZ
} TransformAxis;

typedef struct {
    Editor* editor;
    TransformAxis active_axis;
    int dragging;
    Vec3 drag_start;
    float drag_value;
    GPUHandle vao;
    GPUHandle vbo;
    int uploaded;
} TransformTool;

#ifdef __cplusplus
extern "C" {
#endif

TransformTool* transform_tool_create(Editor* ed);
void transform_tool_destroy(TransformTool* tt);
void transform_tool_begin_drag(TransformTool* tt, TransformAxis axis, Vec3 mouse_world);
void transform_tool_update_drag(TransformTool* tt, Vec3 mouse_world);
void transform_tool_end_drag(TransformTool* tt);
void transform_tool_render(TransformTool* tt, Mat4 view, Mat4 proj);
int transform_tool_hit_test(TransformTool* tt, Vec3 ray_origin, Vec3 ray_dir);

#ifdef __cplusplus
}
#endif

#endif /* TRANSFORM_TOOL_H */
