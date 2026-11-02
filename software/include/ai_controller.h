#ifndef AI_CONTROLLER_H
#define AI_CONTROLLER_H

#include "vector3d.h"
#include "skeleton.h"

typedef struct AIController AIController;

#ifdef __cplusplus
extern "C" {
#endif

AIController* ai_controller_create(Skeleton* body);
void ai_controller_destroy(AIController* ai);
void ai_controller_set_target(AIController* ai, Vec3 target_position);
void ai_controller_update(AIController* ai, float delta_time);
Vec3 ai_controller_get_position(AIController* ai);
int ai_controller_is_moving(AIController* ai);

#ifdef __cplusplus
}
#endif

#endif /* AI_CONTROLLER_H */
