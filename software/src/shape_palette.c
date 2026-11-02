#include "shape_palette.h"
#include "gl_functions.h"
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void shape_set_bevel(Shape* s, float bevel_size, int segments) {
    if (!s) return;
    s->bevel_size = bevel_size;
    s->bevel_segments = segments;
    /* Regenerate mesh with bevel */
    if (s->mesh) { mesh_destroy(s->mesh); s->mesh = NULL; }
    s->mesh = shape_generate_mesh(s->type, s->width, s->depth, s->segments);
}

Mesh* shape_generate_mesh(ShapeType type, float w, float d, int segments) {
    if (!segments) segments = 16;
    Mesh* m = (Mesh*)calloc(1, sizeof(Mesh));
    if (!m) return NULL;
    
    float bevel = 0.0f; /* Assume default 0. Shape struct would hold this */
    int bevel_segs = 0;

    /* Simplified generation: sharp edges by default. 
       Full bevel implementation requires significant vertex re-layout.
       This placeholder shows the hook for bevel generation. */
    
    if (type == SHAPE_SQUARE || type == SHAPE_RECTANGLE) {
        m->vertex_count = 4;
        m->index_count = 6;
        m->positions = (float*)malloc(4 * 3 * sizeof(float));
        m->indices = (uint32_t*)malloc(6 * sizeof(uint32_t));
        
        float hw = w * 0.5f, hd = d * 0.5f;
        m->positions[0] = -hw; m->positions[1] = 0.0f; m->positions[2] = -hd;
        m->positions[3] = hw; m->positions[4] = 0.0f; m->positions[5] = -hd;
        m->positions[6] = hw; m->positions[7] = 0.0f; m->positions[8] = hd;
        m->positions[9] = -hw; m->positions[10] = 0.0f; m->positions[11] = hd;
        
        m->indices[0]=0; m->indices[1]=1; m->indices[2]=2;
        m->indices[3]=0; m->indices[4]=2; m->indices[5]=3;
        
    } else if (type == SHAPE_CIRCLE || type == SHAPE_POLYGON) {
        m->vertex_count = segments + 1;
        m->index_count = segments * 3;
        m->positions = (float*)malloc(m->vertex_count * 3 * sizeof(float));
        m->indices = (uint32_t*)malloc(m->index_count * sizeof(uint32_t));
        
        m->positions[0] = 0.0f; m->positions[1] = 0.0f; m->positions[2] = 0.0f;
        
        float r = fminf(w, d) * 0.5f;
        for (int i = 0; i < segments; i++) {
            float a = 2.0f * M_PI * i / segments;
            m->positions[(i+1)*3] = cosf(a) * r;
            m->positions[(i+1)*3+1] = 0.0f;
            m->positions[(i+1)*3+2] = sinf(a) * r;
        }
        
        for (int i = 0; i < segments; i++) {
            m->indices[i*3] = 0;
            m->indices[i*3+1] = (i+1) % segments + 1;
            m->indices[i*3+2] = i + 1;
        }
        
    } else if (type == SHAPE_LINE) {
        m->vertex_count = 2;
        m->index_count = 2;
        m->positions = (float*)malloc(2 * 3 * sizeof(float));
        m->indices = (uint32_t*)malloc(2 * sizeof(uint32_t));
        
        m->positions[0] = -w*0.5f; m->positions[1] = 0.0f; m->positions[2] = 0.0f;
        m->positions[3] = w*0.5f; m->positions[4] = 0.0f; m->positions[5] = 0.0f;
        m->indices[0] = 0; m->indices[1] = 1;
    }
    
    return m;
}

Shape* shape_create(ShapeType type, Vec3 position, float w, float d, int segments) {
    Shape* s = (Shape*)calloc(1, sizeof(Shape));
    if (!s) return NULL;
    s->type = type;
    s->position = position;
    s->width = w; s->depth = d;
    s->segments = segments;
    s->mesh = shape_generate_mesh(type, w, d, segments);
    return s;
}

void shape_destroy(Shape* s) {
    if (!s) return;
    if (s->mesh) mesh_destroy(s->mesh);
    free(s);
}

void shape_set_position(Shape* s, Vec3 pos) {
    if (s) s->position = pos;
}

void shape_render(Shape* s, Mat4 view, Mat4 proj) {
    if (!s || !s->mesh) return;
    
    Mat4 model = mat4_translate(s->position);
    glUseProgram(shader_program);
    
    GLint loc_model = glGetUniformLocation(shader_program, "uModel");
    if (loc_model >= 0) {
        float f[16]; for(int i=0;i<16;i++) f[i]=(float)model.m[i];
        glUniformMatrix4fv(loc_model, 1, GL_FALSE, f);
    }
    GLint loc_view = glGetUniformLocation(shader_program, "uView");
    if(loc_view>=0){ float f[16]; for(int i=0;i<16;i++) f[i]=(float)view.m[i]; glUniformMatrix4fv(loc_view,1,GL_FALSE,f); }
    GLint loc_proj = glGetUniformLocation(shader_program, "uProjection");
    if(loc_proj>=0){ float f[16]; for(int i=0;i<16;i++) f[i]=(float)proj.m[i]; glUniformMatrix4fv(loc_proj,1,GL_FALSE,f); }
    
    mesh_render(s->mesh);
}
