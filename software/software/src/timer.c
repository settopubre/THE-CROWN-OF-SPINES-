#include "engine_abi.h"
#include <stdlib.h>
#include <time.h>
#include <math.h>

struct Timer {
    struct timespec last_tick;
    struct timespec current_tick;
    double delta_time;
    double frame_time;
    double fps;
    uint32_t frame_count;
    double fps_accumulator;
    
    /* Pause State */
    int is_paused;
    double pause_start_time;
    double total_paused_time;
};

static double _get_timespec_seconds(struct timespec* ts) {
    return ts->tv_sec + ts->tv_nsec * 1e-9;
}

Timer* engine_timer_create(void) {
    Timer* t = (Timer*)calloc(1, sizeof(Timer));
    if (!t) return NULL;
    clock_gettime(CLOCK_MONOTONIC, &t->last_tick);
    t->current_tick = t->last_tick;
    t->is_paused = 0;
    t->pause_start_time = 0.0;
    t->total_paused_time = 0.0;
    return t;
}

void engine_timer_destroy(Timer* t) {
    free(t);
}

void engine_timer_pause(Timer* t) {
    if (!t || t->is_paused) return;
    t->is_paused = 1;
    clock_gettime(CLOCK_MONOTONIC, &t->current_tick);
    t->pause_start_time = _get_timespec_seconds(&t->current_tick);
}

void engine_timer_resume(Timer* t) {
    if (!t || !t->is_paused) return;
    
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    double current_time = _get_timespec_seconds(&now);
    
    /* Accumulate the duration we were paused */
    t->total_paused_time += (current_time - t->pause_start_time);
    
    t->is_paused = 0;
    
    /* Reset last_tick to now so delta doesn't jump on resume */
    t->last_tick = now;
}

void engine_timer_reset(Timer* t) {
    if (!t) return;
    clock_gettime(CLOCK_MONOTONIC, &t->last_tick);
    t->current_tick = t->last_tick;
    t->delta_time = 0.0;
    t->frame_time = 0.0;
    t->fps = 0.0;
    t->frame_count = 0;
    t->fps_accumulator = 0.0;
    t->is_paused = 0;
    t->total_paused_time = 0.0;
}

void engine_timer_tick(Timer* t) {
    if (!t) return;
    if (t->is_paused) return; /* Do not update time while paused */

    clock_gettime(CLOCK_MONOTONIC, &t->current_tick);

    double now_raw = _get_timespec_seconds(&t->current_tick);
    double last_raw = _get_timespec_seconds(&t->last_tick);
    
    /* Delta is time elapsed minus time spent paused */
    t->delta_time = (now_raw - last_raw); 
    
    /* Note: Since we update last_tick every frame, and pause handles the jump,
       the logic is simply the difference between current and last, 
       provided resume updates last_tick to "now". */
       
    t->frame_time = t->delta_time;
    
    /* Update last tick for next frame */
    t->last_tick = t->current_tick;

    t->frame_count++;
    t->fps_accumulator += t->frame_time;

    if (t->fps_accumulator >= 1.0) {
        t->fps = (double)t->frame_count / t->fps_accumulator;
        t->frame_count = 0;
        t->fps_accumulator = 0.0;
    }
}

double engine_timer_get_fps(const Timer* t) {
    return t ? t->fps : 0.0;
}

double engine_timer_get_delta_time(const Timer* t) {
    return t ? t->delta_time : 0.0;
}

double engine_timer_get_frame_time(const Timer* t) {
    return t ? t->frame_time : 0.0;
}
