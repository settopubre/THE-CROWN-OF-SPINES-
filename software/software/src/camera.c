#include "engine_abi.h"
#include "vector3d.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct Camera {
    Vec3 target;
    Vec3 position;
    float azimuth, polar;
    float distance;
    float min_polar, max_polar;
    float min_distance, max_distance;
    int background_index;
    Mat4 view_matrix;
    Mat4 projection_matrix;
    int dirty;
    
    /* Transition state */
    int transition_active;
    int target_mode;
    float transition_timer;
    float transition_duration;
    Vec3 start_pos, start_target;
    float start_distance;
    float target_distance;
};

Camera* camera_create_orbit(float distance) {
    Camera* cam = (Camera*)calloc(1, sizeof(Camera));
    if (!cam) return NULL;
    cam->distance = distance;
    cam->min_distance = 1.0f; cam->max_distance = 100.0f;
    cam->target = vec3_create(0.0, 0.0, 0.0);
    cam->azimuth = 0.0f; cam->polar = (float)(45.0 * M_PI / 180.0);
    cam->min_polar = (float)(5.0 * M_PI / 180.0); cam->max_polar = (float)(85.0 * M_PI / 180.0);
    cam->background_index = 0; cam->dirty = 1;
    return cam;
}

void camera_destroy(Camera* cam) { free(cam); }

void camera_switch_mode(Camera* cam, int target_mode, float transition_time) {
    if (!cam || target_mode < 1 || target_mode > 7) return;
    
    cam->transition_active = 1;
    cam->target_mode = target_mode;
    cam->transition_timer = 0.0f;
    cam->transition_duration = transition_time;
    
    cam->start_pos = cam->position;
    cam->start_target = cam->target;
    cam->start_distance = cam->distance;
    
    /* Pre-calculate target values */
    switch (target_mode) {
        case 1: cam->target_distance = 0.5f; break;
        case 2: cam->target_distance = 8.0f; break;
        case 3: cam->target_distance = 8.0f; break;
        case 4: cam->target_distance = 10.0f; break;
        case 5: cam->target_distance = 5.0f; break;
        case 6: cam->target_distance = 12.0f; break;
        case 7: cam->target_distance = 15.0f; break;
        default: cam->target_distance = 5.0f;
    }
}

static float _cubic_ease(float t) { return t * t * (3.0f - 2.0f * t); }

void camera_update(Camera* cam, float delta_time) {
    if (!cam) return;
    
    if (cam->transition_active) {
        cam->transition_timer += delta_time;
        float t = cam->transition_timer / cam->transition_duration;
        if (t >= 1.0f) {
            t = 1.0f;
            cam->transition_active = 0;
        }
        
        float ease = _cubic_ease(t);
        
        cam->distance = cam->start_distance + (cam->target_distance - cam->start_distance) * ease;
        /* Interpolate target smoothly */
        cam->target.x = cam->start_target.x + (0.0f - cam->start_target.x) * ease;
        cam->target.y = cam->start_target.y + (0.0f - cam->start_target.y) * ease;
        cam->target.z = cam->start_target.z + (0.0f - cam->start_target.z) * ease;
        
        cam->dirty = 1;
    } else {
        /* Standard orbit update */
        if (cam->dirty) {
            double d = (double)cam->distance;
            double az = (double)cam->azimuth;
            double pol = (double)cam->polar;
            double t_x = cam->target.x, t_y = cam->target.y, t_z = cam->target.z;

            double sin_az = sin(az), cos_az = cos(az);
            double sin_pol = sin(pol), cos_pol = cos(pol);

            cam->position.x = (float)(t_x + d * sin_az * cos_pol);
            cam->position.y = (float)(t_y + d * sin_pol);
            cam->position.z = (float)(t_z + d * cos_az * cos_pol);
            cam->dirty = 0;
        }
    }
}

void camera_mouse_drag(Camera* cam, float dx, float dy) {
    if (!cam || cam->transition_active) return;
    cam->azimuth += dx * 0.01f;
    cam->polar += dy * 0.01f;
    if (cam->polar < cam->min_polar) cam->polar = cam->min_polar;
    if (cam->polar > cam->max_polar) cam->polar = cam->max_polar;
    cam->dirty = 1;
}

void camera_mouse_scroll(Camera* cam, float delta) {
    if (!cam || cam->transition_active) return;
    cam->distance -= delta * 0.5f;
    if (cam->distance < cam->min_distance) cam->distance = cam->min_distance;
    if (cam->distance > cam->max_distance) cam->distance = cam->max_distance;
    cam->dirty = 1;
}

Mat4 camera_get_view_matrix(const Camera* cam) {
    if (!cam) return mat4_identity();
    Vec3 pos_v = vec3_create(cam->position.x, cam->position.y, cam->position.z);
    Vec3 tgt_v = vec3_create(cam->target.x, cam->target.y, cam->target.z);
    Vec3 up_v = vec3_create(0.0, 1.0, 0.0);
    return mat4_lookat(pos_v, tgt_v, up_v);
}

Mat4 camera_get_projection_matrix(const Camera* cam, float aspect) {
    (void)cam; /* Explicitly silence unused-parameter warning */
    double fov = 45.0 * M_PI / 180.0;
    return mat4_perspective(fov, (double)aspect, 0.1, 1000.0);
}

void camera_set_target(Camera* cam, Vec3 target) { 
    if (cam) {
        cam->target = target;
        cam->dirty = 1;
    } 
}

void camera_set_distance(Camera* cam, float distance) { 
    if (cam) {
        cam->distance = distance;
        cam->dirty = 1;
    } 
}

void camera_set_position(Camera* cam, Vec3 pos) { 
    if (cam) {
        cam->position = pos;
        cam->dirty = 1;
    } 
}

int camera_get_background_index(const Camera* cam) { 
    return cam ? cam->background_index : 0; 
}

void camera_cycle_background(Camera* cam) { 
    if (cam) {
        cam->background_index = (cam->background_index + 1) % 3; 
    }
}
