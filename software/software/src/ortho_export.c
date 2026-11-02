#include "ortho_export.h"
#include "gl_functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

OrthoExport* ortho_export_create(Vec3 min_bounds, Vec3 max_bounds, float resolution) {
    OrthoExport* oe = (OrthoExport*)calloc(1, sizeof(OrthoExport));
    if (!oe) return NULL;
    oe->min_bounds = min_bounds;
    oe->max_bounds = max_bounds;
    oe->resolution = resolution;
    
    float w = (max_bounds.x - min_bounds.x) / resolution;
    float h = (max_bounds.z - min_bounds.z) / resolution;
    oe->output_width = (uint32_t)ceilf(w);
    oe->output_height = (uint32_t)ceilf(h);
    
    oe->pixel_buffer = (uint8_t*)malloc(oe->output_width * oe->output_height * 3);
    return oe;
}

void ortho_export_destroy(OrthoExport* oe) {
    if (oe) {
        free(oe->pixel_buffer);
        free(oe);
    }
}

void ortho_export_set_bounds(OrthoExport* oe, Vec3 min, Vec3 max) {
    if (!oe) return;
    oe->min_bounds = min;
    oe->max_bounds = max;
}

void ortho_export_set_resolution(OrthoExport* oe, uint32_t width, uint32_t height) {
    if (!oe) return;
    oe->output_width = width;
    oe->output_height = height;
    free(oe->pixel_buffer);
    oe->pixel_buffer = (uint8_t*)malloc(width * height * 3);
}

static void set_ortho_projection(float left, float right, float bottom, float top, float near, float far) {
    float proj[16] = {0};
    proj[0] = 2.0f / (right - left);
    proj[5] = 2.0f / (top - bottom);
    proj[10] = -2.0f / (far - near);
    proj[12] = -(right + left) / (right - left);
    proj[13] = -(top + bottom) / (top - bottom);
    proj[14] = -(far + near) / (far - near);
    proj[15] = 1.0f;

    GLint loc = glGetUniformLocation(shader_program, "uProjection");
    if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, proj);
}

/* Fixed: Removed 'int skip_ui' parameter to match header */
void ortho_export_render_to_file(OrthoExport* oe, const char* filename, void (*render_callback)(void)) {
    if (!oe || !render_callback || !filename) return;
    
    set_ortho_projection(oe->min_bounds.x, oe->max_bounds.x,
                         oe->min_bounds.z, oe->max_bounds.z,
                         -1000.0f, 1000.0f);
    
    /* Render callback */
    render_callback();
    
    /* Read pixels */
    glReadPixels(0, 0, oe->output_width, oe->output_height, GL_RGB, GL_UNSIGNED_BYTE, oe->pixel_buffer);
    
    DEFER_FILE(fp) = fopen(filename, "wb");
    if (!fp) return;
    fprintf(fp, "P6\n%u %u\n255\n", oe->output_width, oe->output_height);
    
    /* PPM stores bottom-up, flip to top-down */
    for (uint32_t y = oe->output_height; y > 0; y--) {
        fwrite(&oe->pixel_buffer[(y-1) * oe->output_width * 3], 1, oe->output_width * 3, fp);
    }
}
