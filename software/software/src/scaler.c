#include "thermal.h"
#include "mesh.h"
#include "engine_abi.h"
#include "decimate.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

typedef struct {
    int poly_budget;
    int physics_rate;
    int r_paused;
    float thermal_threshold;
    float frame_time_avg[10];
    int frame_time_idx;
    float cool_start_time;
    float current_temp;
    uint32_t base_poly_budget;
} Scaler;

static float _get_avg_frame_time(Scaler* s) {
    float sum = 0.0f;
    for(int i=0; i<10; i++) sum += s->frame_time_avg[i];
    return sum / 10.0f;
}

Scaler* scaler_create(int initial_poly_budget) {
    Scaler* s = (Scaler*)calloc(1, sizeof(Scaler));
    if (!s) return NULL;
    s->poly_budget = initial_poly_budget;
    s->base_poly_budget = initial_poly_budget;
    s->physics_rate = 120;
    s->r_paused = 0;
    s->thermal_threshold = 75.0f;
    s->cool_start_time = -1.0;
    return s;
}

void scaler_destroy(Scaler* s) {
    free(s);
}

int scaler_should_decimate(Scaler* s, float frame_time, float thermal_temp) {
    if (!s) return 0;
    
    /* Update thermal state */
    s->current_temp = thermal_temp;
    
    /* Update frame time history */
    s->frame_time_avg[s->frame_time_idx] = frame_time;
    s->frame_time_idx = (s->frame_time_idx + 1) % 10;
    
    float avg_ft = _get_avg_frame_time(s);
    
    /* Decimate if avg frame time > 20ms (50 FPS) */
    if (avg_ft > 0.020f) return 1;
    
    /* Decimate if thermal > 85°C */
    if (thermal_temp > 85.0f) return 1;
    
    return 0;
}

int scaler_target_poly_count(Scaler* s) {
    if (!s) return s->poly_budget;
    
    float avg_ft = _get_avg_frame_time(s);
    float target_ratio = 1.0f;
    
    /* Aggressively reduce target if performance is poor */
    if (avg_ft > 0.050f) target_ratio = 0.2f; /* 5x reduction */
    else if (avg_ft > 0.030f) target_ratio = 0.5f;
    else if (avg_ft > 0.020f) target_ratio = 0.8f;
    
    if (s->current_temp > 90.0f) target_ratio = 0.3f;
    else if (s->current_temp > 85.0f) target_ratio = 0.6f;
    
    int target = (int)(s->base_poly_budget * target_ratio);
    if (target < 1000) target = 1000; /* Minimum budget */
    return target;
}

void scaler_update(Scaler* s, ThermalMonitor* tm, float frame_time, uint32_t poly_count) {
    if (!s) return;
    
    float temp = tm ? tm->current_temp : 0.0f;
    
    if (scaler_should_decimate(s, frame_time, temp)) {
        s->poly_budget = scaler_target_poly_count(s);
        if (temp > 85.0f) {
            s->r_paused = 1;
            s->physics_rate = 30;
        } else {
            s->r_paused = 0;
            s->physics_rate = 60;
        }
        printf("[SCALER] Throttle active: Budget %d (Avg FT: %.3fs, Temp: %.1f)\n", s->poly_budget, frame_time, temp);
    } else {
        /* Restore performance */
        if (s->poly_budget < s->base_poly_budget) {
            s->poly_budget += 1000; /* Gradual increase */
            if (s->poly_budget > s->base_poly_budget) s->poly_budget = s->base_poly_budget;
        }
        if (temp < 70.0f && s->poly_budget == s->base_poly_budget) {
            s->r_paused = 0;
            s->physics_rate = 120;
        }
    }
}

void scaler_apply(Scaler* s, Mesh** mesh, Skeleton* skel) {
    if (!s || !mesh || !*mesh) return;
    
    uint32_t current_tris = mesh_get_triangle_count(*mesh);
    
    /* Only decimate if current count is significantly above budget */
    if ((int)current_tris > s->poly_budget * 1.1f) {
        printf("[SCALER] Decimating mesh: %u -> %d tris\n", current_tris, s->poly_budget);
        *mesh = decimate_mesh(*mesh, (uint32_t)s->poly_budget);
    }
}
