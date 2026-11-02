#include "skeleton.h"
#include "kdtree.h"
#include "mesh.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static Vec3 _compute_centroid(Mesh* mesh) {
    Vec3 centroid = vec3_create(0.0, 0.0, 0.0);
    if (!mesh || mesh->vertex_count == 0) return centroid;
    for (uint32_t i = 0; i < mesh->vertex_count; i++) {
        centroid.x += mesh->positions[i*3];
        centroid.y += mesh->positions[i*3+1];
        centroid.z += mesh->positions[i*3+2];
    }
    double inv_n = 1.0 / mesh->vertex_count;
    centroid.x *= inv_n; centroid.y *= inv_n; centroid.z *= inv_n;
    return centroid;
}

static Vec3 _compute_principal_axis(Mesh* mesh, Vec3 centroid) {
    Vec3 axis = vec3_create(0.0, 0.0, 0.0);
    if (!mesh || mesh->vertex_count < 2) return axis;
    for (uint32_t i = 0; i < mesh->vertex_count; i++) {
        Vec3 v = vec3_create(mesh->positions[i*3], mesh->positions[i*3+1], mesh->positions[i*3+2]);
        Vec3 d = vec3_sub(v, centroid);
        axis.x += d.x * d.x; axis.y += d.y * d.y; axis.z += d.z * d.z;
    }
    if (axis.x >= axis.y && axis.x >= axis.z) return vec3_create(1.0, 0.0, 0.0);
    if (axis.y >= axis.x && axis.y >= axis.z) return vec3_create(0.0, 1.0, 0.0);
    return vec3_create(0.0, 0.0, 1.0);
}

Skeleton* autorig_generate(Mesh* mesh) {
    if (!mesh || mesh->vertex_count < 4) return NULL;
    
    Vec3 centroid = _compute_centroid(mesh);
    Vec3 up = _compute_principal_axis(mesh, centroid);
    
    Skeleton* skel = skeleton_create();
    if (!skel) return NULL;
    
    KDTree* kdt = kdtree_create(mesh->vertex_count);
    if (!kdt) { skeleton_destroy(skel); return NULL; }
    
    for (uint32_t i = 0; i < mesh->vertex_count; i++) {
        Vec3 v = vec3_create(mesh->positions[i*3], mesh->positions[i*3+1], mesh->positions[i*3+2]);
        kdtree_insert(kdt, v, i);
    }
    
    /* Create humanoid template skeleton */
    float spine_len = 0.3f, arm_len = 0.25f, leg_len = 0.35f;
    
    /* Root (hips) */
    uint32_t root = skeleton_add_bone(skel, UINT32_MAX, centroid, vec3_add(centroid, vec3_mul(up, spine_len * 0.3f)));
    
    /* Spine (2 segments) */
    Vec3 spine1 = vec3_add(centroid, vec3_mul(up, spine_len * 0.5f));
    Vec3 spine2 = vec3_add(centroid, vec3_mul(up, spine_len * 0.8f));
    uint32_t spine_bone = skeleton_add_bone(skel, root, centroid, spine1);
    uint32_t chest_bone = skeleton_add_bone(skel, spine_bone, spine1, spine2);
    
    /* Head */
    Vec3 head_pos = vec3_add(spine2, vec3_mul(up, 0.15f));
    skeleton_add_bone(skel, chest_bone, spine2, head_pos);
    
    /* Shoulders */
    Vec3 shoulder_offset = vec3_create(0.15f, 0.0, 0.0);
    Vec3 l_shoulder = vec3_add(spine2, shoulder_offset);
    Vec3 r_shoulder = vec3_sub(spine2, shoulder_offset);
    uint32_t l_shoulder_bone = skeleton_add_bone(skel, chest_bone, spine2, l_shoulder);
    uint32_t r_shoulder_bone = skeleton_add_bone(skel, chest_bone, spine2, r_shoulder);
    
    /* Arms (2 segments each) */
    Vec3 l_elbow = vec3_add(l_shoulder, vec3_create(0.0, -arm_len, 0.0));
    Vec3 l_hand = vec3_add(l_elbow, vec3_create(0.0, -arm_len, 0.0));
    skeleton_add_bone(skel, l_shoulder_bone, l_shoulder, l_elbow);
    skeleton_add_bone(skel, l_shoulder_bone, l_elbow, l_hand);
    
    Vec3 r_elbow = vec3_add(r_shoulder, vec3_create(0.0, -arm_len, 0.0));
    Vec3 r_hand = vec3_add(r_elbow, vec3_create(0.0, -arm_len, 0.0));
    skeleton_add_bone(skel, r_shoulder_bone, r_shoulder, r_elbow);
    skeleton_add_bone(skel, r_shoulder_bone, r_elbow, r_hand);
    
    /* Legs (2 segments each) */
    Vec3 l_hip = vec3_add(centroid, shoulder_offset);
    Vec3 r_hip = vec3_sub(centroid, shoulder_offset);
    Vec3 l_knee = vec3_add(l_hip, vec3_create(0.0, -leg_len, 0.0));
    Vec3 l_foot = vec3_add(l_knee, vec3_create(0.0, -leg_len, 0.0));
    skeleton_add_bone(skel, root, l_hip, l_knee);
    skeleton_add_bone(skel, root, l_knee, l_foot);
    
    Vec3 r_knee = vec3_add(r_hip, vec3_create(0.0, -leg_len, 0.0));
    Vec3 r_foot = vec3_add(r_knee, vec3_create(0.0, -leg_len, 0.0));
    skeleton_add_bone(skel, root, r_hip, r_knee);
    skeleton_add_bone(skel, root, r_knee, r_foot);
    
    kdtree_destroy(kdt);
    return skel;
}
