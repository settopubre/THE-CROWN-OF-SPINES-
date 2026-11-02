#include "vector3d.h"
#include "mesh.h"
#include "instance_renderer.h"
#include "gl_functions.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct {
    float m[16];
    float color[3];
} InstanceData;

/* FIX: Explicit typedef so we can use 'InstanceRenderer' without the 'struct' keyword */
typedef struct InstanceRenderer InstanceRenderer;

struct InstanceRenderer {
    Mesh* base_mesh;
    int max_instances;
    
    /* Instance data storage */
    InstanceData* instances;
    int instance_count;
    int instance_capacity;

    GLuint vao;
    GLuint instance_vbo;
};

InstanceRenderer* instance_renderer_create(Mesh* mesh, int max_instances) {
    if (!mesh || max_instances <= 0) return NULL;
    
    InstanceRenderer* ir = (InstanceRenderer*)calloc(1, sizeof(InstanceRenderer));
    if (!ir) return NULL;
    
    ir->base_mesh = mesh;
    ir->max_instances = max_instances;
    ir->instance_capacity = max_instances;
    ir->instances = (InstanceData*)malloc(ir->instance_capacity * sizeof(InstanceData));
    ir->instance_count = 0;
    
    glGenVertexArrays(1, &ir->vao);
    glGenBuffers(1, &ir->instance_vbo);
    
    return ir;
}

void instance_renderer_add_instance(InstanceRenderer* ir, Mat4 model_matrix, Vec3 color) {
    if (!ir || ir->instance_count >= ir->max_instances) return;
    
    if (ir->instance_count >= ir->instance_capacity) {
        int new_cap = ir->instance_capacity * 2;
        InstanceData* new_instances = (InstanceData*)realloc(ir->instances, new_cap * sizeof(InstanceData));
        if (!new_instances) return;
        ir->instances = new_instances;
        ir->instance_capacity = new_cap;
    }
    
    for (int i = 0; i < 16; i++) {
        ir->instances[ir->instance_count].m[i] = (float)model_matrix.m[i];
    }
    ir->instances[ir->instance_count].color[0] = (float)color.x;
    ir->instances[ir->instance_count].color[1] = (float)color.y;
    ir->instances[ir->instance_count].color[2] = (float)color.z;
    
    ir->instance_count++;
}

void instance_renderer_clear(InstanceRenderer* ir) {
    if (ir) ir->instance_count = 0;
}

void instance_renderer_render(InstanceRenderer* ir, Mat4 view, Mat4 proj) {
    if (!ir || ir->instance_count == 0) return;
    if (!ir->base_mesh || !ir->base_mesh->uploaded) return;
    if (ir->base_mesh->index_count == 0) return;
    
    /*
     * P0 CRITICAL FIX: VAO state corruption during instanced rendering.
     *
     * BUG: The original code called glBindVertexArray(ir->base_mesh->vao)
     *      which overwrites ALL attribute bindings with those stored in
     *      the base mesh's VAO. Since the base mesh VAO only knows about
     *      attributes 0 (position) and 1 (normal), all instanced attributes
     *      at locations 2-6 were silently lost, causing:
     *      - Incorrect model matrices (all instances render at origin)
     *      - Missing/broken per-instance colors
     *      - Undefined behavior in Core OpenGL 3.3+ (no default attribute values)
     *
     * FIX: After binding the base mesh VAO (which restores attributes 0-1
     *      and the element buffer), we must immediately rebind the instance
     *      VBO and reconfigure attributes 2-6. The VAO stores these new
     *      bindings because it is still bound. We use the correct stride
     *      of sizeof(InstanceData) = 76 bytes (64 for mat4 + 12 for vec3).
     *      glVertexAttribDivisor(..., 1) tells OpenGL to advance each
     *      attribute once per instance, not per vertex.
     *
     * CORRECT ORDER OF OPERATIONS:
     * 1. Bind base mesh VAO     → restores attributes 0,1 + element buffer
     * 2. Bind instance VBO      → source of per-instance data
     * 3. Set attrib pointers    → define layout within instance VBO
     * 4. Enable attrib arrays   → activate locations 2-6
     * 5. Set divisor to 1       → per-instance advancement
     * 6. Set view/proj uniforms → transform matrices
     * 7. Draw instanced         → glDrawElementsInstanced
     * 8. Unbind VAO             → cleanup
     */
    
    glBindVertexArray(ir->base_mesh->vao);
    
    const GLsizei instance_stride = sizeof(InstanceData);
    
    glBindBuffer(GL_ARRAY_BUFFER, ir->instance_vbo);
    glBufferData(GL_ARRAY_BUFFER, 
                 ir->instance_count * instance_stride, 
                 ir->instances, 
                 GL_DYNAMIC_DRAW);
    
    /*
     * Model matrix columns at locations 2, 3, 4, 5.
     * Each column is 4 floats (16 bytes) wide.
     * Offsets: column 0 at byte 0, column 1 at byte 16, 
     *          column 2 at byte 32, column 3 at byte 48.
     */
    for (int i = 0; i < 4; i++) {
        glVertexAttribPointer(2 + i, 4, GL_FLOAT, GL_FALSE, instance_stride, 
                              (const void*)(uintptr_t)(i * 4 * sizeof(float)));
        glEnableVertexAttribArray(2 + i);
        glVertexAttribDivisor(2 + i, 1);
    }
    
    /*
     * Per-instance color at location 6.
     * Offset: 64 bytes (4 columns * 4 floats * 4 bytes per float).
     */
    glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, instance_stride, 
                          (const void*)(uintptr_t)(64));
    glEnableVertexAttribArray(6);
    glVertexAttribDivisor(6, 1);
    
    GLint loc_view = glGetUniformLocation(shader_program, "uView");
    if (loc_view >= 0) {
        float f[16];
        for (int k = 0; k < 16; k++) f[k] = (float)view.m[k];
        glUniformMatrix4fv(loc_view, 1, GL_FALSE, f);
    }
    
    GLint loc_proj = glGetUniformLocation(shader_program, "uProjection");
    if (loc_proj >= 0) {
        float f[16];
        for (int k = 0; k < 16; k++) f[k] = (float)proj.m[k];
        glUniformMatrix4fv(loc_proj, 1, GL_FALSE, f);
    }
    
    glDrawElementsInstanced(GL_TRIANGLES, 
                            ir->base_mesh->index_count, 
                            GL_UNSIGNED_INT, 
                            (const void*)0, 
                            ir->instance_count);
    
    glBindVertexArray(0);
}

void instance_renderer_destroy(InstanceRenderer* ir) {
    if (ir) {
        if (ir->vao) glDeleteVertexArrays(1, &ir->vao);
        if (ir->instance_vbo) glDeleteBuffers(1, &ir->instance_vbo);
        free(ir->instances);
        free(ir);
    }
}
