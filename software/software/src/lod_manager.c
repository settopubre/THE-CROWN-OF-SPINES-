#include "lod_manager.h"
#include "mesh.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t id;
    Mesh** meshes;
    float* thresholds;
    int level_count;
    int ref_count;
    bool is_valid;
} LODGroup;

/* FIX: Explicit typedef so we can use 'LODManager' without the 'struct' keyword */
typedef struct LODManager LODManager;

struct LODManager {
    LODGroup* groups;
    int group_count;
    int group_capacity;
    float global_max_distance;
    int global_levels;
};

LODManager* lod_create(float max_distance, int levels) {
    LODManager* lm = (LODManager*)calloc(1, sizeof(LODManager));
    if (!lm) return NULL;
    lm->global_max_distance = max_distance;
    lm->global_levels = levels > 1 ? levels : 1;
    lm->group_capacity = 16;
    lm->groups = (LODGroup*)calloc(lm->group_capacity, sizeof(LODGroup));
    return lm;
}

void lod_register_mesh(LODManager* lm, uint32_t id, Mesh** meshes, int mesh_count, float* distances) {
    if (!lm || !meshes || !distances || mesh_count <= 0) return;
    
    for(int i=0; i<lm->group_count; i++) {
        if (lm->groups[i].id == id && lm->groups[i].is_valid) return;
    }

    if (lm->group_count >= lm->group_capacity) {
        lm->group_capacity *= 2;
        lm->groups = (LODGroup*)realloc(lm->groups, lm->group_capacity * sizeof(LODGroup));
        memset(&lm->groups[lm->group_count], 0, lm->group_count * sizeof(LODGroup));
    }
    
    LODGroup* g = &lm->groups[lm->group_count];
    g->id = id;
    g->level_count = mesh_count;
    g->ref_count = 0;
    g->is_valid = true;
    g->meshes = (Mesh**)malloc(mesh_count * sizeof(Mesh*));
    g->thresholds = (float*)malloc(mesh_count * sizeof(float));
    
    memcpy(g->meshes, meshes, mesh_count * sizeof(Mesh*));
    memcpy(g->thresholds, distances, mesh_count * sizeof(float));
    
    for (int i = 0; i < mesh_count - 1; i++) {
        for (int j = 0; j < mesh_count - i - 1; j++) {
            if (g->thresholds[j] > g->thresholds[j+1]) {
                float tmp_f = g->thresholds[j]; g->thresholds[j] = g->thresholds[j+1]; g->thresholds[j+1] = tmp_f;
                Mesh* tmp_m = g->meshes[j]; g->meshes[j] = g->meshes[j+1]; g->meshes[j+1] = tmp_m;
            }
        }
    }
    lm->group_count++;
}

void lod_unregister(LODManager* lm, uint32_t id) {
    if (!lm) return;
    
    for (int i = 0; i < lm->group_count; i++) {
        if (lm->groups[i].is_valid && lm->groups[i].id == id) {
            if (lm->groups[i].meshes) {
                for(int k=0; k<lm->groups[i].level_count; k++) {
                    if(lm->groups[i].meshes[k]) mesh_destroy(lm->groups[i].meshes[k]);
                }
                free(lm->groups[i].meshes);
            }
            free(lm->groups[i].thresholds);
            lm->groups[i].is_valid = false;
            break;
        }
    }
}

void lod_cleanup_unused(LODManager* lm) {
    if (!lm) return;
    int write_idx = 0;
    for (int i = 0; i < lm->group_count; i++) {
        if (lm->groups[i].is_valid) {
            if (lm->groups[i].ref_count > 0) {
                if (write_idx != i) lm->groups[write_idx] = lm->groups[i];
                write_idx++;
            } else {
                lod_unregister(lm, lm->groups[i].id);
            }
        }
    }
}

Mesh* lod_get_mesh(LODManager* lm, uint32_t id, float distance) {
    if (!lm) return NULL;
    
    for (int i = 0; i < lm->group_count; i++) {
        if (lm->groups[i].id == id && lm->groups[i].is_valid) {
            lm->groups[i].ref_count++;
            LODGroup* g = &lm->groups[i];
            for (int j = 0; j < g->level_count; j++) {
                if (distance <= g->thresholds[j]) return g->meshes[j];
            }
            return g->meshes[g->level_count - 1];
        }
    }
    return NULL;
}

void lod_destroy(LODManager* lm) {
    if (!lm) return;
    for (int i = 0; i < lm->group_capacity; i++) {
        if (lm->groups[i].meshes) {
            for(int k=0; k<lm->groups[i].level_count; k++) {
                if(lm->groups[i].meshes[k]) mesh_destroy(lm->groups[i].meshes[k]);
            }
            free(lm->groups[i].meshes);
        }
        free(lm->groups[i].thresholds);
    }
    free(lm->groups);
    free(lm);
}
