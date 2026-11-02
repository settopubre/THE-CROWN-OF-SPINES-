#include "pour_sequence.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* FIX: Explicit typedef so we can use 'PourSequence' without the 'struct' keyword */
typedef struct PourSequence PourSequence;

typedef enum {
    POUR_EXCAVATION,
    POUR_GRAVEL,
    POUR_FORMWORK,
    POUR_REBAR,
    POUR_POURING,
    POUR_FINISHING,
    POUR_CURING,
    POUR_COMPLETE,
    POUR_PAUSED
} PourState;

struct PourSequence {
    float total_volume;
    float pour_rate; /* volume per second */
    float current_progress; /* 0.0 to 1.0 */
    PourState state;
    int is_running;
    float cure_timer;
};

static const char* state_names[] = {
    "excavation", "gravel", "formwork", "rebar", "pouring", "finishing", "curing", "complete", "paused"
};

PourSequence* pour_create(float total_volume, float pour_rate) {
    PourSequence* ps = (PourSequence*)calloc(1, sizeof(PourSequence));
    if (!ps) return NULL;
    ps->total_volume = total_volume;
    ps->pour_rate = pour_rate > 0.0f ? pour_rate : 1.0f;
    ps->state = POUR_EXCAVATION;
    ps->is_running = 0;
    return ps;
}

void pour_start(PourSequence* ps) {
    if (!ps) return;
    ps->is_running = 1;
    if (ps->state == POUR_PAUSED) {
        /* Resume from paused state logic if needed, otherwise just continue */
    }
}

void pour_pause(PourSequence* ps) {
    if (!ps) return;
    ps->is_running = 0;
    if (ps->state != POUR_COMPLETE) ps->state = POUR_PAUSED;
}

float pour_get_progress(PourSequence* ps) {
    if (!ps) return 0.0f;
    return ps->current_progress;
}

const char* pour_get_state(PourSequence* ps) {
    if (!ps) return "unknown";
    return state_names[ps->state];
}

void pour_update(PourSequence* ps, float delta_time) {
    if (!ps || !ps->is_running) return;
    
    /* State machine driven by progress thresholds */
    if (ps->state == POUR_COMPLETE) return;
    
    float increment = (ps->pour_rate * delta_time) / ps->total_volume;
    ps->current_progress += increment;
    
    if (ps->current_progress > 1.0f) ps->current_progress = 1.0f;
    
    /* Thresholds: 
       0.0 - 0.1 : excavation
       0.1 - 0.2 : gravel
       0.2 - 0.3 : formwork
       0.3 - 0.4 : rebar
       0.4 - 0.8 : pouring
       0.8 - 0.9 : finishing
       0.9 - 1.0 : curing
       1.0       : complete */
       
    if (ps->current_progress < 0.1f) ps->state = POUR_EXCAVATION;
    else if (ps->current_progress < 0.2f) ps->state = POUR_GRAVEL;
    else if (ps->current_progress < 0.3f) ps->state = POUR_FORMWORK;
    else if (ps->current_progress < 0.4f) ps->state = POUR_REBAR;
    else if (ps->current_progress < 0.8f) ps->state = POUR_POURING;
    else if (ps->current_progress < 0.9f) ps->state = POUR_FINISHING;
    else if (ps->current_progress < 1.0f) ps->state = POUR_CURING;
    else {
        ps->current_progress = 1.0f;
        ps->state = POUR_COMPLETE;
        ps->is_running = 0;
    }
}
