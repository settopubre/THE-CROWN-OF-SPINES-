#ifndef CONSTRAINTS_H
#define CONSTRAINTS_H
#pragma once

#include "vector3d.h"
#include "scene_graph.h"
#include <stdint.h>

typedef enum {
    CONSTRAINT_PARALLEL,
    CONSTRAINT_TANGENT,
    CONSTRAINT_COINCIDENT,
    CONSTRAINT_DISTANCE,
    CONSTRAINT_ANGLE
} ConstraintType;

typedef struct {
    uint32_t type;
    uint32_t node_a;
    uint32_t node_b;
    double value;
} Constraint;

typedef struct {
    Constraint* constraints;
    uint32_t count;
    uint32_t capacity;
} ConstraintSystem;

#ifdef __cplusplus
extern "C" {
#endif

ConstraintSystem* constraint_system_create(uint32_t capacity);
void constraint_system_destroy(ConstraintSystem* cs);
void constraint_add(ConstraintSystem* cs, uint32_t type, uint32_t a, uint32_t b, double value);
int constraint_solve(ConstraintSystem* cs, SceneGraph* graph, int max_iterations);

#ifdef __cplusplus
}
#endif

#endif /* CONSTRAINTS_H */
