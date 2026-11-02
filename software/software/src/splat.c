#include "splat.h"
#include "gl_functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 1024

Splat* splat_load(const char* filename) {
    DEFER_FILE(fp) = fopen(filename, "r");
    if (!fp) return NULL;

    Splat* s = (Splat*)calloc(1, sizeof(Splat));
    if (!s) return NULL;

    char line[MAX_LINE];
    int vertex_count = 0;
    int has_colors = 0;
    int has_gaussian_props = 0;
    
    while (fgets(line, MAX_LINE, fp)) {
        if (strncmp(line, "element vertex", 13) == 0) sscanf(line, "element vertex %d", &vertex_count);
        if (strncmp(line, "property uchar red", 18) == 0) has_colors = 1;
        if (strncmp(line, "property float scale_0", 20) == 0) has_gaussian_props = 1;
        if (strncmp(line, "end_header", 10) == 0) break;
    }

    if (vertex_count == 0) { free(s); return NULL; }

    s->count = (uint32_t)vertex_count;
    s->positions = (float*)malloc(s->count * 3 * sizeof(float));
    s->colors = (float*)malloc(s->count * 3 * sizeof(float));
    if (!s->positions || !s->colors) { free(s->positions); free(s->colors); free(s); return NULL; }

    s->bbox.min_x = s->bbox.min_y = s->bbox.min_z = 1e30f;
    s->bbox.max_x = s->bbox.max_y = s->bbox.max_z = -1e30f;

    for (uint32_t i = 0; i < s->count; i++) {
        if (has_colors) {
            if (fscanf(fp, "%f %f %f %hhu %hhu %hhu", 
                       &s->positions[i*3], &s->positions[i*3+1], &s->positions[i*3+2],
                       (unsigned char*)&s->colors[i*3], (unsigned char*)&s->colors[i*3+1], (unsigned char*)&s->colors[i*3+2]) < 6) break;
            s->colors[i*3] /= 255.0f; s->colors[i*3+1] /= 255.0f; s->colors[i*3+2] /= 255.0f;
        } else {
            if (fscanf(fp, "%f %f %f", &s->positions[i*3], &s->positions[i*3+1], &s->positions[i*3+2]) < 3) break;
            s->colors[i*3] = s->colors[i*3+1] = s->colors[i*3+2] = 1.0f;
        }
        
        if (s->positions[i*3] < s->bbox.min_x) s->bbox.min_x = s->positions[i*3];
        if (s->positions[i*3] > s->bbox.max_x) s->bbox.max_x = s->positions[i*3];
        if (s->positions[i*3+1] < s->bbox.min_y) s->bbox.min_y = s->positions[i*3+1];
        if (s->positions[i*3+1] > s->bbox.max_y) s->bbox.max_y = s->positions[i*3+1];
        if (s->positions[i*3+2] < s->bbox.min_z) s->bbox.min_z = s->positions[i*3+2];
        if (s->positions[i*3+2] > s->bbox.max_z) s->bbox.max_z = s->positions[i*3+2];
    }
    
    s->has_covariance = has_gaussian_props;
    s->uploaded = 0;
    return s;
}

void splat_destroy(Splat* s) {
    if (!s) return;
    if (s->uploaded) {
        glDeleteVertexArrays(1, &s->vao);
        glDeleteBuffers(1, &s->vbo);
    }
    free(s->positions);
    free(s->colors);
    free(s);
}

void splat_render(Splat* s, Mat4 view, Mat4 proj, float point_size) {
    if (!s || s->count == 0) return;
    if (!s->uploaded) {
        glGenVertexArrays(1, &s->vao);
        glBindVertexArray(s->vao);

        glGenBuffers(1, &s->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, s->vbo);
        glBufferData(GL_ARRAY_BUFFER, s->count * 6 * sizeof(float), NULL, GL_STATIC_DRAW);
        
        float* data = (float*)malloc(s->count * 6 * sizeof(float));
        for (uint32_t i = 0; i < s->count; i++) {
            data[i*6]   = s->positions[i*3];
            data[i*6+1] = s->positions[i*3+1];
            data[i*6+2] = s->positions[i*3+2];
            data[i*6+3] = s->colors[i*3];
            data[i*6+4] = s->colors[i*3+1];
            data[i*6+5] = s->colors[i*3+2];
        }
        glBufferData(GL_ARRAY_BUFFER, s->count * 6 * sizeof(float), data, GL_STATIC_DRAW);
        free(data);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
        s->uploaded = 1;
    }

    glUseProgram(shader_program);
    
    GLint loc_view = glGetUniformLocation(shader_program, "uView");
    if (loc_view >= 0) {
        float fmat[16];
        for (int i = 0; i < 16; i++) fmat[i] = (float)view.m[i];
        glUniformMatrix4fv(loc_view, 1, GL_FALSE, fmat);
    }
    GLint loc_proj = glGetUniformLocation(shader_program, "uProjection");
    if (loc_proj >= 0) {
        float fmat[16];
        for (int i = 0; i < 16; i++) fmat[i] = (float)proj.m[i];
        glUniformMatrix4fv(loc_proj, 1, GL_FALSE, fmat);
    }

    if (s->has_covariance) {
        glDisable(GL_PROGRAM_POINT_SIZE);
        glBindVertexArray(s->vao);
        glDrawArrays(GL_POINTS, 0, s->count);
    } else {
        glPointSize(point_size);
        glEnable(GL_PROGRAM_POINT_SIZE);
        glBindVertexArray(s->vao);
        glDrawArrays(GL_POINTS, 0, s->count);
        glDisable(GL_PROGRAM_POINT_SIZE);
    }
    glBindVertexArray(0);
}

uint32_t splat_get_count(const Splat* s) {
    return s ? s->count : 0;
}
