#include "splat_diff.h"
#include "kdtree.h"
#include "gl_functions.h"
#include <stdlib.h>
#include <math.h>

SplatDiff* splat_diff_create(void) {
    SplatDiff* sd = (SplatDiff*)calloc(1, sizeof(SplatDiff));
    sd->diff_values = NULL;
    sd->count = 0;
    sd->uploaded = 0;
    return sd;
}

void splat_diff_destroy(SplatDiff* sd) {
    if (!sd) return;
    if (sd->uploaded) { glDeleteVertexArrays(1, &sd->vao); glDeleteBuffers(1, &sd->vbo); }
    free(sd->diff_values);
    free(sd);
}

void splat_diff_set_before(SplatDiff* sd, Splat* s) { sd->before = s; }
void splat_diff_set_after(SplatDiff* sd, Splat* s) { sd->after = s; }

void splat_diff_compute(SplatDiff* sd, float tolerance) {
    if (!sd || !sd->after) return;
    free(sd->diff_values);
    sd->count = sd->after->count;
    sd->diff_values = (int*)calloc(sd->count, sizeof(int));

    if (!sd->before || sd->before->count == 0) {
        for (uint32_t i = 0; i < sd->count; i++) sd->diff_values[i] = 1;
        return;
    }

    KDTree* tree = kdtree_create(sd->before->count);
    if (!tree) return;
    
    for (uint32_t i = 0; i < sd->before->count; i++) {
        Vec3 p = {0, sd->before->positions[i*3], 
                      sd->before->positions[i*3+1], 
                      sd->before->positions[i*3+2]};
        kdtree_insert(tree, p, i);
    }

    float tol_sq = tolerance * tolerance;
    for (uint32_t i = 0; i < sd->count; i++) {
        Vec3 p = {0, sd->after->positions[i*3], 
                      sd->after->positions[i*3+1], 
                      sd->after->positions[i*3+2]};
        uint32_t nn = kdtree_nearest(tree, p);
        
        if (nn != UINT32_MAX) {
            uint32_t orig_idx = tree->nodes[nn].data_id;
            
            double dx = p.x - sd->before->positions[orig_idx * 3];
            double dy = p.y - sd->before->positions[orig_idx * 3 + 1];
            double dz = p.z - sd->before->positions[orig_idx * 3 + 2];
            
            if (dx*dx + dy*dy + dz*dz > tol_sq) {
                sd->diff_values[i] = 1;
            } else {
                sd->diff_values[i] = 0;
            }
        } else {
            sd->diff_values[i] = 1;
        }
    }
    kdtree_destroy(tree);
}

void splat_diff_render(SplatDiff* sd, Mat4 view, Mat4 proj) {
    if (!sd || sd->count == 0) return;
    if (!sd->uploaded) {
        float* data = (float*)malloc(sd->count * 6 * sizeof(float));
        for (uint32_t i = 0; i < sd->count; i++) {
            data[i*6]   = sd->after->positions[i*3];
            data[i*6+1] = sd->after->positions[i*3+1];
            data[i*6+2] = sd->after->positions[i*3+2];
            int type = sd->diff_values[i];
            if (type == 0) { data[i*6+3]=0.0f; data[i*6+4]=1.0f; data[i*6+5]=0.0f; }
            else if (type == 1) { data[i*6+3]=1.0f; data[i*6+4]=0.0f; data[i*6+5]=0.0f; }
            else { data[i*6+3]=0.0f; data[i*6+4]=0.0f; data[i*6+5]=1.0f; }
        }
        glGenVertexArrays(1, &sd->vao); glBindVertexArray(sd->vao);
        glGenBuffers(1, &sd->vbo); glBindBuffer(GL_ARRAY_BUFFER, sd->vbo);
        glBufferData(GL_ARRAY_BUFFER, sd->count * 6 * sizeof(float), data, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0); free(data); sd->uploaded = 1;
    }

    glUseProgram(shader_program); glPointSize(4.0f); glEnable(GL_PROGRAM_POINT_SIZE);
    GLint loc_view = glGetUniformLocation(shader_program, "uView");
    if (loc_view >= 0) { float f[16]; for(int i=0;i<16;i++) f[i]=(float)view.m[i]; glUniformMatrix4fv(loc_view,1,GL_FALSE,f); }
    GLint loc_proj = glGetUniformLocation(shader_program, "uProjection");
    if (loc_proj >= 0) { float f[16]; for(int i=0;i<16;i++) f[i]=(float)proj.m[i]; glUniformMatrix4fv(loc_proj,1,GL_FALSE,f); }
    
    glBindVertexArray(sd->vao); glDrawArrays(GL_POINTS, 0, sd->count); glBindVertexArray(0);
    glDisable(GL_PROGRAM_POINT_SIZE);
}
