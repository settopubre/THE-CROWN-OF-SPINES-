#ifndef SEQUENCER_H
#define SEQUENCER_H
#pragma once

#include "spline.h"
#include "engine_abi.h"
#include <stdint.h>

typedef struct {
    Spline* path;
    float duration;
    float fov;
    Vec3 look_at_target;
} Shot;

typedef struct Sequencer Sequencer;

#ifdef __cplusplus
extern "C" {
#endif

Sequencer* sequencer_create(uint32_t capacity);
void sequencer_destroy(Sequencer* seq);
void sequencer_add_shot(Sequencer* seq, Shot shot);
void sequencer_update(Sequencer* seq, Camera* cam, float delta_time);
void sequencer_play(Sequencer* seq);
void sequencer_pause(Sequencer* seq);
void sequencer_stop(Sequencer* seq);

#ifdef __cplusplus
}
#endif

#endif /* SEQUENCER_H */
