#ifndef SPLINE_H
#define SPLINE_H
#pragma once

#include "vector3d.h"
#include <stdint.h>

typedef struct Spline {
    Vec3* control_points;
    uint32_t count;
    uint32_t capacity;
} Spline;

#ifdef __cplusplus
extern "C" {
#endif

Spline* spline_create(uint32_t capacity);
void spline_destroy(Spline* s);
void spline_add_point(Spline* s, Vec3 point);
Vec3 spline_evaluate(const Spline* s, float t);
Vec3 spline_evaluate_arc_length(const Spline* s, float t);

#ifdef __cplusplus
}
#endif

#endif /* SPLINE_H */
