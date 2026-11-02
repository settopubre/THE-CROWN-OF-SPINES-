#include "vector3d.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* === EXISTING FUNCTIONS (preserved) === */
Vec3 vec3_create(double x, double y, double z) {
    Vec3 v;
    v.id = 0;
    v.x = x; v.y = y; v.z = z;
    return v;
}

double vec3_distance(const Vec3* a, const Vec3* b) {
    if (!a || !b) return -1.0;
    double dx = a->x - b->x, dy = a->y - b->y, dz = a->z - b->z;
    return sqrt(dx*dx + dy*dy + dz*dz);
}

double vec3_determinant_3x3(const Vec3* a, const Vec3* b, const Vec3* c) {
    if (!a || !b || !c) return 0.0;
    double det = a->x*(b->y*c->z - b->z*c->y)
               - a->y*(b->x*c->z - b->z*c->x)
               + a->z*(b->x*c->y - b->y*c->x);
    return (fabs(det) < 1e-10) ? 0.0 : det;
}

void vec3_cleanup(Vec3** vec_array) {
    if (vec_array && *vec_array) { free(*vec_array); *vec_array = NULL; }
}

void safe_vec3_demo(void) {
    printf("\n=== DEFER MACRO DEMONSTRATION ===\n");
    printf("  [DEFER] Module loaded - cleanup macros active\n");
    printf("=== DEMO COMPLETE ===\n\n");
}

/* === NEW VEC3 FUNCTIONS === */
Vec3 vec3_add(Vec3 a, Vec3 b) {
    Vec3 r = {a.id, a.x + b.x, a.y + b.y, a.z + b.z};
    return r;
}

Vec3 vec3_sub(Vec3 a, Vec3 b) {
    Vec3 r = {a.id, a.x - b.x, a.y - b.y, a.z - b.z};
    return r;
}

Vec3 vec3_mul(Vec3 v, double s) {
    Vec3 r = {v.id, v.x * s, v.y * s, v.z * s};
    return r;
}

