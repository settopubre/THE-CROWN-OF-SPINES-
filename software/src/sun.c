#include "sun.h"
#include "gl_functions.h"
#include "vector3d.h"
#include <stdlib.h>
#include <math.h>
#include <GL/gl.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct Sun {
    Vec3 direction;
    float intensity;
    Vec3 color;
    float time_of_day; /* 0.0 - 24.0 */
    Mat4 light_space_matrix;
};

/* Helper: Build orthographic projection for shadow mapping */
static Mat4 _mat4_ortho(float l, float r, float b, float t, float n, float f) {
    Mat4 m = {0};
    m.m[0] = 2.0f / (r - l);
    m.m[5] = 2.0f / (t - b);
    m.m[10] = -2.0f / (f - n);
    m.m[12] = -(r + l) / (r - l);
    m.m[13] = -(t + b) / (t - b);
    m.m[14] = -(f + n) / (f - n);
    m.m[15] = 1.0f;
    return m;
}

Sun* sun_create(Vec3 direction, float intensity, Vec3 color) {
    Sun* s = (Sun*)calloc(1, sizeof(Sun));
    if (!s) return NULL;
    s->direction = direction;
    s->intensity = intensity;
    s->color = color;
    s->time_of_day = 12.0f; /* Default noon */
    
    /* Initialize default light matrix */
    Vec3 pos = vec3_create(s->direction.x * 10.0f, s->direction.y * 10.0f, s->direction.z * 10.0f);
    Vec3 up = vec3_create(0.0f, 1.0f, 0.0f);
    Mat4 view = mat4_lookat(pos, vec3_create(0.0f, 0.0f, 0.0f), up);
    Mat4 proj = _mat4_ortho(-10.0f, 10.0f, -10.0f, 10.0f, -20.0f, 30.0f);
    s->light_space_matrix = mat4_mul(proj, view);
    
    return s;
}

void sun_destroy(Sun* s) {
    free(s);
}

void sun_update(Sun* s, float time_of_day) {
    if (!s) return;
    s->time_of_day = fmodf(time_of_day, 24.0f);
    if (s->time_of_day < 0.0f) s->time_of_day += 24.0f;
    
    /* Map time to sun arc: 6am = sunrise (east), 12pm = zenith, 18pm = sunset (west) */
    float angle = ((s->time_of_day - 6.0f) / 12.0f) * M_PI;
    float x = cosf(angle);
    float y = sinf(angle);
    float z = 0.3f; /* Slight south offset */
    
    Vec3 dir = vec3_normalize(vec3_create(x, y, z));
    s->direction = dir;
    
    /* Update light space matrix */
    Vec3 light_pos = vec3_create(dir.x * 15.0f, dir.y * 15.0f, dir.z * 15.0f);
    Vec3 target = vec3_create(0.0f, 0.0f, 0.0f);
    Vec3 up = vec3_create(0.0f, 1.0f, 0.0f);
    if (y < 0.0f) up = vec3_create(0.0f, -1.0f, 0.0f); /* Flip when sun is below horizon */
    
    Mat4 view = mat4_lookat(light_pos, target, up);
    Mat4 proj = _mat4_ortho(-12.0f, 12.0f, -12.0f, 12.0f, -25.0f, 35.0f);
    s->light_space_matrix = mat4_mul(proj, view);
}

void sun_set_intensity(Sun* s, float intensity) {
    if (s) s->intensity = fmaxf(0.0f, fminf(intensity, 5.0f));
}

Mat4 sun_get_light_matrix(Sun* s) {
    if (!s) return mat4_identity();
    return s->light_space_matrix;
}

void sun_apply_to_shader(Sun* s, uint32_t shader) {
    if (!s) return;
    glUseProgram(shader);
    
    GLint loc_dir = glGetUniformLocation(shader, "uSunDirection");
    if (loc_dir >= 0) glUniform3f(loc_dir, s->direction.x, s->direction.y, s->direction.z);
    
    GLint loc_int = glGetUniformLocation(shader, "uSunIntensity");
    if (loc_int >= 0) glUniform1f(loc_int, s->intensity);
    
    GLint loc_col = glGetUniformLocation(shader, "uSunColor");
    if (loc_col >= 0) glUniform3f(loc_col, s->color.x, s->color.y, s->color.z);
    
    GLint loc_mat = glGetUniformLocation(shader, "uLightSpaceMatrix");
    if (loc_mat >= 0) {
        float mat_arr[16];
        for (int i = 0; i < 16; i++) mat_arr[i] = (float)s->light_space_matrix.m[i];
        glUniformMatrix4fv(loc_mat, 1, GL_FALSE, mat_arr);
    }
}
