#ifndef EDITOR_H
#define EDITOR_H
#pragma once

#include "vector3d.h"
#include "asset_db.h"
#include "scene_serializer.h"
#include <stdint.h>

typedef enum {
    EDIT_MODE_SELECT,
    EDIT_MODE_PLACE,
    EDIT_MODE_MOVE,
    EDIT_MODE_ROTATE,
    EDIT_MODE_SCALE,
    EDIT_MODE_PAINT
} EditorMode;

typedef struct {
    EditorMode mode;
    uint32_t selected_asset_id;
    AssetInstance** placed_objects;
    uint32_t object_count;
    uint32_t object_capacity;
    Scene* current_scene;
    AssetDB* db;
    int snap_to_grid;
    float grid_size;
} Editor;

#ifdef __cplusplus
extern "C" {
#endif

Editor* editor_create(AssetDB* db);
void editor_destroy(Editor* ed);
void editor_set_mode(Editor* ed, EditorMode mode);
void editor_place_asset(Editor* ed, uint32_t asset_id, Vec3 position);
void editor_select_at(Editor* ed, Vec3 ray_origin, Vec3 ray_dir);
void editor_move_selected(Editor* ed, Vec3 delta);
void editor_rotate_selected(Editor* ed, Quat rotation);
void editor_scale_selected(Editor* ed, Vec3 scale);
void editor_delete_selected(Editor* ed);
void editor_render(Editor* ed, Mat4 view, Mat4 proj);
void editor_save_scene(Editor* ed, const char* filename);
void editor_load_scene(Editor* ed, const char* filename);

#ifdef __cplusplus
}
#endif

#endif /* EDITOR_H */
