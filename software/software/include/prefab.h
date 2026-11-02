#ifndef PREFAB_H
#define PREFAB_H

#include "vector3d.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Prefab Prefab;
typedef struct PrefabInstance PrefabInstance;

Prefab* prefab_load(const char* filepath);
void prefab_unload(Prefab* p);
int prefab_get_ref_count(Prefab* p);

PrefabInstance* prefab_instantiate(Prefab* p, Vec3 position, Quat rotation, Vec3 scale);
void prefab_instance_destroy(PrefabInstance* inst);
void prefab_instance_render(PrefabInstance* inst, Mat4 view, Mat4 proj);

#ifdef __cplusplus
}
#endif

#endif /* PREFAB_H */
