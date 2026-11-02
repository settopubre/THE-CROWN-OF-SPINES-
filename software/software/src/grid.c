#include "grid.h"
#include "gl_functions.h"
#include <stdlib.h>
#include <math.h>

Grid* grid_create(float spacing, float size) {
    Grid* g = (Grid*)calloc(1, sizeof(Grid));
    if (!g) return NULL;
    g->plane = GRID_XZ;
    g->spacing = spacing > 0.0f ? spacing : 1.0f;
    g->size = size > 0.0f ? size : 100.0f;
    g->visible = 1;
    g->snap_enabled = 1;
    g->origin = vec3_create(0.0f, 0.0f, 0.0f);
    g->uploaded = 0;
    return g;
}

void grid_destroy(Grid* g) {
    if (!g) return;
    if (g->uploaded) {
        glDeleteVertexArrays(1, &g->vao);
        glDeleteBuffers(1, &g->vbo);
    }
    free(g);
}

static void _build_grid(Grid* g) {
    if (!g) return;
    
    int lines = (int)(g->size / g->spacing) * 2 + 1;
    uint32_t vert_count = lines * 4; /* 2 endpoints per line * 2 directions */
    float* data = (float*)malloc(vert_count * 6 * sizeof(float));
    if (!data) return;
    
    uint32_t idx = 0;
    float half = g->size * 0.5f;
    
    /* Lines along axis A */
    for (int i = 0; i <= (int)(g->size / g->spacing); i++) {
        float pos = -half + i * g->spacing;
        float color_val = (i % 5 == 0) ? 0.4f : 0.2f;
        
        if (g->plane == GRID_XZ) {
            data[idx*6] = -half; data[idx*6+1] = 0.0f; data[idx*6+2] = pos;
            data[idx*6+3] = color_val; data[idx*6+4] = color_val; data[idx*6+5] = color_val; idx++;
            data[idx*6] = half; data[idx*6+1] = 0.0f; data[idx*6+2] = pos;
            data[idx*6+3] = color_val; data[idx*6+4] = color_val; data[idx*6+5] = color_val; idx++;
            
            data[idx*6] = pos; data[idx*6+1] = 0.0f; data[idx*6+2] = -half;
            data[idx*6+3] = color_val; data[idx*6+4] = color_val; data[idx*6+5] = color_val; idx++;
            data[idx*6] = pos; data[idx*6+1] = 0.0f; data[idx*6+2] = half;
            data[idx*6+3] = color_val; data[idx*6+4] = color_val; data[idx*6+5] = color_val; idx++;
        }
        /* Add other planes similarly... */
    }
    
    /* Origin crosshair */
    data[idx*6] = -0.5f; data[idx*6+1] = 0.0f; data[idx*6+2] = 0.0f;
    data[idx*6+3] = 1.0f; data[idx*6+4] = 0.0f; data[idx*6+5] = 0.0f; idx++;
    data[idx*6] = 0.5f; data[idx*6+1] = 0.0f; data[idx*6+2] = 0.0f;
    data[idx*6+3] = 1.0f; data[idx*6+4] = 0.0f; data[idx*6+5] = 0.0f; idx++;
    
    data[idx*6] = 0.0f; data[idx*6+1] = 0.0f; data[idx*6+2] = -0.5f;
    data[idx*6+3] = 0.0f; data[idx*6+4] = 0.0f; data[idx*6+5] = 1.0f; idx++;
    data[idx*6] = 0.0f; data[idx*6+1] = 0.0f; data[idx*6+2] = 0.5f;
    data[idx*6+3] = 0.0f; data[idx*6+4] = 0.0f; data[idx*6+5] = 1.0f; idx++;
    
    glGenVertexArrays(1, &g->vao); glBindVertexArray(g->vao);
    glGenBuffers(1, &g->vbo); glBindBuffer(GL_ARRAY_BUFFER, g->vbo);
    glBufferData(GL_ARRAY_BUFFER, idx * 6 * sizeof(float), data, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    free(data);
    g->uploaded = 1;
}

void grid_render(Grid* g, Mat4 view, Mat4 proj) {
    if (!g || !g->visible) return;
    if (!g->uploaded) _build_grid(g);
    
    glUseProgram(shader_program); glLineWidth(1.0f);
    GLint loc_view = glGetUniformLocation(shader_program, "uView");
    if(loc_view>=0){ float f[16]; for(int i=0;i<16;i++) f[i]=(float)view.m[i]; glUniformMatrix4fv(loc_view,1,GL_FALSE,f); }
    GLint loc_proj = glGetUniformLocation(shader_program, "uProjection");
    if(loc_proj>=0){ float f[16]; for(int i=0;i<16;i++) f[i]=(float)proj.m[i]; glUniformMatrix4fv(loc_proj,1,GL_FALSE,f); }
    GLint loc_model = glGetUniformLocation(shader_program, "uModel");
    if(loc_model>=0){ Mat4 m = mat4_translate(g->origin); float f[16]; for(int i=0;i<16;i++) f[i]=(float)m.m[i]; glUniformMatrix4fv(loc_model,1,GL_FALSE,f); }
    
    glBindVertexArray(g->vao);
    glDrawArrays(GL_LINES, 0, (int)((g->size/g->spacing)*4+2));
    glBindVertexArray(0);
}

Vec3 grid_snap(const Grid* g, Vec3 point) {
    if (!g || !g->snap_enabled) return point;
    Vec3 snapped = point;
    float s = g->spacing;
    if (g->plane == GRID_XZ) {
        snapped.x = roundf((point.x - g->origin.x) / s) * s + g->origin.x;
        snapped.z = roundf((point.z - g->origin.z) / s) * s + g->origin.z;
    } else if (g->plane == GRID_XY) {
        snapped.x = roundf((point.x - g->origin.x) / s) * s + g->origin.x;
        snapped.y = roundf((point.y - g->origin.y) / s) * s + g->origin.y;
    } else {
        snapped.y = roundf((point.y - g->origin.y) / s) * s + g->origin.y;
        snapped.z = roundf((point.z - g->origin.z) / s) * s + g->origin.z;
    }
    return snapped;
}

void grid_cycle_plane(Grid* g) {
    if (g) g->plane = (GridPlane)((g->plane + 1) % 3);
    g->uploaded = 0;
}

void grid_set_spacing(Grid* g, float spacing) {
    if (g && spacing > 0.0f) { g->spacing = spacing; g->uploaded = 0; }
}

void grid_toggle_visible(Grid* g) { if (g) g->visible = !g->visible; }
void grid_toggle_snap(Grid* g) { if (g) g->snap_enabled = !g->snap_enabled; }
