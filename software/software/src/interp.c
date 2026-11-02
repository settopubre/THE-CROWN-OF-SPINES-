#include "ai_thread.h"
#include "vector3d.h"
#include "kalman.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static Vec3 catmull_rom_1d(double p0, double p1, double p2, double p3, double t) {
    double t2 = t * t;
    double t3 = t2 * t;
    return (Vec3){0,
        0.5 * ((2.0 * p1) +
               (-p0 + p2) * t +
               (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t2 +
               (-p0 + 3.0 * p1 - 3.0 * p2 + p3) * t3),
        0, 0
    };
}

static Quat quat_slerp_simple(Quat a, Quat b, double t) {
    double cos_half = a.w*b.w + a.x*b.x + a.y*b.y + a.z*b.z;
    if (cos_half < 0) { a.w=-a.w; a.x=-a.x; a.y=-a.y; a.z=-a.z; cos_half=-cos_half; }
    if (cos_half > 0.9995) {
        Quat r = {a.w+t*(b.w-a.w), a.x+t*(b.x-a.x), a.y+t*(b.y-a.y), a.z+t*(b.z-a.z)};
        double len = sqrt(r.w*r.w + r.x*r.x + r.y*r.y + r.z*r.z);
        if (len > 1e-10) { r.w/=len; r.x/=len; r.y/=len; r.z/=len; }
        return r;
    }
    double theta = acos(cos_half);
    double sin_theta = sin(theta);
    double ra = sin((1-t)*theta)/sin_theta, rb = sin(t*theta)/sin_theta;
    return (Quat){a.w*ra+b.w*rb, a.x*ra+b.x*rb, a.y*ra+b.y*rb, a.z*ra+b.z*rb};
}

void interp_poses(AIPose* out, const AIPose* p0, const AIPose* p1, const AIPose* p2, const AIPose* p3, double t) {
    if (!out || !p1) return;
    out->timestamp = p1->timestamp + t * (p2 ? p2->timestamp - p1->timestamp : 0);
    out->joint_count = p1->joint_count;
    
    for (uint32_t i = 0; i < out->joint_count && i < MAX_JOINTS; i++) {
        Vec3 v0 = p0 ? p0->joint_positions[i] : p1->joint_positions[i];
        Vec3 v1 = p1->joint_positions[i];
        Vec3 v2 = p2 ? p2->joint_positions[i] : p1->joint_positions[i];
        Vec3 v3 = p3 ? p3->joint_positions[i] : p2 ? p2->joint_positions[i] : p1->joint_positions[i];
        
        Vec3 interp_x = catmull_rom_1d(v0.x, v1.x, v2.x, v3.x, t);
        Vec3 interp_y = catmull_rom_1d(v0.y, v1.y, v2.y, v3.y, t);
        Vec3 interp_z = catmull_rom_1d(v0.z, v1.z, v2.z, v3.z, t);
        out->joint_positions[i] = (Vec3){v1.id, interp_x.y, interp_y.y, interp_z.y};
        
        Quat q0 = p0 ? p0->joint_rotations[i] : p1->joint_rotations[i];
        Quat q1 = p1->joint_rotations[i];
        Quat q2 = p2 ? p2->joint_rotations[i] : q1;
        Quat q3 = p3 ? p3->joint_rotations[i] : q2;
        (void)q3; /* Reserved for higher-order spline interpolation */
        Quat qa = quat_slerp_simple(q0, q1, t);
        Quat qb = quat_slerp_simple(q1, q2, t);
        out->joint_rotations[i] = quat_slerp_simple(qa, qb, t);
    }
}

void interp_predict(AIPose* out, const AIPose* last, const AIPose* prev, double delta_time) {
    if (!out || !last) return;
    *out = *last;
    out->timestamp = last->timestamp + delta_time;
    
    if (!prev) return;
    
    double dt = last->timestamp - prev->timestamp;
    if (dt <= 1e-6) return;
    
    for (uint32_t i = 0; i < out->joint_count && i < MAX_JOINTS; i++) {
        Vec3 vel = vec3_sub(last->joint_positions[i], prev->joint_positions[i]);
        out->joint_positions[i] = vec3_add(last->joint_positions[i], vec3_mul(vel, (float)(delta_time / dt)));
        
        /* Simple rotation extrapolation */
        float t_extrap = 1.0f + (float)(delta_time / dt);
        out->joint_rotations[i] = quat_slerp_simple(prev->joint_rotations[i], last->joint_rotations[i], t_extrap);
    }
}
