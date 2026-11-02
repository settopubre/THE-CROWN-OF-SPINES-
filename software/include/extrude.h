#ifndef EXTRUDE_H
#define EXTRUDE_H
#pragma once

#include "vector3d.h"
#include "mesh.h"

typedef struct {
    Mesh* original;
    Mesh* extruded;
    float depth;
    int direction; /* 1=out, -1=in */
} Extrude;

#ifdef __cplusplus
extern "C" {
#endif

Extrude* extrude_create(Mesh* face, float depth, int direction);
void extrude_destroy(Extrude* e);
void extrude_render(Extrude* e);
Mesh* extrude_mesh(Mesh* face, float depth, int direction);

#ifdef __cplusplus
}
#endif

#endif /* EXTRUDE_H */
