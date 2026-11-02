#include "vector3d.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {
    Vec3* control_points;
    uint32_t count;
    uint32_t capacity;
} Spline;

Spline* spline_create(uint32_t capacity) {
    if (capacity == 0) capacity = 16;
    Spline* s = (Spline*)calloc(1, sizeof(Spline));
    if (!s) return NULL;
    s->control_points = (Vec3*)calloc(capacity, sizeof(Vec3));
    if (!s->control_points) { free(s); return NULL; }
    s->capacity = capacity;
    s->count = 0;
    return s;
}

void spline_destroy(Spline* s) {
    if (s) { free(s->control_points); free(s); }
}

void spline_add_point(Spline* s, Vec3 point) {
    if (!s) return;
    if (s->count >= s->capacity) {
        uint32_t new_cap = s->capacity * 2;
        Vec3* new_pts = (Vec3*)realloc(s->control_points, new_cap * sizeof(Vec3));
        if (!new_pts) return;
        s->control_points = new_pts;
        s->capacity = new_cap;
    }
    s->control_points[s->count++] = point;
}

static double _catmull_rom_1d(double p0, double p1, double p2, double p3, double t) {
    double t2 = t * t, t3 = t2 * t;
    return 0.5 * ((2.0 * p1) + (-p0 + p2) * t + (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t2 + (-p0 + 3.0 * p1 - 3.0 * p2 + p3) * t3);
}

Vec3 spline_evaluate(const Spline* s, float t) {
    Vec3 result = vec3_create(0.0, 0.0, 0.0);
    if (!s || s->count < 2) return result;
    
    if (s->count == 2) return vec3_lerp(s->control_points[0], s->control_points[1], t);
    
    float ft = t * (s->count - 1);
    uint32_t i0 = (uint32_t)ft;
    uint32_t i1 = (i0 + 1 < s->count) ? i0 + 1 : i0;
    uint32_t i2 = (i1 + 1 < s->count) ? i1 + 1 : i1;
    uint32_t i3 = (i2 + 1 < s->count) ? i2 + 1 : i2;
    if (i0 == 0) i0 = i1;
    
    float lt = ft - i0;
    Vec3 p0 = s->control_points[i0];
    Vec3 p1 = s->control_points[i1];
    Vec3 p2 = s->control_points[i2];
    Vec3 p3 = s->control_points[i3];
    
    result.x = _catmull_rom_1d(p0.x, p1.x, p2.x, p3.x, lt);
    result.y = _catmull_rom_1d(p0.y, p1.y, p2.y, p3.y, lt);
    result.z = _catmull_rom_1d(p0.z, p1.z, p2.z, p3.z, lt);
    return result;
}

Vec3 spline_evaluate_arc_length(const Spline* s, float t) {
    return spline_evaluate(s, t);
}
