#include "ai_controller.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

struct AIController {
    Skeleton* body;
    Vec3 target;
    Vec3 position;
    Vec3 velocity;
    float walk_phase;
    int is_moving;
    float move_speed;
};

AIController* ai_controller_create(Skeleton* body) {
    AIController* ai = (AIController*)calloc(1, sizeof(AIController));
    if (!ai) return NULL;
    ai->body = body;
    ai->position = vec3_create(0.0, 0.0, 0.0);
    ai->target = vec3_create(0.0, 0.0, 0.0);
    ai->velocity = vec3_create(0.0, 0.0, 0.0);
    ai->walk_phase = 0.0f;
    ai->is_moving = 0;
    ai->move_speed = 2.0f;
    return ai;
}

void ai_controller_destroy(AIController* ai) {
    free(ai);
}

void ai_controller_set_target(AIController* ai, Vec3 target_position) {
    if (ai) {
        ai->target = target_position;
        ai->is_moving = 1;
    }
}

Vec3 ai_controller_get_position(AIController* ai) {
    return ai ? ai->position : vec3_create(0, 0, 0);
}

int ai_controller_is_moving(AIController* ai) {
    return ai ? ai->is_moving : 0;
}

void ai_controller_update(AIController* ai, float delta_time) {
    if (!ai) return;

    Vec3 to_target = vec3_sub(ai->target, ai->position);
    to_target.y = 0.0;
    
    float dist = sqrtf(to_target.x * to_target.x + to_target.z * to_target.z);
    
    if (dist > 0.1f) {
        Vec3 dir = vec3_normalize(to_target);
        ai->position.x += dir.x * ai->move_speed * delta_time;
        ai->position.z += dir.z * ai->move_speed * delta_time;
        ai->walk_phase += delta_time * 8.0f;
        if (ai->body) {
            skeleton_update_transforms(ai->body);
        }
    } else {
        ai->position = ai->target;
        ai->is_moving = 0;
    }
}
