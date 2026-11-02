#include "prefab.h"
#include "mesh.h"
#include "gl_functions.h"
#include "render_server.h"
#include <stdlib.h>
#include <string.h>

struct Prefab {
    char filepath[256];
    Mesh* mesh;
    int ref_count;
};

struct PrefabInstance {
    Prefab* base;
    Vec3 position;
    Quat rotation;
    Vec3 scale;
};

/* Phase 14: Getter for debug purposes */
int prefab_get_ref_count(Prefab* p) {
    return p ? p->ref_count : 0;
}

Prefab* prefab_load(const char* filepath) {
    if (!filepath) return NULL;
    Prefab* p = (Prefab*)calloc(1, sizeof(Prefab));
    if (!p) return NULL;
    
    strncpy(p->filepath, filepath, sizeof(p->filepath) - 1);
    p->mesh = mesh_load_obj(filepath);
    if (!p->mesh) { free(p); return NULL; }
    p->ref_count = 1;
    return p;
}

void prefab_unload(Prefab* p) {
    if (!p) return;
    p->ref_count--;
    if (p->ref_count <= 0) {
        if (p->mesh) mesh_destroy(p->mesh);
        free(p);
    }
}

PrefabInstance* prefab_instantiate(Prefab* p, Vec3 position, Quat rotation, Vec3 scale) {
    if (!p) return NULL;
    PrefabInstance* inst = (PrefabInstance*)calloc(1, sizeof(PrefabInstance));
    if (!inst) return NULL;
    
    inst->base = p;
    p->ref_count++;
    inst->position = position;
    inst->rotation = rotation;
    inst->scale = scale;
    return inst;
}

void prefab_instance_destroy(PrefabInstance* inst) {
    if (!inst) return;
    if (inst->base) prefab_unload(inst->base);
    free(inst);
}

void prefab_instance_render(PrefabInstance* inst, Mat4 view, Mat4 proj) {
    if (!inst || !inst->base || !inst->base->mesh) return;
    
    Mat4 T = mat4_translate(inst->position);
    Mat4 R = mat4_from_quat(inst->rotation);
    Mat4 S = mat4_scale(inst->scale);
    Mat4 model = mat4_mul(T, mat4_mul(R, S));
    
    render_server_draw_mesh(inst->base->mesh, model);
}