Vec3 vec3_cross(Vec3 a, Vec3 b) {
    Vec3 r = {0,
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
    return r;
}

double vec3_dot(Vec3 a, Vec3 b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

double vec3_length(Vec3 v) {
    return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

Vec3 vec3_normalize(Vec3 v) {
    double len = vec3_length(v);
    if (len < 1e-10) return v;
    return vec3_mul(v, 1.0 / len);
}

Vec3 vec3_lerp(Vec3 a, Vec3 b, double t) {
    Vec3 r = {a.id,
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    };
    return r;
}

/* === QUATERNION FUNCTIONS === */
Quat quat_identity(void) {
    Quat q = {1.0, 0.0, 0.0, 0.0};
    return q;
}

Quat quat_from_axis_angle(Vec3 axis, double angle) {
    Quat q;
    double half = angle * 0.5;
    double s = sin(half);
    Vec3 n = vec3_normalize(axis);
    q.w = cos(half);
    q.x = n.x * s;
    q.y = n.y * s;
    q.z = n.z * s;
    return q;
}

Quat quat_mul(Quat a, Quat b) {
    Quat r;
    r.w = a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z;
    r.x = a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y;
    r.y = a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x;
    r.z = a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w;
    return r;
}

Vec3 quat_rotate(Quat q, Vec3 v) {
    Quat p = {0.0, v.x, v.y, v.z};
    Quat q_conj = {q.w, -q.x, -q.y, -q.z};
    Quat result = quat_mul(quat_mul(q, p), q_conj);
    Vec3 r = {v.id, result.x, result.y, result.z};
    return r;
}

Quat quat_slerp(Quat a, Quat b, double t) {
    double cos_half_theta = a.w*b.w + a.x*b.x + a.y*b.y + a.z*b.z;
    if (cos_half_theta < 0) {
        a.w = -a.w; a.x = -a.x; a.y = -a.y; a.z = -a.z;
        cos_half_theta = -cos_half_theta;
    }
    if (cos_half_theta > 0.9995) {
        Quat r = {
            a.w + t*(b.w - a.w),
            a.x + t*(b.x - a.x),
            a.y + t*(b.y - a.y),
            a.z + t*(b.z - a.z)
        };
        double len = sqrt(r.w*r.w + r.x*r.x + r.y*r.y + r.z*r.z);
        if (len > 1e-10) { r.w/=len; r.x/=len; r.y/=len; r.z/=len; }
        return r;
    }
    double half_theta = acos(cos_half_theta);
    double sin_half_theta = sin(half_theta);
    double ratio_a = sin((1.0 - t) * half_theta) / sin_half_theta;
    double ratio_b = sin(t * half_theta) / sin_half_theta;
    Quat r = {
        a.w * ratio_a + b.w * ratio_b,
        a.x * ratio_a + b.x * ratio_b,
        a.y * ratio_a + b.y * ratio_b,
        a.z * ratio_a + b.z * ratio_b
    };
    return r;
}

/* === MATRIX 4x4 FUNCTIONS === */
static void mat4_set_identity(double m[16]) {
    memset(m, 0, 16 * sizeof(double));
    m[0] = m[5] = m[10] = m[15] = 1.0;
}

Mat4 mat4_identity(void) {
    Mat4 m;
    mat4_set_identity(m.m);
    return m;
}

Mat4 mat4_mul(Mat4 a, Mat4 b) {
    Mat4 r;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            r.m[i*4+j] = 0;
            for (int k = 0; k < 4; k++)
                r.m[i*4+j] += a.m[i*4+k] * b.m[k*4+j];
        }
    }
    return r;
}

Mat4 mat4_from_quat(Quat q) {
    Mat4 m = mat4_identity();
    double xx = q.x*q.x, yy = q.y*q.y, zz = q.z*q.z;
    double xy = q.x*q.y, xz = q.x*q.z, yz = q.y*q.z;
    double wx = q.w*q.x, wy = q.w*q.y, wz = q.w*q.z;
    m.m[0] = 1 - 2*(yy + zz);  m.m[1] = 2*(xy - wz);    m.m[2] = 2*(xz + wy);
    m.m[4] = 2*(xy + wz);      m.m[5] = 1 - 2*(xx + zz); m.m[6] = 2*(yz - wx);
    m.m[8] = 2*(xz - wy);      m.m[9] = 2*(yz + wx);    m.m[10] = 1 - 2*(xx + yy);
    return m;
}

Mat4 mat4_translate(Vec3 t) {
    Mat4 m = mat4_identity();
    m.m[12] = t.x; m.m[13] = t.y; m.m[14] = t.z;
    return m;
}

Mat4 mat4_scale(Vec3 s) {
    Mat4 m = mat4_identity();
    m.m[0] = s.x; m.m[5] = s.y; m.m[10] = s.z;
    return m;
}

Mat4 mat4_rotate(double angle, Vec3 axis) {
    Vec3 a = vec3_normalize(axis);
    double c = cos(angle), s = sin(angle), t = 1.0 - c;
    Mat4 m = mat4_identity();
    m.m[0] = t*a.x*a.x + c;      m.m[1] = t*a.x*a.y - s*a.z;  m.m[2] = t*a.x*a.z + s*a.y;
    m.m[4] = t*a.x*a.y + s*a.z;  m.m[5] = t*a.y*a.y + c;      m.m[6] = t*a.y*a.z - s*a.x;
    m.m[8] = t*a.x*a.z - s*a.y;  m.m[9] = t*a.y*a.z + s*a.x;  m.m[10] = t*a.z*a.z + c;
    return m;
}

Mat4 mat4_perspective(double fov, double aspect, double near, double far) {
    Mat4 m = {0};
    double tan_half_fov = tan(fov * 0.5);
    m.m[0] = 1.0 / (aspect * tan_half_fov);
    m.m[5] = 1.0 / tan_half_fov;
    m.m[10] = -(far + near) / (far - near);
    m.m[11] = -1.0;
    m.m[14] = -(2.0 * far * near) / (far - near);
    return m;
}

Mat4 mat4_lookat(Vec3 eye, Vec3 center, Vec3 up) {
    Vec3 f = vec3_normalize(vec3_sub(center, eye));
    Vec3 s = vec3_normalize(vec3_cross(f, up));
    Vec3 u = vec3_cross(s, f);
    Mat4 m = mat4_identity();
    m.m[0] = s.x;  m.m[1] = u.x;  m.m[2] = -f.x;
    m.m[4] = s.y;  m.m[5] = u.y;  m.m[6] = -f.y;
    m.m[8] = s.z;  m.m[9] = u.z;  m.m[10] = -f.z;
    m.m[12] = -vec3_dot(s, eye);
    m.m[13] = -vec3_dot(u, eye);
    m.m[14] =  vec3_dot(f, eye);
    return m;
}

Mat4 mat4_transpose(Mat4 m) {
    Mat4 r;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            r.m[i*4+j] = m.m[j*4+i];
    return r;
}
