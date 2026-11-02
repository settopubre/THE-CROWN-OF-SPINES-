#ifndef VECTOR3D_H
#define VECTOR3D_H
#pragma once

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

typedef struct {
    int id;
    double x;
    double y;
    double z;
} Vec3;

typedef struct {
    double w, x, y, z;
} Quat;

typedef struct {
    double m[16];
} Mat4;

static inline void _defer_close_file(FILE** fp) {
    if (fp && *fp) {
        fclose(*fp);
        *fp = NULL;
    }
}

static inline void _defer_close_dir(DIR** dirp) {
    if (dirp && *dirp) {
        closedir(*dirp);
        *dirp = NULL;
    }
}

static inline void _defer_free(void** ptr) {
    if (ptr && *ptr) {
        free(*ptr);
        *ptr = NULL;
    }
}

static inline void _defer_cleanup_vec3(Vec3** vec);

#define DEFER_CLEANUP(cleanup_func, type, var) \
    __attribute__((cleanup(cleanup_func))) type var

#define DEFER_FILE(var) \
    __attribute__((cleanup(_defer_close_file))) FILE* var

#define DEFER_DIR(var) \
    __attribute__((cleanup(_defer_close_dir))) DIR* var

#define DEFER_FREE(var) \
    __attribute__((cleanup(_defer_free))) void* var

#define DEFER_VEC3_ARRAY(var) \
    __attribute__((cleanup(_defer_cleanup_vec3))) Vec3* var

static inline void _defer_cleanup_quat(Quat** q);
#define DEFER_QUAT_ARRAY(var) \
    __attribute__((cleanup(_defer_cleanup_quat))) Quat* var

#ifdef __cplusplus
extern "C" {
#endif

Vec3 vec3_create(double x, double y, double z);
double vec3_distance(const Vec3* a, const Vec3* b);
double vec3_determinant_3x3(const Vec3* a, const Vec3* b, const Vec3* c);
void vec3_cleanup(Vec3** vec_array);

Vec3 vec3_add(Vec3 a, Vec3 b);
Vec3 vec3_sub(Vec3 a, Vec3 b);
Vec3 vec3_mul(Vec3 v, double s);
Vec3 vec3_cross(Vec3 a, Vec3 b);
double vec3_dot(Vec3 a, Vec3 b);
double vec3_length(Vec3 v);
Vec3 vec3_normalize(Vec3 v);
Vec3 vec3_lerp(Vec3 a, Vec3 b, double t);

Quat quat_identity(void);
Quat quat_from_axis_angle(Vec3 axis, double angle);
Quat quat_mul(Quat a, Quat b);
Vec3 quat_rotate(Quat q, Vec3 v);
Quat quat_slerp(Quat a, Quat b, double t);

Mat4 mat4_identity(void);
Mat4 mat4_mul(Mat4 a, Mat4 b);
Mat4 mat4_from_quat(Quat q);
Mat4 mat4_translate(Vec3 t);
Mat4 mat4_scale(Vec3 s);
Mat4 mat4_rotate(double angle, Vec3 axis);
Mat4 mat4_perspective(double fov, double aspect, double near, double far);
Mat4 mat4_lookat(Vec3 eye, Vec3 center, Vec3 up);
Mat4 mat4_transpose(Mat4 m);

#ifdef __cplusplus
}
#endif

static inline void _defer_cleanup_vec3(Vec3** vec) {
    vec3_cleanup(vec);
}

static inline void _defer_cleanup_quat(Quat** q) {
    if (q && *q) { free(*q); *q = NULL; }
}

#endif
