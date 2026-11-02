#include "transform_tool.h"
#include "gl_functions.h"
#include <stdlib.h>
#include <math.h>

TransformTool* transform_tool_create(Editor* ed) {
    TransformTool* tt = (TransformTool*)calloc(1, sizeof(TransformTool));
    if (!tt) return NULL;
    tt->editor = ed;
    tt->active_axis = AXIS_X;
    
    float line_data[] = {
        0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f
    };
    
    glGenVertexArrays(1, &tt->vao);
    glBindVertexArray(tt->vao);
    glGenBuffers(1, &tt->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, tt->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(line_data), line_data, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    tt->uploaded = 1;
    
    return tt;
}

void transform_tool_destroy(TransformTool* tt) {
    if (!tt) return;
    if (tt->uploaded) {
        glDeleteVertexArrays(1, &tt->vao);
        glDeleteBuffers(1, &tt->vbo);
    }
    free(tt);
}

void transform_tool_begin_drag(TransformTool* tt, TransformAxis axis, Vec3 mouse_world) {
    if (!tt) return;
    tt->active_axis = axis;
    tt->dragging = 1;
    tt->drag_start = mouse_world;
    tt->drag_value = 0.0f;
}

void transform_tool_update_drag(TransformTool* tt, Vec3 mouse_world) {
    if (!tt || !tt->dragging || !tt->editor) return;
    
    Vec3 delta = vec3_sub(mouse_world, tt->drag_start);
    
    switch (tt->active_axis) {
        case AXIS_X:
            editor_move_selected(tt->editor, vec3_create(delta.x, 0.0f, 0.0f));
            break;
        case AXIS_Y:
            editor_move_selected(tt->editor, vec3_create(0.0f, delta.y, 0.0f));
            break;
        case AXIS_Z:
            editor_move_selected(tt->editor, vec3_create(0.0f, 0.0f, delta.z));
            break;
        case AXIS_XYZ:
            editor_move_selected(tt->editor, delta);
            break;
        default:
            break;
    }
    tt->drag_start = mouse_world;
}

void transform_tool_end_drag(TransformTool* tt) {
    if (!tt) return;
    tt->dragging = 0;
    tt->active_axis = AXIS_X;
}

static float _ray_line_dist_sq(Vec3 ray_o, Vec3 ray_d, Vec3 line_p, Vec3 line_d) {
    Vec3 w0 = vec3_sub(ray_o, line_p);
    float a = vec3_dot(ray_d, ray_d);
    float b = vec3_dot(ray_d, line_d);
    float c = vec3_dot(line_d, line_d);
    float d = vec3_dot(ray_d, w0);
    float e = vec3_dot(line_d, w0);
    float denom = a * c - b * b;
    float sc, tc;
    if (denom < 1e-6) { sc = 0.0f; tc = (b > c ? d / b : e / c); }
    else { sc = (b * e - c * d) / denom; tc = (a * e - b * d) / denom; }
    Vec3 closest_ray = vec3_add(ray_o, vec3_mul(ray_d, sc));
    Vec3 closest_line = vec3_add(line_p, vec3_mul(line_d, tc));
    Vec3 diff = vec3_sub(closest_ray, closest_line);
    return vec3_dot(diff, diff);
}

int transform_tool_hit_test(TransformTool* tt, Vec3 ray_origin, Vec3 ray_dir) {
    if (!tt) return 0;

    Vec3 pivot = vec3_create(0.0f, 0.0f, 0.0f);
    if (tt->editor && tt->editor->selected_asset_id != UINT32_MAX) {
        AssetInstance* inst = tt->editor->placed_objects[tt->editor->selected_asset_id];
        if (inst) pivot = inst->position;
    }

    float len = 1.0f;
    float threshold = 0.01f;

    Vec3 x_axis = vec3_create(1.0f, 0.0f, 0.0f);
    Vec3 y_axis = vec3_create(0.0f, 1.0f, 0.0f);
    Vec3 z_axis = vec3_create(0.0f, 0.0f, 1.0f);

    float dx_sq = _ray_line_dist_sq(ray_origin, ray_dir, pivot, x_axis);
    float dy_sq = _ray_line_dist_sq(ray_origin, ray_dir, pivot, y_axis);
    float dz_sq = _ray_line_dist_sq(ray_origin, ray_dir, pivot, z_axis);

    float min_dist = dx_sq;
    TransformAxis hit = AXIS_X;

    if (dy_sq < min_dist) { min_dist = dy_sq; hit = AXIS_Y; }
    if (dz_sq < min_dist) { min_dist = dz_sq; hit = AXIS_Z; }

    if (min_dist < threshold) {
        return 1;
    }
    
    return 0;
}

void transform_tool_render(TransformTool* tt, Mat4 view, Mat4 proj) {
    if (!tt || !tt->editor || !tt->uploaded) return;

    Vec3 pos = vec3_create(0.0f, 0.0f, 0.0f);
    if (tt->editor->selected_asset_id != UINT32_MAX && tt->editor->placed_objects) {
        AssetInstance* inst = tt->editor->placed_objects[tt->editor->selected_asset_id];
        if (inst) pos = inst->position;
    }

    glUseProgram(shader_program);
    glLineWidth(2.5f);

    GLint loc_view = glGetUniformLocation(shader_program, "uView");
    if (loc_view >= 0) {
        float f[16]; for(int i=0;i<16;i++) f[i]=(float)view.m[i];
        glUniformMatrix4fv(loc_view, 1, GL_FALSE, f);
    }
    GLint loc_proj = glGetUniformLocation(shader_program, "uProjection");
    if (loc_proj >= 0) {
        float f[16]; for(int i=0;i<16;i++) f[i]=(float)proj.m[i];
        glUniformMatrix4fv(loc_proj, 1, GL_FALSE, f);
    }
    GLint loc_model = glGetUniformLocation(shader_program, "uModel");
    if (loc_model >= 0) {
        Mat4 m = mat4_translate(pos);
        float f[16]; for(int i=0;i<16;i++) f[i]=(float)m.m[i];
        glUniformMatrix4fv(loc_model, 1, GL_FALSE, f);
    }

    glBindVertexArray(tt->vao);
    glDrawArrays(GL_LINES, 0, 6);
    glBindVertexArray(0);

    glLineWidth(1.0f);
}
