#include "rebar_architecture.h"
#include "gl_functions.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

RebarLayout* rebar_layout_generate(const Vec3* perimeter, uint32_t count, 
                                    float slab_thickness, float cover_inches, 
                                    float spacing_inches) {
    if (!perimeter || count < 3) return NULL;
    
    RebarLayout* rl = (RebarLayout*)calloc(1, sizeof(RebarLayout));
    if (!rl) return NULL;
    
    rl->capacity = 2000;
    rl->bars = (Rebar*)calloc(rl->capacity, sizeof(Rebar));
    if (!rl->bars) {
        free(rl);
        return NULL;
    }
    
    rl->spacing = spacing_inches / 12.0f;
    rl->cover = cover_inches / 12.0f;
    rl->count = 0;
    rl->uploaded = 0;
    rl->vao = 0;
    rl->vbo = 0;

    float min_x = 1e30f, max_x = -1e30f, min_z = 1e30f, max_z = -1e30f, y = 0.0f;
    for (uint32_t i = 0; i < count; i++) {
        if (perimeter[i].x < min_x) min_x = perimeter[i].x;
        if (perimeter[i].x > max_x) max_x = perimeter[i].x;
        if (perimeter[i].z < min_z) min_z = perimeter[i].z;
        if (perimeter[i].z > max_z) max_z = perimeter[i].z;
        y += perimeter[i].y;
    }
    y /= (float)count;
    min_x += rl->cover; max_x -= rl->cover;
    min_z += rl->cover; max_z -= rl->cover;

    /* Longitudinal bars (along X) */
    for (float z = min_z; z <= max_z; z += rl->spacing) {
        if (rl->count >= rl->capacity) break;
        Rebar* r = &rl->bars[rl->count++];
        r->start = vec3_create(min_x, y, z);
        r->end   = vec3_create(max_x, y, z);
        r->diameter = 0.5f;
        strcpy(r->grade, "#4");
    }
    
    /* Transverse bars (along Z) */
    for (float x = min_x; x <= max_x; x += rl->spacing) {
        if (rl->count >= rl->capacity) break;
        Rebar* r = &rl->bars[rl->count++];
        r->start = vec3_create(x, y, min_z);
        r->end   = vec3_create(x, y, max_z);
        r->diameter = 0.5f;
        strcpy(r->grade, "#4");
    }
    
    return rl;
}

void rebar_layout_destroy(RebarLayout* rl) {
    if (!rl) return;
    
    /*
     * P0 CRITICAL FIX: Clean up GPU resources on destroy.
     *
     * WAS: No VAO/VBO cleanup — GPU resources leaked.
     *
     * FIX: Delete VAO and VBO if they were uploaded.
     */
    if (rl->uploaded) {
        glDeleteVertexArrays(1, &rl->vao);
        glDeleteBuffers(1, &rl->vbo);
    }
    
    free(rl->bars);
    free(rl);
}

void rebar_layout_render(RebarLayout* rl, Mat4 view, Mat4 proj) {
    if (!rl || rl->count == 0) return;
    
    /*
     * P0 CRITICAL FIX: Proper VAO management for Core OpenGL 3.3+.
     *
     * WAS: glBindBuffer + glVertexAttribPointer + glDrawArrays
     *      WITHOUT a bound VAO. In Core OpenGL 3.3+, all vertex
     *      attribute configuration MUST occur within a VAO.
     *      Calling glVertexAttribPointer without a VAO is
     *      undefined behavior and may crash or render nothing.
     *
     * FIX: Generate and bind a VAO before configuring attributes.
     *      Store vao/vbo in the RebarLayout struct. Upload once
     *      with GL_STATIC_DRAW since rebar geometry doesn't change.
     *      On subsequent frames, just bind the VAO and draw.
     */
    
    if (!rl->uploaded) {
        uint32_t verts = rl->count * 2;
        float* data = (float*)malloc(verts * 6 * sizeof(float));
        if (!data) return;
        
        for (uint32_t i = 0; i < rl->count; i++) {
            float* col;
            if (strcmp(rl->bars[i].grade, "#3") == 0) {
                static float c3[] = {0.0f, 0.8f, 0.0f};
                col = c3;
            } else if (strcmp(rl->bars[i].grade, "#5") == 0) {
                static float c5[] = {1.0f, 0.0f, 0.0f};
                col = c5;
            } else {
                static float c4[] = {0.0f, 0.0f, 1.0f};
                col = c4;
            }
            
            uint32_t base = i * 12;
            data[base + 0] = rl->bars[i].start.x;
            data[base + 1] = rl->bars[i].start.y;
            data[base + 2] = rl->bars[i].start.z;
            data[base + 3] = col[0];
            data[base + 4] = col[1];
            data[base + 5] = col[2];
            data[base + 6] = rl->bars[i].end.x;
            data[base + 7] = rl->bars[i].end.y;
            data[base + 8] = rl->bars[i].end.z;
            data[base + 9] = col[0];
            data[base + 10] = col[1];
            data[base + 11] = col[2];
        }
        
        glGenVertexArrays(1, &rl->vao);
        glBindVertexArray(rl->vao);
        
        glGenBuffers(1, &rl->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, rl->vbo);
        glBufferData(GL_ARRAY_BUFFER, verts * 6 * sizeof(float), data, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 
                              (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glBindVertexArray(0);
        free(data);
        rl->uploaded = 1;
    }
    
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
    
    glLineWidth(2.0f);
    glBindVertexArray(rl->vao);
    glDrawArrays(GL_LINES, 0, (GLsizei)(rl->count * 2));
    glBindVertexArray(0);
    glLineWidth(1.0f);
}

void rebar_layout_print_report(const RebarLayout* rl) {
    if (!rl) return;
    float total_ft = 0.0f;
    for (uint32_t i = 0; i < rl->count; i++) {
        total_ft += vec3_length(vec3_sub(rl->bars[i].end, rl->bars[i].start));
    }
    printf("[REBAR] Bars: %u | Total Length: %.1f ft\n", rl->count, total_ft);
}
