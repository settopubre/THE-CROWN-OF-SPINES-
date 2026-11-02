#include "skeleton.h"
#include "mesh.h"
#include "gl_functions.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Rest pose storage - allocated on first skinning call */
static float* g_rest_positions = NULL;
static uint32_t g_rest_vertex_count = 0;

/* Static deformation pool to avoid malloc per frame */
static float* deform_pool = NULL;
static uint32_t pool_size = 0;

static void _apply_lbs_vertex(float* out_pos, const float* rest_pos, 
                               const Mat4* bone_mats, const uint32_t* bone_indices, 
                               const float* bone_weights, uint32_t weight_count) {
    out_pos[0] = out_pos[1] = out_pos[2] = 0.0f;
    
    /* Fallback: if no weights provided, use single-bone assignment */
    if (weight_count == 0 || !bone_weights || !bone_indices) {
        /* Use first bone matrix as fallback */
        const Mat4* m = &bone_mats[0];
        double x = rest_pos[0], y = rest_pos[1], z = rest_pos[2];
        out_pos[0] = (float)(m->m[0]*x + m->m[1]*y + m->m[2]*z + m->m[3]);
        out_pos[1] = (float)(m->m[4]*x + m->m[5]*y + m->m[6]*z + m->m[7]);
        out_pos[2] = (float)(m->m[8]*x + m->m[9]*y + m->m[10]*z + m->m[11]);
        return;
    }
    
    for (uint32_t i = 0; i < weight_count; i++) {
        uint32_t bone_idx = bone_indices[i];
        float w = bone_weights[i];
        if (w < 0.001f) continue;
        const Mat4* m = &bone_mats[bone_idx];
        double x = rest_pos[0], y = rest_pos[1], z = rest_pos[2];
        out_pos[0] += w * (float)(m->m[0]*x + m->m[1]*y + m->m[2]*z + m->m[3]);
        out_pos[1] += w * (float)(m->m[4]*x + m->m[5]*y + m->m[6]*z + m->m[7]);
        out_pos[2] += w * (float)(m->m[8]*x + m->m[9]*y + m->m[10]*z + m->m[11]);
    }
}

/* Simple nearest-bone assignment for autorig fallback */
static uint32_t _find_nearest_bone(Skeleton* skel, Vec3 pos) {
    if (!skel || skel->bone_count == 0) return 0;
    
    uint32_t best = 0;
    double best_dist = 1e30;
    
    for (uint32_t i = 0; i < skel->bone_count; i++) {
        Bone* bone = skeleton_get_bone(skel, i);
        if (!bone) continue;
        SceneNode* node = &skel->graph->nodes[bone->node_idx];
        double d = vec3_distance(&pos, &node->local_position);
        if (d < best_dist) {
            best_dist = d;
            best = i;
        }
    }
    return best;
}

void skinning_apply(Mesh* mesh, Skeleton* skel) {
    if (!mesh || !skel || mesh->vertex_count == 0) return;
    
    uint32_t bone_count = skeleton_get_bone_count(skel);
    if (bone_count == 0) return;
    
    skeleton_update_transforms(skel);
    Mat4* bone_mats = skeleton_get_bone_matrices(skel);
    
    /* Allocate rest pose on first call */
    if (!g_rest_positions || g_rest_vertex_count != mesh->vertex_count) {
        free(g_rest_positions);
        g_rest_positions = (float*)malloc(mesh->vertex_count * 3 * sizeof(float));
        if (!g_rest_positions) return;
        memcpy(g_rest_positions, mesh->positions, mesh->vertex_count * 3 * sizeof(float));
        g_rest_vertex_count = mesh->vertex_count;
    }
    
    /* Use static pool instead of malloc */
    uint32_t needed = mesh->vertex_count * 3 * sizeof(float);
    if (pool_size < needed) {
        deform_pool = (float*)realloc(deform_pool, needed);
        if (!deform_pool) return;
        pool_size = needed;
    }
    float* deformed = deform_pool;
    
    /* Simple fallback: assign each vertex to nearest bone with weight 1.0 */
    for (uint32_t i = 0; i < mesh->vertex_count; i++) {
        float rest_arr[3] = {
            g_rest_positions[i*3],
            g_rest_positions[i*3+1],
            g_rest_positions[i*3+2]
        };
        Vec3 rest = vec3_create(rest_arr[0], rest_arr[1], rest_arr[2]);
        uint32_t bone_idx = _find_nearest_bone(skel, rest);
        
        float out[3];
        _apply_lbs_vertex(out, rest_arr, bone_mats, &bone_idx, NULL, 0);
        deformed[i*3] = out[0];
        deformed[i*3+1] = out[1];
        deformed[i*3+2] = out[2];
    }
    
    /* Update VBO with deformed positions */
    if (mesh->uploaded) {
        glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo_pos);
        glBufferData(GL_ARRAY_BUFFER, mesh->vertex_count * 3 * sizeof(float), deformed, GL_DYNAMIC_DRAW);
    }
    
    /* Removed free(deformed); buffer persists */
}
