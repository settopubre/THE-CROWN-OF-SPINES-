#ifndef SUN_H
#define SUN_H

#include "vector3d.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Sun Sun;

Sun* sun_create(Vec3 direction, float intensity, Vec3 color);
void sun_update(Sun* s, float time_of_day);
void sun_set_intensity(Sun* s, float intensity);
Mat4 sun_get_light_matrix(Sun* s);
void sun_apply_to_shader(Sun* s, uint32_t shader);
void sun_destroy(Sun* s);

#ifdef __cplusplus
}
#endif

#endif /* SUN_H */
