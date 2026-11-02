#ifndef ORTHO_EXPORT_H
#define ORTHO_EXPORT_H
#pragma once

#include "vector3d.h"
#include <stdint.h>

typedef struct {
    Vec3 min_bounds;
    Vec3 max_bounds;
    float resolution;
    uint32_t output_width;
    uint32_t output_height;
    uint8_t* pixel_buffer;
} OrthoExport;

#ifdef __cplusplus
extern "C" {
#endif

OrthoExport* ortho_export_create(Vec3 min_bounds, Vec3 max_bounds, float resolution);
void ortho_export_destroy(OrthoExport* oe);
void ortho_export_render_to_file(OrthoExport* oe, const char* filename, void (*render_callback)(void));
void ortho_export_set_bounds(OrthoExport* oe, Vec3 min, Vec3 max);

#ifdef __cplusplus
}
#endif

#endif /* ORTHO_EXPORT_H */
