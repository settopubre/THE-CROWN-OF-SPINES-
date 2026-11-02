#ifndef INSTANCE_RENDERER_H
#define INSTANCE_RENDERER_H

#include "vector3d.h"
#include "mesh.h"

typedef struct InstanceRenderer InstanceRenderer;

#ifdef __cplusplus
extern "C" {
#endif

InstanceRenderer* instance_renderer_create(Mesh* mesh, int max_instances);
void instance_renderer_add_instance(InstanceRenderer* ir, Mat4 model_matrix, Vec3 color);
void instance_renderer_clear(InstanceRenderer* ir);
void instance_renderer_render(InstanceRenderer* ir, Mat4 view, Mat4 proj);
void instance_renderer_destroy(InstanceRenderer* ir);

#ifdef __cplusplus
}
#endif

#endif /* INSTANCE_RENDERER_H */
