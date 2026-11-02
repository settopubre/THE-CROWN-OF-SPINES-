#ifndef ARSENAL_H
#define ARSENAL_H
#pragma once

#include "vector3d.h"
#include "mesh.h"

typedef struct {
    Mesh* platform;
    float radius;
    Vec3 position;
} Arsenal;

#ifdef __cplusplus
extern "C" {
#endif

Arsenal* arsenal_create(float radius, int segments);
void arsenal_destroy(Arsenal* arsenal);
void arsenal_render(Arsenal* arsenal, Mat4 view, Mat4 projection);
Vec3 arsenal_get_position(const Arsenal* arsenal);

#ifdef __cplusplus
}
#endif

#endif /* ARSENAL_H */
