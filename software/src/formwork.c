#include "formwork.h"
#include "gl_functions.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Formwork* formwork_generate(const Vec3* perimeter, uint32_t count, float slab_thickness) {
    if (!perimeter || count < 3) return NULL;
    
    Formwork* fw = (Formwork*)calloc(1, sizeof(Formwork));
    if (!fw) return NULL;
    
    fw->capacity = count * 4;
    fw->panels = (FormPanel*)calloc(fw->capacity, sizeof(FormPanel));
    if (!fw->panels) {
        free(fw);
        return NULL;
    }
    
    Lumber lumber;
    if (slab_thickness <= 5.0f) {
        lumber.width = 3.5f;
        lumber.thickness = 1.5f;
    } else {
        lumber.width = 5.5f;
        lumber.thickness = 1.5f;
    }
    lumber.cost_per_ft = 1.20f;
    
    fw->total_bf = 0.0f;
    fw->total_cost = 0.0f;
    fw->count = 0;
    
    for (uint32_t i = 0; i < count; i++) {
        Vec3 p1 = perimeter[i];
        Vec3 p2 = perimeter[(i + 1) % count];
        Vec3 dir = vec3_sub(p2, p1);
        float len = vec3_length(dir);
        
        if (len < 0.001f) continue;
        
        dir = vec3_normalize(dir);
        
        /*
         * Compute edge normal (perpendicular in XZ plane) for proper
         * board placement. Rotate direction vector 90 degrees around Y axis.
         */
        Quat board_rot;
        float yaw = atan2f(dir.x, dir.z);
        board_rot = quat_from_axis_angle(vec3_create(0.0, 1.0, 0.0), yaw);
        
        /* Main form board */
        if (fw->count >= fw->capacity) break;
        FormPanel* board = &fw->panels[fw->count++];
        board->position = vec3_add(p1, vec3_mul(dir, len * 0.5f));
        board->position.y += slab_thickness * 0.5f;
        board->size = vec3_create(len, slab_thickness, lumber.thickness / 12.0f);
        board->rotation = board_rot;
        board->is_stake = 0;
        
        float bf = (len * 12.0f) * lumber.width * lumber.thickness / 144.0f;
        fw->total_bf += bf;
        fw->total_cost += bf * lumber.cost_per_ft;
        
        /* Stakes every 24 inches (2 ft) */
        int num_stakes = (int)(len / 2.0f) + 1;
        for (int s = 0; s < num_stakes && fw->count < fw->capacity; s++) {
            FormPanel* stake = &fw->panels[fw->count++];
            float t = (num_stakes > 1) ? (float)s / (float)(num_stakes - 1) : 0.5f;
            stake->position = vec3_lerp(p1, p2, t);
            stake->position.y += (slab_thickness + 0.5f) * 0.5f;
            stake->size = vec3_create(0.1f, slab_thickness + 0.5f, 0.1f);
            stake->is_stake = 1;
            stake->rotation = quat_identity();
            
            float stake_bf = (stake->size.y * 12.0f) * 1.5f * 1.5f / 144.0f;
            fw->total_bf += stake_bf;
            fw->total_cost += stake_bf * lumber.cost_per_ft;
        }
    }
    
    /*
     * P0 CRITICAL FIX: Pre-generate GPU geometry at creation time.
     *
     * WAS: glGenVertexArrays + glGenBuffers + glBufferData +
     *      glDeleteVertexArrays + glDeleteBuffers inside formwork_render()
     *      EVERY SINGLE FRAME. This caused:
     *      1. Main thread GPU pipeline stalls on every draw
     *      2. Driver allocator thrashing creating/destroying objects
     *      3. Panels/stakes rendered as disconnected GL_POINTS,
     *         making the formwork appear as scattered dots instead
     *         of structural lumber visualization
     *
     * FIX: Allocate VAO/VBO once in formwork_generate(). Generate
     *      proper triangle geometry for each panel/stake as oriented
     *      boxes. Upload with GL_STATIC_DRAW since formwork geometry
     *      doesn't change after generation. Render with GL_TRIANGLES
     *      for solid structural visualization. Clean up in
     *      formwork_destroy().
     */
    
    /*
     * Each panel/stake is rendered as a wireframe box (12 lines = 24 vertices
     * per box). We use GL_LINES for clear structural visualization.
     * 8 corners * 12 edges * 2 vertices per edge = 24 vertices per box.
     */
    uint32_t boxes_vertex_count = fw->count * 24;
    float* vertex_data = (float*)malloc(boxes_vertex_count * 6 * sizeof(float));
    
    if (vertex_data) {
        uint32_t vi = 0;
        
        for (uint32_t p = 0; p < fw->count; p++) {
            FormPanel* panel = &fw->panels[p];
            
            float hw = panel->size.x * 0.5f;
            float hh = panel->size.y * 0.5f;
            float hd = panel->size.z * 0.5f;
            
            /*
             * 8 corners of the oriented box in local space.
             * Apply rotation to get world-space corners.
             * Indices: 0=---, 1=+--, 2=++-, 3=-+-, 
             *          4=--+, 5=+-+, 6=+++, 7=-++
             */
            Vec3 local_corners[8] = {
                vec3_create(-hw, -hh, -hd), vec3_create( hw, -hh, -hd),
                vec3_create( hw,  hh, -hd), vec3_create(-hw,  hh, -hd),
                vec3_create(-hw, -hh,  hd), vec3_create( hw, -hh,  hd),
                vec3_create( hw,  hh,  hd), vec3_create(-hw,  hh,  hd)
            };
            
            Vec3 world_corners[8];
            for (int c = 0; c < 8; c++) {
                world_corners[c] = quat_rotate(panel->rotation, local_corners[c]);
                world_corners[c] = vec3_add(world_corners[c], panel->position);
            }
            
            /*
             * 12 edges of the box. Each edge is a pair of corner indices.
             */
            int edges[12][2] = {
                {0, 1}, {1, 2}, {2, 3}, {3, 0}, /* bottom face */
                {4, 5}, {5, 6}, {6, 7}, {7, 4}, /* top face */
                {0, 4}, {1, 5}, {2, 6}, {3, 7}  /* vertical edges */
            };
            
            /*
             * Color scheme:
             * - Form boards (panels): warm tan/brown (0.85, 0.70, 0.50)
             * - Stakes: darker brown (0.55, 0.35, 0.15)
             */
            float r, g, b;
            if (panel->is_stake) {
                r = 0.55f; g = 0.35f; b = 0.15f;
            } else {
                r = 0.85f; g = 0.70f; b = 0.50f;
            }
            
            for (int e = 0; e < 12; e++) {
                int i0 = edges[e][0];
                int i1 = edges[e][1];
                
                /* First vertex of edge */
                vertex_data[vi * 6 + 0] = world_corners[i0].x;
                vertex_data[vi * 6 + 1] = world_corners[i0].y;
                vertex_data[vi * 6 + 2] = world_corners[i0].z;
                vertex_data[vi * 6 + 3] = r;
                vertex_data[vi * 6 + 4] = g;
                vertex_data[vi * 6 + 5] = b;
                vi++;
                
                /* Second vertex of edge */
                vertex_data[vi * 6 + 0] = world_corners[i1].x;
                vertex_data[vi * 6 + 1] = world_corners[i1].y;
                vertex_data[vi * 6 + 2] = world_corners[i1].z;
                vertex_data[vi * 6 + 3] = r;
                vertex_data[vi * 6 + 4] = g;
                vertex_data[vi * 6 + 5] = b;
                vi++;
            }
        }
        
        fw->vertex_count = vi;
        
        glGenVertexArrays(1, &fw->vao);
        glBindVertexArray(fw->vao);
        
        glGenBuffers(1, &fw->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, fw->vbo);
        glBufferData(GL_ARRAY_BUFFER, vi * 6 * sizeof(float), vertex_data, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 
                              (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glBindVertexArray(0);
        
        free(vertex_data);
        fw->uploaded = 1;
    }
    
    return fw;
}

void formwork_destroy(Formwork* fw) {
    if (!fw) return;
    
    if (fw->uploaded) {
        glDeleteVertexArrays(1, &fw->vao);
        glDeleteBuffers(1, &fw->vbo);
    }
    
    free(fw->panels);
    free(fw);
}

void formwork_render(Formwork* fw, Mat4 view, Mat4 proj) {
    if (!fw || fw->count == 0 || !fw->uploaded || fw->vertex_count == 0) return;
    
    /*
     * P0 CRITICAL FIX: Direct draw, no per-frame allocation.
     *
     * WAS: malloc vertex data + glGenVertexArrays + glGenBuffers +
     *      glBufferData + glVertexAttribPointer * 2 + glDrawArrays(GL_POINTS) +
     *      glDeleteVertexArrays + glDeleteBuffers + free(data).
     *      EVERY SINGLE FRAME.
     *
     * NOW: Bind pre-uploaded VAO and draw as GL_LINES. The geometry
     *      was generated once in formwork_generate() with proper oriented
     *      box wireframes for each panel and stake. Zero per-frame
     *      allocation, zero GPU object creation/destruction.
     */
    
    glUseProgram(shader_program);
    
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
    
    GLint loc_model = glGetUniformLocation(shader_program, "uModel");
    if (loc_model >= 0) {
        Mat4 identity = mat4_identity();
        float f[16];
        for (int i = 0; i < 16; i++) f[i] = (float)identity.m[i];
        glUniformMatrix4fv(loc_model, 1, GL_FALSE, f);
    }
    
    glLineWidth(1.5f);
    glBindVertexArray(fw->vao);
    glDrawArrays(GL_LINES, 0, (GLsizei)fw->vertex_count);
    glBindVertexArray(0);
    glLineWidth(1.0f);
}

void formwork_print_report(const Formwork* fw) {
    if (!fw) return;
    printf("[FORMWORK] Panels: %u | Board Feet: %.2f | Est Cost: $%.2f\n", 
           fw->count, fw->total_bf, fw->total_cost);
}
