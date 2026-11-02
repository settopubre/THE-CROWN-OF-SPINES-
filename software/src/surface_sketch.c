#include "surface_sketch.h"
#include "shape_palette.h"
#include "extrude.h"
#include "bvh.h"
#include <stdlib.h>
#include <math.h>

SurfaceSketch* surface_sketch_create(void) {
    SurfaceSketch* ss = (SurfaceSketch*)calloc(1, sizeof(SurfaceSketch));
    if (!ss) return NULL;
    ss->mode = SKETCH_BUMP;
    return ss;
}

void surface_sketch_destroy(SurfaceSketch* ss) {
    free(ss);
}

int surface_sketch_pick_face(SurfaceSketch* ss, Mesh* mesh, Vec3 ray_origin, Vec3 ray_dir) {
    if (!ss || !mesh) return 0;
    
    /* Simplified: pick first triangle hit */
    for (uint32_t t = 0; t < mesh->index_count; t += 3) {
        uint32_t i0 = mesh->indices[t], i1 = mesh->indices[t+1], i2 = mesh->indices[t+2];
        float* v0 = &mesh->positions[i0*3];
        float* v1 = &mesh->positions[i1*3];
        float* v2 = &mesh->positions[i2*3];
        
        /* Möller-Trumbore ray-triangle intersection */
        float e1[3] = {v1[0]-v0[0], v1[1]-v0[1], v1[2]-v0[2]};
        float e2[3] = {v2[0]-v0[0], v2[1]-v0[1], v2[2]-v0[2]};
        float h[3] = {ray_dir.y*e2[2]-ray_dir.z*e2[1], ray_dir.z*e2[0]-ray_dir.x*e2[2], ray_dir.x*e2[1]-ray_dir.y*e2[0]};
        float a = e1[0]*h[0] + e1[1]*h[1] + e1[2]*h[2];
        if (fabsf(a) < 1e-8) continue;
        float f = 1.0f / a;
        float s[3] = {ray_origin.x-v0[0], ray_origin.y-v0[1], ray_origin.z-v0[2]};
        float u = f * (s[0]*h[0] + s[1]*h[1] + s[2]*h[2]);
        if (u < 0.0f || u > 1.0f) continue;
        float q[3] = {s[1]*e1[2]-s[2]*e1[1], s[2]*e1[0]-s[0]*e1[2], s[0]*e1[1]-s[1]*e1[0]};
        float v = f * (ray_dir.x*q[0] + ray_dir.y*q[1] + ray_dir.z*q[2]);
        if (v < 0.0f || u+v > 1.0f) continue;
        float dist = f * (e2[0]*q[0] + e2[1]*q[1] + e2[2]*q[2]);
        if (dist > 1e-4) {
            ss->face_index = t / 3;
            ss->face_center = vec3_create((v0[0]+v1[0]+v2[0])/3, (v0[1]+v1[1]+v2[1])/3, (v0[2]+v1[2]+v2[2])/3);
            /* Normal from cross product */
            float n[3] = {e1[1]*e2[2]-e1[2]*e2[1], e1[2]*e2[0]-e1[0]*e2[2], e1[0]*e2[1]-e1[1]*e2[0]};
            float len = sqrtf(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]);
            if (len > 1e-6) { n[0]/=len; n[1]/=len; n[2]/=len; }
            ss->face_normal = vec3_create(n[0], n[1], n[2]);
            return 1;
        }
    }
    return 0;
}

Mesh* surface_sketch_place_shape(SurfaceSketch* ss, ShapeType type, float w, float d, float depth) {
    if (!ss) return NULL;
    
    /* Create shape aligned to face plane */
    Shape* shape = shape_create(type, ss->face_center, w, d, 16);
    if (!shape) return NULL;
    
    /* Extrude along face normal */
    Extrude* ext = extrude_create(shape->mesh, depth, (ss->mode == SKETCH_BUMP) ? 1 : -1);
    Mesh* result = ext ? ext->extruded : NULL;
    
    /* Note: In full implementation, would transform result to face orientation */
    /* For now, return extruded mesh at face center */
    
    shape_destroy(shape);
    if (ext) { ext->extruded = NULL; extrude_destroy(ext); }
    
    return result;
}
