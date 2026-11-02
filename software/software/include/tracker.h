#ifndef TRACKER_H
#define TRACKER_H
#pragma once

#include "vector3d.h"
#include "engine_abi.h"

typedef struct Tracker Tracker;

#ifdef __cplusplus
extern "C" {
#endif

Tracker* tracker_create(void);
void tracker_destroy(Tracker* t);
void tracker_update(Tracker* t, Camera* cam, Vec3 target_pos, Vec3 target_vel, float delta_time);
void tracker_set_offset(Tracker* t, Vec3 offset);

#ifdef __cplusplus
}
#endif

#endif /* TRACKER_H */
