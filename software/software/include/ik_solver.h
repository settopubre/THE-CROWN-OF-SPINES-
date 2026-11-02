#ifndef IK_SOLVER_H
#define IK_SOLVER_H
#pragma once

#include "skeleton.h"
#include "vector3d.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void ik_solve_fabrik(Skeleton* skel, uint32_t end_effector_bone, Vec3 target, int max_iterations, float tolerance);

#ifdef __cplusplus
}
#endif

#endif /* IK_SOLVER_H */
