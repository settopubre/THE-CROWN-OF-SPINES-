#include "render_server.h"
#include "gl_functions.h"
#include "mesh.h"
#include <stdio.h>
#include <stdlib.h>

static RenderServer* g_rs = NULL;
static GLuint g_line_vbo = 0;
static GLuint g_line_vao = 0;
static float g_clear_color[4] = {0.2f, 0.3f, 0.8f, 1.0f};

/* Uniform Cache */
typedef struct {
    GLint uModel;
    GLint uView;
    GLint uProjection;
    GLint uColor;
    GLint uAmbient;
    GLint uSpecular;
    GLint uShininess;
    GLint uTexture;
} UniformLocations;

static UniformLocations g_uniforms = {0};

RenderServer* render_server_get(void) {
    if (!g_rs) {
        g_rs = (RenderServer*)calloc(1, sizeof(RenderServer));
        glGenVertexArrays(1, &g_line_vao);
        glGenBuffers(1, &g_line_vbo);
    }
    return g_rs;
}

void render_server_init(void) {
    RenderServer* rs = render_server_get();
    rs->draw_calls = 0;
    rs->triangle_count = 0;
    rs->active_shader = shader_program;
    glClearColor(g_clear_color[0], g_clear_color[1], g_clear_color[2], g_clear_color[3]);
    
    /* Cache Uniform Locations */
    glUseProgram(shader_program);
    g_uniforms.uModel = glGetUniformLocation(shader_program, "uModel");
    g_uniforms.uView = glGetUniformLocation(shader_program, "uView");
    g_uniforms.uProjection = glGetUniformLocation(shader_program, "uProjection");
    g_uniforms.uColor = glGetUniformLocation(shader_program, "uColor");
    g_uniforms.uAmbient = glGetUniformLocation(shader_program, "uAmbient");
    g_uniforms.uSpecular = glGetUniformLocation(shader_program, "uSpecular");
    g_uniforms.uShininess = glGetUniformLocation(shader_program, "uShininess");
    g_uniforms.uTexture = glGetUniformLocation(shader_program, "uTexture");
}

void render_server_shutdown(void) {
    if (g_line_vbo) glDeleteBuffers(1, &g_line_vbo);
    if (g_line_vao) glDeleteVertexArrays(1, &g_line_vao);
    free(g_rs);
    g_rs = NULL;
}

void render_server_begin_frame(Mat4 view, Mat4 proj) {
    RenderServer* rs = render_server_get();
    rs->current_view = view;
    rs->current_proj = proj;
    rs->draw_calls = 0;
    rs->triangle_count = 0;
    
    /* Clear color & depth buffers */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    
    /* Bind default shader */
    glUseProgram(rs->active_shader);
}

void render_server_end_frame(void) {
    /* Flush handled by engine swap */
}

void render_server_draw_mesh(Mesh* mesh, Mat4 model) {
    if (!mesh || mesh->vertex_count == 0) return;
    RenderServer* rs = render_server_get();
    
    if (g_uniforms.uModel >= 0) {
        float f[16];
        for (int i = 0; i < 16; i++) f[i] = (float)model.m[i];
        glUniformMatrix4fv(g_uniforms.uModel, 1, GL_FALSE, f);
    }
    
    if (g_uniforms.uView >= 0) {
        float f[16];
        for (int i = 0; i < 16; i++) f[i] = (float)rs->current_view.m[i];
        glUniformMatrix4fv(g_uniforms.uView, 1, GL_FALSE, f);
    }
    
    if (g_uniforms.uProjection >= 0) {
        float f[16];
        for (int i = 0; i < 16; i++) f[i] = (float)rs->current_proj.m[i];
        glUniformMatrix4fv(g_uniforms.uProjection, 1, GL_FALSE, f);
    }
    
    mesh_render(mesh);
    
    rs->draw_calls++;
    rs->triangle_count += mesh->index_count / 3;
}

void render_server_draw_lines(float* vertices, uint32_t count, Vec3 color) {
    if (!vertices || count == 0) return;
    glUseProgram(shader_program);
    
    RenderServer* rs = render_server_get();
    
    if (g_uniforms.uView >= 0) {
        float f[16]; 
        for(int i=0;i<16;i++) f[i]=(float)rs->current_view.m[i];
        glUniformMatrix4fv(g_uniforms.uView, 1, GL_FALSE, f);
    }
    if (g_uniforms.uProjection >= 0) {
        float f[16]; 
        for(int i=0;i<16;i++) f[i]=(float)rs->current_proj.m[i];
        glUniformMatrix4fv(g_uniforms.uProjection, 1, GL_FALSE, f);
    }
    if (g_uniforms.uColor >= 0) {
        glUniform3f(g_uniforms.uColor, color.x, color.y, color.z);
    }

    glBindVertexArray(g_line_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_line_vbo);
    glBufferData(GL_ARRAY_BUFFER, count * 6 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, count * 6 * sizeof(float), vertices);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glDrawArrays(GL_LINES, 0, count);
    glBindVertexArray(0);
    
    rs->draw_calls++;
}

void render_server_set_clear_color(float r, float g, float b) {
    g_clear_color[0] = r;
    g_clear_color[1] = g;
    g_clear_color[2] = b;
    glClearColor(r, g, b, 1.0f);
}

uint32_t render_server_get_draw_calls(void) { return render_server_get()->draw_calls; }
uint32_t render_server_get_triangle_count(void) { return render_server_get()->triangle_count; }
