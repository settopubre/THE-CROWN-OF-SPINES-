#ifndef SCENE_SERIALIZER_H
#define SCENE_SERIALIZER_H
#pragma once

#include "asset_importer.h"
#include <stdint.h>

typedef struct {
    char asset_name[128];
    Vec3 position;
    Quat rotation;
    Vec3 scale;
} SceneObject;

typedef struct {
    SceneObject* objects;
    uint32_t count;
    uint32_t capacity;
    char terrain_file[256];
} Scene;

#ifdef __cplusplus
extern "C" {
#endif

Scene* scene_create(uint32_t capacity);
void scene_destroy(Scene* scene);
void scene_add_object(Scene* scene, const SceneObject* obj);
int scene_save(const Scene* scene, const char* filename);
Scene* scene_load(const char* filename);

#ifdef __cplusplus
}
#endif

#endif /* SCENE_SERIALIZER_H */
