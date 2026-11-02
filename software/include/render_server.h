#ifndef RENDER_SERVER_H
#define RENDER_SERVER_H

#include "vector3d.h"
#include <stdint.h>
#include <GL/gl.h>

struct Mesh;

typedef struct {
    GLuint active_shader;
    Mat4 current_view;
    Mat4 current_proj;
    uint32_t draw_calls;
    uint32_t triangle_count;
} RenderServer;

#ifdef __cplusplus
extern "C" {
#endif

RenderServer* render_server_get(void);
void render_server_init(void);
void render_server_shutdown(void);
void render_server_begin_frame(Mat4 view, Mat4 proj);
void render_server_end_frame(void);
void render_server_draw_mesh(struct Mesh* mesh, Mat4 model);
void render_server_draw_lines(float* vertices, uint32_t count, Vec3 color);
void render_server_set_clear_color(float r, float g, float b);
uint32_t render_server_get_draw_calls(void);
uint32_t render_server_get_triangle_count(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDER_SERVER_H */
