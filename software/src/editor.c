#include "editor.h"
#include "asset_importer.h"
#include "gl_functions.h"
#include "asset_db.h"
#include "bvh.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

extern BVH* g_bvh;

static Vec3 _snap(Vec3 v, float grid) {
    if (grid <= 0.0f) return v;
    return vec3_create(
        roundf(v.x / grid) * grid,
        roundf(v.y / grid) * grid,
        roundf(v.z / grid) * grid
    );
}

void editor_snap_to_surface(Editor* ed, Vec3* position) {
    if (!ed || !position) return;
    
    if (g_bvh && g_bvh->root != UINT32_MAX && g_bvh->src_verts && g_bvh->src_inds) {
        Vec3 ray_origin = {position->x, position->y + 50.0f, position->z};
        Vec3 ray_dir = {0.0f, -1.0f, 0.0f};
        float t;
        uint32_t tri_idx;
        
        if (bvh_raycast(g_bvh, g_bvh->src_verts, g_bvh->src_inds, ray_origin, ray_dir, &t, &tri_idx)) {
            position->y = ray_origin.y + ray_dir.y * t;
            return;
        }
    }
    
    *position = _snap(*position, ed->grid_size);
}

Editor* editor_create(AssetDB* db) {
    Editor* ed = (Editor*)calloc(1, sizeof(Editor));
    if (!ed) return NULL;
    ed->db = db;
    ed->grid_size = 1.0f;
    ed->snap_to_grid = 1;
    ed->object_capacity = 64;
    ed->placed_objects = (AssetInstance**)calloc(ed->object_capacity, sizeof(AssetInstance*));
    ed->selected_asset_id = UINT32_MAX;
    return ed;
}

void editor_destroy(Editor* ed) {
    if (!ed) return;
    for (uint32_t i = 0; i < ed->object_count; i++) {
        if (ed->placed_objects[i]) asset_instance_destroy(ed->placed_objects[i]);
    }
    free(ed->placed_objects);
    if (ed->current_scene) scene_destroy(ed->current_scene);
    free(ed);
}

void editor_set_mode(Editor* ed, EditorMode mode) {
    if (ed) ed->mode = mode;
}

void editor_place_asset(Editor* ed, uint32_t asset_id, Vec3 position) {
    if (!ed || !ed->db) return;
    const AssetEntry* entry = asset_db_get(ed->db, asset_id);
    if (!entry) return;

    editor_snap_to_surface(ed, &position);

    AssetInstance* inst = asset_importer_load(entry->path);
    if (!inst) return;
    inst->asset_id = asset_id;
    inst->position = position;

    if (ed->object_count >= ed->object_capacity) {
        ed->object_capacity *= 2;
        ed->placed_objects = realloc(ed->placed_objects, ed->object_capacity * sizeof(AssetInstance*));
    }
    ed->placed_objects[ed->object_count++] = inst;
}

void editor_select_at(Editor* ed, Vec3 ray_origin, Vec3 ray_dir) {
    (void)ray_origin; (void)ray_dir;
    if (ed->object_count > 0) {
        ed->selected_asset_id = ed->object_count - 1; 
    }
}

void editor_move_selected(Editor* ed, Vec3 delta) {
    if (!ed || ed->selected_asset_id == UINT32_MAX) return;
    AssetInstance* inst = ed->placed_objects[ed->selected_asset_id];
    if (inst) {
        inst->position = vec3_add(inst->position, delta);
        if (ed->snap_to_grid) inst->position = _snap(inst->position, ed->grid_size);
    }
}

void editor_rotate_selected(Editor* ed, Quat rotation) {
    if (!ed || ed->selected_asset_id == UINT32_MAX) return;
    AssetInstance* inst = ed->placed_objects[ed->selected_asset_id];
    if (inst) inst->rotation = quat_mul(inst->rotation, rotation);
}

void editor_scale_selected(Editor* ed, Vec3 scale) {
    if (!ed || ed->selected_asset_id == UINT32_MAX) return;
    AssetInstance* inst = ed->placed_objects[ed->selected_asset_id];
    if (inst) inst->scale = vec3_mul(inst->scale, 1.0f + scale.x);
}

void editor_delete_selected(Editor* ed) {
    if (!ed || ed->selected_asset_id == UINT32_MAX) return;
    uint32_t idx = ed->selected_asset_id;
    asset_instance_destroy(ed->placed_objects[idx]);
    for (uint32_t i = idx; i < ed->object_count - 1; i++) {
        ed->placed_objects[i] = ed->placed_objects[i+1];
    }
    ed->object_count--;
    ed->placed_objects[ed->object_count] = NULL;
    ed->selected_asset_id = UINT32_MAX;
}

void editor_render(Editor* ed, Mat4 view, Mat4 proj) {
    if (!ed) return;
    for (uint32_t i = 0; i < ed->object_count; i++) {
        AssetInstance* inst = ed->placed_objects[i];
        if (inst) {
            asset_instance_render(inst, view, proj);
        }
    }
}

void editor_save_scene(Editor* ed, const char* filename) {
    if (!ed || !filename) return;
    Scene* s = scene_create(ed->object_count);
    for (uint32_t i = 0; i < ed->object_count; i++) {
        AssetInstance* inst = ed->placed_objects[i];
        if (!inst) continue;
        SceneObject obj;
        const AssetEntry* entry = asset_db_get(ed->db, inst->asset_id);
        if (entry) strncpy(obj.asset_name, entry->name, sizeof(obj.asset_name)-1);
        else obj.asset_name[0] = '\0';
        obj.position = inst->position;
        obj.rotation = inst->rotation;
        obj.scale = inst->scale;
        scene_add_object(s, &obj);
    }
    scene_save(s, filename);
    scene_destroy(s);
}

void editor_load_scene(Editor* ed, const char* filename) {
    if (!ed || !filename) return;
    Scene* s = scene_load(filename);
    if (!s) return;
    
    for (uint32_t i = 0; i < ed->object_count; i++) asset_instance_destroy(ed->placed_objects[i]);
    ed->object_count = 0;

    for (uint32_t i = 0; i < s->count; i++) {
        SceneObject* obj = &s->objects[i];
        if (ed->db) {
            const AssetEntry* entry = asset_db_find_by_name(ed->db, obj->asset_name);
            if (entry) {
                if (ed->object_count >= ed->object_capacity) {
                    ed->object_capacity *= 2;
                    ed->placed_objects = realloc(ed->placed_objects, ed->object_capacity * sizeof(AssetInstance*));
                }
                AssetInstance* inst = asset_importer_load(entry->path);
                if (inst) {
                    inst->asset_id = entry->id;
                    inst->position = obj->position;
                    inst->rotation = obj->rotation;
                    inst->scale = obj->scale;
                    ed->placed_objects[ed->object_count++] = inst;
                }
            }
        }
    }
    scene_destroy(s);
}
