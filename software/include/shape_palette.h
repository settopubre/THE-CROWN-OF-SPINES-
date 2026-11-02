#ifndef SHAPE_PALETTE_H
#define SHAPE_PALETTE_H
#pragma once

#include "mesh.h"
#include "grid.h"
#include <stdint.h>

typedef enum {
    SHAPE_SQUARE,
    SHAPE_CIRCLE,
    SHAPE_RECTANGLE,
    SHAPE_POLYGON,
    SHAPE_LINE
} ShapeType;

typedef struct {
    ShapeType type;
    Mesh* mesh;
    Vec3 position;
    float width, depth;
    int segments;
    float bevel_size;
    int bevel_segments;
} Shape;

#ifdef __cplusplus
extern "C" {
#endif

Shape* shape_create(ShapeType type, Vec3 position, float w, float d, int segments);
void shape_destroy(Shape* s);
void shape_render(Shape* s, Mat4 view, Mat4 proj);
Mesh* shape_generate_mesh(ShapeType type, float w, float d, int segments);
void shape_set_position(Shape* s, Vec3 pos);
void shape_set_bevel(Shape* s, float bevel_size, int segments);

#ifdef __cplusplus
}
#endif

#endif /* SHAPE_PALETTE_H */
