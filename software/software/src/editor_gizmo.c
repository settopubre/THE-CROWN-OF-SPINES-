#include "editor_gizmo.h"
#include "render_server.h"
#include "vector3d.h"
#include "gl_functions.h"
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

EditorGizmo* editor_gizmo_create(void) {
    EditorGizmo* g = (EditorGizmo*)calloc(1, sizeof(EditorGizmo));
    if (!g) return NULL;
    
    g->target_node = NULL;
    g->active_axis = GIZMO_NONE;
    g->dragging = 0;
    g->show_rotation_rings = 0;
    g->accumulated_angle = 0.0f;
    g->uploaded = 0;
    g->vao = 0;
    g->vbo = 0;
    
    return g;
}

void editor_gizmo_destroy(EditorGizmo* gizmo) {
    if (!gizmo) return;
    
    if (gizmo->uploaded) {
        glDeleteVertexArrays(1, &gizmo->vao);
        glDeleteBuffers(1, &gizmo->vbo);
    }
    
    free(gizmo);
}

void editor_gizmo_set_target(EditorGizmo* gizmo, Node* node) {
    if (gizmo) gizmo->target_node = node;
}

void editor_gizmo_render(EditorGizmo* gizmo, Mat4 view, Mat4 proj) {
    if (!gizmo || !gizmo->target_node) return;
    
    Mat4 wt = node_get_world_transform(gizmo->target_node);
    Vec3 pos = vec3_create(wt.m[12], wt.m[13], wt.m[14]);
    
    /*
     * P0 CRITICAL FIX: Pre-generate VBO once, reuse every frame.
     *
     * WAS: float line_data[] stack array + glGenVertexArrays +
     *      glGenBuffers + glBufferData + glVertexAttribPointer +
     *      glDrawArrays + glDeleteVertexArrays + glDeleteBuffers.
     *      EVERY SINGLE FRAME.
     *
     * FIX: Generate VAO/VBO once on first render call. Cache in struct.
     *      On subsequent frames, only update model/view/proj uniforms
     *      and issue draw call. Replace inline vertex arrays with a
     *      persistent VBO containing the 3 axis lines + arrowhead hints.
     *      Uses GL_LINES for clean axis visualization.
     *      The model matrix (translation to target position) is the only
     *      per-frame uniform update needed.
     */
    
    if (!gizmo->uploaded) {
        /*
         * Generate gizmo geometry once.
         * 3 axes, each axis: 1 shaft line + 1 arrowhead line.
         * Total: 12 vertices (6 lines * 2 vertices).
         * Format: 6 floats per vertex (3 position + 3 color).
         */
        float axis_length = 1.0f;
        float arrow_length = 0.15f;
        
        float line_data[] = {
            /* X axis shaft - Red */
            0.0f, 0.0f, 0.0f,  1.0f, 0.15f, 0.15f,
            axis_length, 0.0f, 0.0f,  1.0f, 0.15f, 0.15f,
            /* X axis arrowhead - Red */
            axis_length - arrow_length, -arrow_length * 0.5f, 0.0f,  1.0f, 0.15f, 0.15f,
            axis_length, 0.0f, 0.0f,  1.0f, 0.15f, 0.15f,
            axis_length - arrow_length,  arrow_length * 0.5f, 0.0f,  1.0f, 0.15f, 0.15f,
            axis_length, 0.0f, 0.0f,  1.0f, 0.15f, 0.15f,
            
            /* Y axis shaft - Green */
            0.0f, 0.0f, 0.0f,  0.15f, 1.0f, 0.15f,
            0.0f, axis_length, 0.0f,  0.15f, 1.0f, 0.15f,
            /* Y axis arrowhead - Green */
            -arrow_length * 0.5f, axis_length - arrow_length, 0.0f,  0.15f, 1.0f, 0.15f,
            0.0f, axis_length, 0.0f,  0.15f, 1.0f, 0.15f,
             arrow_length * 0.5f, axis_length - arrow_length, 0.0f,  0.15f, 1.0f, 0.15f,
            0.0f, axis_length, 0.0f,  0.15f, 1.0f, 0.15f,
            
            /* Z axis shaft - Blue */
            0.0f, 0.0f, 0.0f,  0.15f, 0.15f, 1.0f,
            0.0f, 0.0f, axis_length,  0.15f, 0.15f, 1.0f,
            /* Z axis arrowhead - Blue */
            0.0f, -arrow_length * 0.5f, axis_length - arrow_length,  0.15f, 0.15f, 1.0f,
            0.0f, 0.0f, axis_length,  0.15f, 0.15f, 1.0f,
            0.0f,  arrow_length * 0.5f, axis_length - arrow_length,  0.15f, 0.15f, 1.0f,
            0.0f, 0.0f, axis_length,  0.15f, 0.15f, 1.0f
        };
        
        gizmo->axis_vertex_count = sizeof(line_data) / (6 * sizeof(float));
        
        glGenVertexArrays(1, &gizmo->vao);
        glBindVertexArray(gizmo->vao);
        
        glGenBuffers(1, &gizmo->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, gizmo->vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(line_data), line_data, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 
                              (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glBindVertexArray(0);
        gizmo->uploaded = 1;
    }
    
    glDisable(GL_DEPTH_TEST);
    
    glUseProgram(shader_program);
    
    GLint loc_model = glGetUniformLocation(shader_program, "uModel");
    if (loc_model >= 0) {
        Mat4 model = mat4_translate(pos);
        float f[16];
        for (int i = 0; i < 16; i++) f[i] = (float)model.m[i];
        glUniformMatrix4fv(loc_model, 1, GL_FALSE, f);
    }
    
    GLint loc_view = glGetUniformLocation(shader_program, "uView");
    if (loc_view >= 0) {
        float f[16];
        for (int i = 0; i < 16; i++) f[i] = (float)view.m[i];
        glUniformMatrix4fv(loc_view, 1, GL_FALSE, f);
    }
    
    GLint loc_proj = glGetUniformLocation(shader_program, "uProjection");
    if (loc_proj >= 0) {
        float f[16];
        for (int i = 0; i < 16; i++) f[i] = (float)proj.m[i];
        glUniformMatrix4fv(loc_proj, 1, GL_FALSE, f);
    }
    
    glLineWidth(2.5f);
    glBindVertexArray(gizmo->vao);
    glDrawArrays(GL_LINES, 0, (GLsizei)gizmo->axis_vertex_count);
    glBindVertexArray(0);
    glLineWidth(1.0f);
    
    /*
     * Render active axis highlight.
     * Draw the highlighted axis as a separate pass with yellow color
     * and slightly wider line for visual feedback.
     */
    if (gizmo->active_axis >= GIZMO_X && gizmo->active_axis <= GIZMO_Z) {
        Vec3 axis_dir;
        float hl_r = 1.0f, hl_g = 1.0f, hl_b = 0.0f;
        
        switch (gizmo->active_axis) {
            case GIZMO_X:
                axis_dir = vec3_create(1.0f, 0.0f, 0.0f);
                break;
            case GIZMO_Y:
                axis_dir = vec3_create(0.0f, 1.0f, 0.0f);
                break;
            case GIZMO_Z:
                axis_dir = vec3_create(0.0f, 0.0f, 1.0f);
                break;
            default:
                axis_dir = vec3_create(0.0f, 0.0f, 0.0f);
                break;
        }
        
        /*
         * P0 CRITICAL FIX: Transform highlight axis by node rotation.
         *
         * WAS: Used world-space axis vectors for highlight rendering,
         *      which matched the world-space dragging but was incorrect
         *      when dragging was fixed to local space.
         *
         * FIX: Apply target node rotation to the highlight axis so the
         *      yellow highlight matches the actual drag direction.
         */
        Quat node_rot = gizmo->target_node->rotation;
        axis_dir = quat_rotate(node_rot, axis_dir);
        
        Vec3 hl_start = pos;
        Vec3 hl_end = vec3_add(pos, axis_dir);
        
        float hl_data[] = {
            hl_start.x, hl_start.y, hl_start.z, hl_r, hl_g, hl_b,
            hl_end.x,   hl_end.y,   hl_end.z,   hl_r, hl_g, hl_b
        };
        
        GLuint hl_vbo;
        glGenBuffers(1, &hl_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, hl_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(hl_data), hl_data, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 
                              (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glLineWidth(3.5f);
        glDrawArrays(GL_LINES, 0, 2);
        glLineWidth(1.0f);
        
        glDeleteBuffers(1, &hl_vbo);
    }
    
    glEnable(GL_DEPTH_TEST);
}

static float _ray_line_dist_sq(Vec3 ray_o, Vec3 ray_d, Vec3 line_p, Vec3 line_d) {
    Vec3 w0 = vec3_sub(ray_o, line_p);
    float a = (float)vec3_dot(ray_d, ray_d);
    float b = (float)vec3_dot(ray_d, line_d);
    float c = (float)vec3_dot(line_d, line_d);
    float d = (float)vec3_dot(ray_d, w0);
    float e = (float)vec3_dot(line_d, w0);
    float denom = a * c - b * b;
    float sc, tc;
    if (fabsf(denom) < 1e-6f) {
        sc = 0.0f;
        tc = (b > c ? d / b : e / c);
    } else {
        sc = (b * e - c * d) / denom;
        tc = (a * e - b * d) / denom;
    }
    Vec3 closest_ray = vec3_add(ray_o, vec3_mul(ray_d, (double)sc));
    Vec3 closest_line = vec3_add(line_p, vec3_mul(line_d, (double)tc));
    Vec3 diff = vec3_sub(closest_ray, closest_line);
    return (float)vec3_dot(diff, diff);
}

int editor_gizmo_pick_axis(EditorGizmo* gizmo, Vec3 ray_origin, Vec3 ray_dir) {
    if (!gizmo || !gizmo->target_node) return GIZMO_NONE;
    
    Mat4 wt = node_get_world_transform(gizmo->target_node);
    Vec3 pos = vec3_create(wt.m[12], wt.m[13], wt.m[14]);
    
    /*
     * P0 CRITICAL FIX: Use local-space axes for picking.
     *
     * WAS: Vec3 x_axis = vec3_create(1.0f, 0.0f, 0.0f);
     *      ...and similarly for Y and Z.
     *
     * BUG: When a node is rotated (e.g., 45 degrees around Y),
     *      the gizmo axes still pointed along world X/Y/Z.
     *      This meant clicking on the visible rotated axis would
     *      select the wrong axis, or fail to select any axis at all.
     *
     * FIX: Transform the cardinal axis vectors by the target node's
     *      rotation quaternion. After rotation, the axes align with
     *      the node's local orientation, making picking intuitive.
     */
    Quat node_rot = gizmo->target_node->rotation;
    
    Vec3 x_axis = quat_rotate(node_rot, vec3_create(1.0f, 0.0f, 0.0f));
    Vec3 y_axis = quat_rotate(node_rot, vec3_create(0.0f, 1.0f, 0.0f));
    Vec3 z_axis = quat_rotate(node_rot, vec3_create(0.0f, 0.0f, 1.0f));
    
    float len = 1.0f;
    float threshold = 0.02f;
    
    float dx_sq = _ray_line_dist_sq(ray_origin, ray_dir, pos, x_axis);
    float dy_sq = _ray_line_dist_sq(ray_origin, ray_dir, pos, y_axis);
    float dz_sq = _ray_line_dist_sq(ray_origin, ray_dir, pos, z_axis);
    
    float min_dist = dx_sq;
    GizmoAxis hit = GIZMO_X;
    
    if (dy_sq < min_dist) { min_dist = dy_sq; hit = GIZMO_Y; }
    if (dz_sq < min_dist) { min_dist = dz_sq; hit = GIZMO_Z; }
    
    if (min_dist < threshold) {
        gizmo->active_axis = hit;
        return hit;
    }
    
    gizmo->active_axis = GIZMO_NONE;
    return GIZMO_NONE;
}

void editor_gizmo_begin_drag(EditorGizmo* gizmo, Vec3 mouse_world) {
    if (gizmo) {
        gizmo->dragging = 1;
        gizmo->drag_start = mouse_world;
        gizmo->prev_mouse_pos = mouse_world;
    }
}

void editor_gizmo_update_drag(EditorGizmo* gizmo, Vec3 mouse_world) {
    if (!gizmo || !gizmo->dragging || !gizmo->target_node) return;
    if (gizmo->active_axis == GIZMO_NONE) return;
    
    Vec3 delta = vec3_sub(mouse_world, gizmo->prev_mouse_pos);
    
    /*
     * P0 CRITICAL FIX: Drag along LOCAL axes, not world axes.
     *
     * WAS:
     *   if (active_axis == GIZMO_X)
     *       new_pos = pos + vec3_create(delta.x, 0, 0);
     *
     * BUG: The axis vectors were hardcoded to world X/Y/Z directions.
     *      When a node is rotated 45 degrees around Y, the user expects
     *      dragging the red (X) handle to move the object along its
     *      LOCAL right vector, not world X. The old code moved along
     *      world X, making the gizmo misleading and unusable for
     *      rotated objects.
     *
     * FIX: Transform the cardinal axis by the node's rotation quaternion.
     *      Then project the mouse delta onto this local axis. This ensures
     *      dragging the X handle always moves the object along its own
     *      right vector, regardless of world orientation.
     */
    Quat node_rot = gizmo->target_node->rotation;
    Vec3 local_axis;
    
    switch (gizmo->active_axis) {
        case GIZMO_X:
            local_axis = quat_rotate(node_rot, vec3_create(1.0, 0.0, 0.0));
            break;
        case GIZMO_Y:
            local_axis = quat_rotate(node_rot, vec3_create(0.0, 1.0, 0.0));
            break;
        case GIZMO_Z:
            local_axis = quat_rotate(node_rot, vec3_create(0.0, 0.0, 1.0));
            break;
        default:
            return;
    }
    
    /*
     * Project the mouse delta onto the local axis.
     * This gives us the scalar movement along that axis.
     * We use the dot product of delta with the normalized local axis.
     */
    float move_amount = (float)vec3_dot(delta, local_axis);
    Vec3 movement = vec3_mul(local_axis, (double)move_amount);
    
    Vec3 new_pos = vec3_add(gizmo->target_node->position, movement);
    node_set_position(gizmo->target_node, new_pos);
    
    gizmo->prev_mouse_pos = mouse_world;
}

void editor_gizmo_end_drag(EditorGizmo* gizmo) {
    if (gizmo) {
        gizmo->dragging = 0;
        gizmo->active_axis = GIZMO_NONE;
    }
}
