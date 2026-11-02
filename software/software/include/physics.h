#ifndef PHYSICS_H
#define PHYSICS_H

#include "vector3d.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PhysicsWorld PhysicsWorld;

PhysicsWorld* physics_create(void);
void physics_destroy(PhysicsWorld* pw);
void physics_step(PhysicsWorld* pw, float dt);
void physics_set_gravity(PhysicsWorld* pw, float x, float y, float z);

/* Rigid bodies */
int  physics_add_box(PhysicsWorld* pw, Vec3 pos, Vec3 half_extents, float mass);
int  physics_add_sphere(PhysicsWorld* pw, Vec3 pos, float radius, float mass);
int  physics_add_mesh(PhysicsWorld* pw, const float* verts, int vert_count, 
                      const int* indices, int tri_count, Vec3 pos, float mass);
void physics_remove_body(PhysicsWorld* pw, int body_id);

/* Get transforms back */
void physics_get_transform(PhysicsWorld* pw, int body_id, Vec3* pos, Quat* rot);

/* Raycast */
int physics_raycast(PhysicsWorld* pw, Vec3 from, Vec3 to, Vec3* hit_point, Vec3* hit_normal);

/* Ground plane */
void physics_add_ground(PhysicsWorld* pw);

#ifdef __cplusplus
}
#endif

#endif
