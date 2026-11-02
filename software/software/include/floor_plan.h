#ifndef FLOOR_PLAN_H
#define FLOOR_PLAN_H

#include "vector3d.h"
#include "mesh.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FloorPlan FloorPlan;

FloorPlan* floor_plan_create(void);
FloorPlan* floor_plan_get(void);
void floor_plan_destroy(FloorPlan* fp);
void floor_plan_add_point(FloorPlan* fp, Vec3 point);
void floor_plan_close_loop(FloorPlan* fp);
void floor_plan_extrude_walls(FloorPlan* fp);
void floor_plan_clear(FloorPlan* fp);
void floor_plan_set_wall_height(FloorPlan* fp, float height);
void floor_plan_set_wall_thickness(FloorPlan* fp, float thickness);
void floor_plan_render(FloorPlan* fp, Mat4 view, Mat4 proj);
int floor_plan_is_active(FloorPlan* fp);

#ifdef __cplusplus
}
#endif

#endif /* FLOOR_PLAN_H */
