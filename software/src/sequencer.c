#include "engine_abi.h"
#include "spline.h"
#include <stdlib.h>
#include <string.h>

#define MAX_EVENTS_PER_SHOT 64

typedef enum {
    EVENT_NONE,
    EVENT_PLAY_SOUND,
    EVENT_SPAWN_PARTICLE,
    EVENT_LOAD_MODEL,
    EVENT_TRIGGER_LUA
} SequenceEventType;

typedef struct {
    float time;
    int event_type;
    void* data;
    int triggered;
} SequenceEvent;

typedef struct {
    Spline* path;
    float duration;
    float fov;
    Vec3 look_at_target;
    
    /* Event System */
    SequenceEvent events[MAX_EVENTS_PER_SHOT];
    int event_count;
} Shot;

typedef struct {
    Shot* shots;
    uint32_t count;
    uint32_t capacity;
    uint32_t current_shot;
    float current_time;
    int playing;
} Sequencer;

Sequencer* sequencer_create(uint32_t capacity) {
    if (capacity == 0) capacity = 8;
    Sequencer* seq = (Sequencer*)calloc(1, sizeof(Sequencer));
    if (!seq) return NULL;
    seq->shots = (Shot*)calloc(capacity, sizeof(Shot));
    if (!seq->shots) { free(seq); return NULL; }
    seq->capacity = capacity;
    seq->count = 0;
    seq->current_shot = 0;
    seq->current_time = 0;
    seq->playing = 0;
    return seq;
}

void sequencer_destroy(Sequencer* seq) {
    if (!seq) return;
    for (uint32_t i = 0; i < seq->count; i++) {
        if (seq->shots[i].path) spline_destroy(seq->shots[i].path);
        /* Clean up event data if necessary */
    }
    free(seq->shots);
    free(seq);
}

void sequencer_add_shot(Sequencer* seq, Shot shot) {
    if (!seq || seq->count >= seq->capacity) return;
    if (shot.duration < 0.016f) shot.duration = 0.016f;
    seq->shots[seq->count++] = shot;
}

void sequencer_add_event(Sequencer* seq, int shot_index, float time, int event_type, void* data) {
    if (!seq || shot_index < 0 || shot_index >= seq->count) return;
    Shot* shot = &seq->shots[shot_index];
    
    if (shot->event_count >= MAX_EVENTS_PER_SHOT) return;
    
    SequenceEvent* ev = &shot->events[shot->event_count];
    ev->time = time;
    ev->event_type = event_type;
    ev->data = data;
    ev->triggered = 0;
    
    shot->event_count++;
}

void sequencer_clear_events(Sequencer* seq) {
    if (!seq) return;
    for (uint32_t i = 0; i < seq->count; i++) {
        seq->shots[i].event_count = 0;
        memset(seq->shots[i].events, 0, sizeof(seq->shots[i].events));
    }
}

void sequencer_update(Sequencer* seq, Camera* cam, float delta_time) {
    if (!seq || !cam || !seq->playing || seq->count == 0) return;
    
    Shot* shot = &seq->shots[seq->current_shot];
    seq->current_time += delta_time;
    
    /* Check for shot end */
    if (seq->current_time >= shot->duration) {
        seq->current_shot = (seq->current_shot + 1) % seq->count;
        seq->current_time = 0;
        shot = &seq->shots[seq->current_shot];
        
        /* Reset events for new shot */
        for(int i=0; i<shot->event_count; i++) shot->events[i].triggered = 0;
    }
    
    float t;
    if (shot->duration <= 0.001f) {
        t = 0.0f;
    } else {
        t = seq->current_time / shot->duration;
    }
    
    /* Update Camera */
    if (shot->path && shot->path->count > 0) {
        Vec3 cam_pos = spline_evaluate(shot->path, t);
        camera_set_position(cam, cam_pos);
        camera_set_target(cam, shot->look_at_target);
    }
    
    /* Trigger Events */
    for (int i = 0; i < shot->event_count; i++) {
        SequenceEvent* ev = &shot->events[i];
        if (!ev->triggered && seq->current_time >= ev->time) {
            ev->triggered = 1;
            /* Execute event logic based on type */
            switch (ev->event_type) {
                case EVENT_TRIGGER_LUA:
                    if (ev->data) {
                        /* Call lua_bindings_call((char*)ev->data) */
                    }
                    break;
                case EVENT_PLAY_SOUND:
                    /* Play sound logic */
                    break;
                default:
                    break;
            }
        }
    }
}

void sequencer_play(Sequencer* seq) { if (seq) seq->playing = 1; }
void sequencer_pause(Sequencer* seq) { if (seq) seq->playing = 0; }
void sequencer_stop(Sequencer* seq) { 
    if (seq) { seq->playing = 0; seq->current_shot = 0; seq->current_time = 0; } 
}
