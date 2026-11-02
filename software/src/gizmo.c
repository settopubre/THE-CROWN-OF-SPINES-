#include "engine_abi.h"
#include "gl_functions.h"
#include "vector3d.h"
#include <GL/gl.h>
#include <stdlib.h>
#include <string.h>

struct Gizmo {
    Vec3 position;
    float axis_length;
    GLuint vao, vbo;
    int uploaded;
};

Gizmo* gizmo_create(void) {
    Gizmo* g = (Gizmo*)calloc(1, sizeof(Gizmo));
    if (!g) return NULL;
    g->position = vec3_create(0.0, 0.0, 0.0);
    g->axis_length = 1.0f;
    g->uploaded = 0;
    return g;
}

void gizmo_destroy(Gizmo* g) {
    if (!g) return;
    if (g->uploaded) {
        glDeleteVertexArrays(1, &g->vao);
        glDeleteBuffers(1, &g->vbo);
    }
    free(g);
}

void gizmo_render(Gizmo* g, Mat4 view, Mat4 projection) {
    if (!g) return;

    if (!g->uploaded) {
        float data[] = {
            0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f,
            0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 1.0f
        };

        glGenVertexArrays(1, &g->vao);
        glBindVertexArray(g->vao);

        glGenBuffers(1, &g->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, g->vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
        g->uploaded = 1;
    }

    glUseProgram(shader_program);

    GLint loc_model = glGetUniformLocation(shader_program, "uModel");
    if (loc_model >= 0) {
        Mat4 model = mat4_translate(g->position);
        float fmat[16];
        for (int i = 0; i < 16; i++) fmat[i] = (float)model.m[i];
        glUniformMatrix4fv(loc_model, 1, GL_FALSE, fmat);
    }

    GLint loc_view = glGetUniformLocation(shader_program, "uView");
    if (loc_view >= 0) {
        float fmat[16];
        for (int i = 0; i < 16; i++) fmat[i] = (float)view.m[i];
        glUniformMatrix4fv(loc_view, 1, GL_FALSE, fmat);
    }

    GLint loc_proj = glGetUniformLocation(shader_program, "uProjection");
    if (loc_proj >= 0) {
        float fmat[16];
        for (int i = 0; i < 16; i++) fmat[i] = (float)projection.m[i];
        glUniformMatrix4fv(loc_proj, 1, GL_FALSE, fmat);
    }

    glBindVertexArray(g->vao);
    glDrawArrays(GL_LINES, 0, 6);
    glBindVertexArray(0);
}

void gizmo_set_position(Gizmo* g, Vec3 pos) {
    if (g) g->position = pos;
}

Vec3 gizmo_get_position(const Gizmo* g) {
    return g ? g->position : vec3_create(0.0, 0.0, 0.0);
}
