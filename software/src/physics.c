#include "physics.h"
#include <bullet/btBulletDynamicsCommon.h>
#include <bullet/btBulletCollisionCommon.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BODIES 1024

struct PhysicsWorld {
    btDefaultCollisionConfiguration* config;
    btCollisionDispatcher* dispatcher;
    btBroadphaseInterface* broadphase;
    btSequentialImpulseConstraintSolver* solver;
    btDiscreteDynamicsWorld* world;
    
    btCollisionShape* shapes[MAX_BODIES];
    btRigidBody* bodies[MAX_BODIES];
    int body_count;
};

PhysicsWorld* physics_create(void) {
    PhysicsWorld* pw = (PhysicsWorld*)calloc(1, sizeof(PhysicsWorld));
    if (!pw) return NULL;
    
    pw->config = new btDefaultCollisionConfiguration();
    pw->dispatcher = new btCollisionDispatcher(pw->config);
    pw->broadphase = new btDbvtBroadphase();
    pw->solver = new btSequentialImpulseConstraintSolver();
    pw->world = new btDiscreteDynamicsWorld(pw->dispatcher, pw->broadphase, 
                                              pw->solver, pw->config);
    pw->world->setGravity(btVector3(0, -9.81, 0));
    pw->body_count = 0;
    
    printf("[PHYSICS] Bullet physics world created\n");
    return pw;
}

void physics_destroy(PhysicsWorld* pw) {
    if (!pw) return;
    for (int i = 0; i < pw->body_count; i++) {
        pw->world->removeRigidBody(pw->bodies[i]);
        delete pw->bodies[i]->getMotionState();
        delete pw->bodies[i];
        delete pw->shapes[i];
    }
    delete pw->world;
    delete pw->solver;
    delete pw->broadphase;
    delete pw->dispatcher;
    delete pw->config;
    free(pw);
}

void physics_step(PhysicsWorld* pw, float dt) {
    if (pw) pw->world->stepSimulation(dt, 10, 1.0f/60.0f);
}

void physics_set_gravity(PhysicsWorld* pw, float x, float y, float z) {
    if (pw) pw->world->setGravity(btVector3(x, y, z));
}

static int _add_body(PhysicsWorld* pw, btCollisionShape* shape, btRigidBody* body) {
    if (pw->body_count >= MAX_BODIES) return -1;
    int id = pw->body_count;
    pw->shapes[id] = shape;
    pw->bodies[id] = body;
    pw->body_count++;
    return id;
}

int physics_add_box(PhysicsWorld* pw, Vec3 pos, Vec3 half_extents, float mass) {
    if (!pw) return -1;
    btCollisionShape* shape = new btBoxShape(btVector3(half_extents.x, half_extents.y, half_extents.z));
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(pos.x, pos.y, pos.z));
    
    btVector3 localInertia(0, 0, 0);
    if (mass > 0) shape->calculateLocalInertia(mass, localInertia);
    
    btDefaultMotionState* motionState = new btDefaultMotionState(transform);
    btRigidBody::btRigidBodyConstructionInfo ci(mass, motionState, shape, localInertia);
    btRigidBody* body = new btRigidBody(ci);
    pw->world->addRigidBody(body);
    
    return _add_body(pw, shape, body);
}

int physics_add_sphere(PhysicsWorld* pw, Vec3 pos, float radius, float mass) {
    if (!pw) return -1;
    btCollisionShape* shape = new btSphereShape(radius);
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(pos.x, pos.y, pos.z));
    
    btVector3 localInertia(0, 0, 0);
    if (mass > 0) shape->calculateLocalInertia(mass, localInertia);
    
    btDefaultMotionState* motionState = new btDefaultMotionState(transform);
    btRigidBody::btRigidBodyConstructionInfo ci(mass, motionState, shape, localInertia);
    btRigidBody* body = new btRigidBody(ci);
    pw->world->addRigidBody(body);
    
    return _add_body(pw, shape, body);
}

