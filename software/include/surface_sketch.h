#ifndef SURFACE_SKETCH_H
#define SURFACE_SKETCH_H
#pragma once

#include "vector3d.h"
#include "mesh.h"
#include "shape_palette.h"

typedef enum {
    SKETCH_BUMP,
    SKETCH_HOLE
} SketchMode;

typedef struct {
    Mesh* target_mesh;
    uint32_t face_index;
    Vec3 face_normal;
    Vec3 face_center;
    SketchMode mode;
} SurfaceSketch;

#ifdef __cplusplus
extern "C" {
#endif

SurfaceSketch* surface_sketch_create(void);
void surface_sketch_destroy(SurfaceSketch* ss);
int surface_sketch_pick_face(SurfaceSketch* ss, Mesh* mesh, Vec3 ray_origin, Vec3 ray_dir);
Mesh* surface_sketch_place_shape(SurfaceSketch* ss, ShapeType type, float w, float d, float depth);

#ifdef __cplusplus
}
#endif

#endif /* SURFACE_SKETCH_H */
