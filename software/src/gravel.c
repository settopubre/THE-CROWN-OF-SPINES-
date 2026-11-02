#include "gravel.h"
#include "gl_functions.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

GravelLayer* gravel_generate(const Vec3* perimeter, uint32_t count, float thickness) {
    if (!perimeter || count < 3) return NULL;
    GravelLayer* gl = (GravelLayer*)calloc(1, sizeof(GravelLayer));
    if (!gl) return NULL;
    gl->perimeter = (Vec3*)malloc(count * sizeof(Vec3));
    if (!gl->perimeter) { free(gl); return NULL; }
    memcpy(gl->perimeter, perimeter, count * sizeof(Vec3));
    gl->perimeter_count = count;
    gl->thickness = thickness;

    /* Shoelace formula */
    double area = 0.0;
    for (uint32_t i = 0; i < count; i++) {
        double x1 = perimeter[i].x, y1 = perimeter[i].z; /* Assuming XZ plane */
        double x2 = perimeter[(i+1)%count].x, y2 = perimeter[(i+1)%count].z;
        area += (x1*y2 - x2*y1);
    }
    gl->area_sqft = (float)(fabs(area) / 2.0);
    gl->volume_yards = (gl->area_sqft * (thickness/12.0f)) / 27.0f;
    return gl;
}

void gravel_destroy(GravelLayer* gl) {
    if (!gl) return;
    if (gl->uploaded) { glDeleteVertexArrays(1, &gl->vao); glDeleteBuffers(1, &gl->vbo); }
    free(gl->perimeter); free(gl);
}

void gravel_render(GravelLayer* gl, Mat4 view, Mat4 proj) {
    if (!gl || gl->perimeter_count == 0) return;
    if (!gl->uploaded) {
        uint32_t count = gl->perimeter_count + 1;
        float* data = (float*)malloc(count * 6 * sizeof(float));
        for(uint32_t i=0; i<gl->perimeter_count; i++){
            data[i*6]=gl->perimeter[i].x; data[i*6+1]=gl->perimeter[i].y; data[i*6+2]=gl->perimeter[i].z;
            data[i*6+3]=0.4f; data[i*6+4]=0.4f; data[i*6+5]=0.4f;
        }
        data[(count-1)*6] = data[0]; data[(count-1)*6+1] = data[1]; data[(count-1)*6+2] = data[2];
        data[(count-1)*6+3]=0.4f; data[(count-1)*6+4]=0.4f; data[(count-1)*6+5]=0.4f;

        glGenVertexArrays(1, &gl->vao); glBindVertexArray(gl->vao);
        glGenBuffers(1, &gl->vbo); glBindBuffer(GL_ARRAY_BUFFER, gl->vbo);
        glBufferData(GL_ARRAY_BUFFER, count*6*sizeof(float), data, GL_STATIC_DRAW);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0); glEnableVertexAttribArray(0);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(3*sizeof(float))); glEnableVertexAttribArray(1);
        glBindVertexArray(0); free(data); gl->uploaded = 1;
    }

    glUseProgram(shader_program); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLint loc_view = glGetUniformLocation(shader_program, "uView");
    if(loc_view>=0){ float f[16]; for(int i=0;i<16;i++) f[i]=(float)view.m[i]; glUniformMatrix4fv(loc_view,1,GL_FALSE,f); }
    GLint loc_proj = glGetUniformLocation(shader_program, "uProjection");
    if(loc_proj>=0){ float f[16]; for(int i=0;i<16;i++) f[i]=(float)proj.m[i]; glUniformMatrix4fv(loc_proj,1,GL_FALSE,f); }
    glBindVertexArray(gl->vao); glDrawArrays(GL_LINE_LOOP, 0, gl->perimeter_count); glBindVertexArray(0);
    glDisable(GL_BLEND);
}

void gravel_print_report(const GravelLayer* gl) {
    if(!gl) return;
    printf("[GRAVEL] Area: %.2f sqft | Thickness: %.1f\" | Volume: %.2f yards\n", gl->area_sqft, gl->thickness, gl->volume_yards);
}