int physics_add_mesh(PhysicsWorld* pw, const float* vertices, const uint32_t* indices, 
                     uint32_t tri_count, Vec3 pos, float mass) {
    if (!pw || !vertices || !indices || tri_count == 0) return -1;
    if (pw->body_count >= MAX_BODIES) return -1;

    btTriangleMesh* triMesh = new btTriangleMesh();
    for (uint32_t i = 0; i < tri_count; i++) {
        uint32_t i0 = indices[i * 3];
        uint32_t i1 = indices[i * 3 + 1];
        uint32_t i2 = indices[i * 3 + 2];
        
        btVector3 v0(vertices[i0*3], vertices[i0*3+1], vertices[i0*3+2]);
        btVector3 v1(vertices[i1*3], vertices[i1*3+1], vertices[i1*3+2]);
        btVector3 v2(vertices[i2*3], vertices[i2*3+1], vertices[i2*3+2]);
        
        triMesh->addTriangle(v0, v1, v2);
    }

    btCollisionShape* shape = new btBvhTriangleMeshShape(triMesh, true, true);
    
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(pos.x, pos.y, pos.z));
    
    btVector3 localInertia(0, 0, 0);
    if (mass > 0) shape->calculateLocalInertia(mass, localInertia);
    
    btDefaultMotionState* motionState = new btDefaultMotionState(transform);
    btRigidBody::btRigidBodyConstructionInfo ci(mass, motionState, shape, localInertia);
    btRigidBody* body = new btRigidBody(ci);
    pw->world->addRigidBody(body);
    
    return _add_body(pw, shape, body);
}

void physics_remove_body(PhysicsWorld* pw, int body_id) {
    if (!pw || body_id < 0 || body_id >= pw->body_count) return;
    pw->world->removeRigidBody(pw->bodies[body_id]);
    delete pw->bodies[body_id]->getMotionState();
    delete pw->bodies[body_id];
    delete pw->shapes[body_id];
}

void physics_get_transform(PhysicsWorld* pw, int body_id, Vec3* pos, Quat* rot) {
    if (!pw || body_id < 0 || body_id >= pw->body_count) return;
    btTransform trans;
    pw->bodies[body_id]->getMotionState()->getWorldTransform(trans);
    btVector3 p = trans.getOrigin();
    btQuaternion q = trans.getRotation();
    *pos = vec3_create(p.x(), p.y(), p.z());
    *rot = (Quat){q.w(), q.x(), q.y(), q.z()};
}

int physics_raycast(PhysicsWorld* pw, Vec3 from, Vec3 to, Vec3* hit_point, Vec3* hit_normal) {
    if (!pw) return 0;
    btVector3 bfrom(from.x, from.y, from.z);
    btVector3 bto(to.x, to.y, to.z);
    btCollisionWorld::ClosestRayResultCallback callback(bfrom, bto);
    pw->world->rayTest(bfrom, bto, callback);
    
    if (callback.hasHit()) {
        btVector3 hp = callback.m_hitPointWorld;
        btVector3 hn = callback.m_hitNormalWorld;
        *hit_point = vec3_create(hp.x(), hp.y(), hp.z());
        *hit_normal = vec3_create(hn.x(), hn.y(), hn.z());
        return 1;
    }
    return 0;
}

void physics_add_ground(PhysicsWorld* pw) {
    if (!pw) return;
    btCollisionShape* shape = new btStaticPlaneShape(btVector3(0, 1, 0), 0);
    btTransform transform;
    transform.setIdentity();
    btDefaultMotionState* motionState = new btDefaultMotionState(transform);
    btRigidBody::btRigidBodyConstructionInfo ci(0, motionState, shape);
    btRigidBody* body = new btRigidBody(ci);
    pw->world->addRigidBody(body);
    _add_body(pw, shape, body);
    printf("[PHYSICS] Ground plane added\n");
}
