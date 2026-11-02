#ifndef ENGINE_ABI_H
#define ENGINE_ABI_H

#include <stdint.h>
#include "vector3d.h"
#include "mesh.h"
#include "skeleton.h"
#include "thermal.h"

/* Forward Declarations */
typedef struct Camera Camera;
typedef struct Gizmo Gizmo;
typedef struct Timer Timer;
typedef struct KDTree KDTree;
typedef struct CommandBuffer CommandBuffer;
typedef struct Node Node;
/* RWorker is opaque — no typedef in header, use struct RWorker* */

#ifdef __cplusplus
extern "C" {
#endif

/* Camera */
Camera* camera_create_orbit(float distance);
void camera_destroy(Camera* cam);
void camera_update(Camera* cam, float delta_time);
void camera_mouse_drag(Camera* cam, float dx, float dy);
void camera_mouse_scroll(Camera* cam, float delta);
Mat4 camera_get_view_matrix(const Camera* cam);
Mat4 camera_get_projection_matrix(const Camera* cam, float aspect);
void camera_set_target(Camera* cam, Vec3 target);
void camera_set_distance(Camera* cam, float distance);
void camera_set_position(Camera* cam, Vec3 pos);
int camera_get_background_index(const Camera* cam);
void camera_cycle_background(Camera* cam);

/* Gizmo */
Gizmo* gizmo_create(void);
void gizmo_destroy(Gizmo* gizmo);
void gizmo_render(Gizmo* gizmo, Mat4 view, Mat4 projection);
void gizmo_set_position(Gizmo* gizmo, Vec3 pos);
Vec3 gizmo_get_position(const Gizmo* gizmo);

/* Timer */
Timer* engine_timer_create(void);
void engine_timer_destroy(Timer* timer);
void engine_timer_tick(Timer* timer);
double engine_timer_get_fps(const Timer* timer);
double engine_timer_get_delta_time(const Timer* timer);
double engine_timer_get_frame_time(const Timer* timer);
void engine_timer_pause(Timer* t);
void engine_timer_resume(Timer* t);
void engine_timer_reset(Timer* t);

/* KDTree */
KDTree* kdtree_create(uint32_t capacity);
void kdtree_destroy(KDTree* tree);
void kdtree_insert(KDTree* tree, Vec3 point, uint32_t data_id);
uint32_t kdtree_nearest(KDTree* tree, Vec3 query);
void kdtree_build(KDTree* tree);

/* Auto-rig */
Skeleton* autorig_generate(Mesh* mesh);

/* IK Solver */
void ik_solve_fabrik(Skeleton* skel, uint32_t end_effector_bone, Vec3 target, int max_iterations, float tolerance);

/* Skinning */
void skinning_apply(Mesh* mesh, Skeleton* skel);

/* Command buffer */
CommandBuffer* cmd_buffer_create(int capacity);
void cmd_buffer_destroy(CommandBuffer* cb);
void cmd_buffer_undo(CommandBuffer* cb);
void cmd_buffer_redo(CommandBuffer* cb);
void cmd_buffer_set_target(CommandBuffer* cb, Node* node);
int cmd_buffer_is_dirty(CommandBuffer* cb);
void cmd_buffer_clear(CommandBuffer* cb);
void cmd_buffer_begin_group(CommandBuffer* cb);
void cmd_buffer_end_group(CommandBuffer* cb);

/* Standard R Worker — opaque struct RWorker */
struct RWorker* r_worker_start(void);
void r_worker_stop(struct RWorker* rw);
int r_worker_send(struct RWorker* rw, const char* cmd, const char* data);
char* r_worker_recv(struct RWorker* rw);
int r_worker_is_alive(struct RWorker* rw);
void r_worker_set_timeout(struct RWorker* rw, int timeout_ms);

/* Statistical R Worker */
struct RWorker* r_stat_worker_start(void);
void r_stat_worker_stop(struct RWorker* rw);
int r_stat_send_test(struct RWorker* rw, const char* test_name, const char* json_data);
char* r_stat_recv_result(struct RWorker* rw);

#ifdef __cplusplus
}
#endif

#endif /* ENGINE_ABI_H */
