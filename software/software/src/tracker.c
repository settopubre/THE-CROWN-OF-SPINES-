#include "engine_abi.h"
#include "vector3d.h"
#include <stdlib.h>
#include <math.h>

typedef struct {
    Vec3 target_offset;
    Vec3 current_pos;
    Vec3 current_offset;
    float follow_speed;
    float smooth_speed;
    float look_ahead;
} Tracker;

Tracker* tracker_create(void) {
    Tracker* t = (Tracker*)calloc(1, sizeof(Tracker));
    if (!t) return NULL;
    t->target_offset = vec3_create(0.0, 2.0, 5.0);
    t->current_pos = vec3_create(0.0, 2.0, 5.0);
    t->current_offset = vec3_create(0.0, 2.0, 5.0);
    t->follow_speed = 5.0f;
    t->smooth_speed = 5.0f;
    t->look_ahead = 0.5f;
    return t;
}

void tracker_destroy(Tracker* t) {
    free(t);
}

void tracker_set_smooth_speed(Tracker* t, float speed) {
    if (t) t->smooth_speed = speed;
}

void tracker_update(Tracker* t, Camera* cam, Vec3 target_pos, Vec3 target_vel, float delta_time) {
    if (!t || !cam) return;
    
    /* Calculate desired position based on prediction */
    Vec3 predicted = vec3_add(target_pos, vec3_mul(target_vel, t->look_ahead));
    Vec3 desired_offset = vec3_add(predicted, t->target_offset);
    
    /* Exponential smoothing */
    float alpha = 1.0f - expf(-t->smooth_speed * delta_time);
    
    /* Interpolate offset */
    Vec3 diff = vec3_sub(desired_offset, t->current_pos);
    t->current_pos.x = t->current_pos.x + diff.x * alpha;
    t->current_pos.y = t->current_pos.y + diff.y * alpha;
    t->current_pos.z = t->current_pos.z + diff.z * alpha;
    
    /* Apply to camera */
    /* Note: In a real tracker, we usually set the camera position to current_pos
       or look_at based on it. Here we update the camera target/distance as per legacy behavior. */
    camera_set_target(cam, predicted);
    camera_set_distance(cam, vec3_length(vec3_sub(t->current_pos, predicted)));
}

void tracker_set_offset(Tracker* t, Vec3 offset) {
    if (t) t->target_offset = offset;
}
