#ifndef SKYBOX_H
#define SKYBOX_H

#include "vector3d.h"

typedef struct Skybox Skybox;

#ifdef __cplusplus
extern "C" {
#endif

Skybox* skybox_create(const char* cubemap_dir);
void skybox_render(Skybox* sb, Mat4 view, Mat4 proj);
void skybox_destroy(Skybox* sb);

#ifdef __cplusplus
}
#endif

#endif /* SKYBOX_H */
