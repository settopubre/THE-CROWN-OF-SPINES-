#ifndef FBO_H
#define FBO_H

#include "gpu_handle.h"

typedef struct {
    GPUHandle fbo_id;
    GPUHandle color_tex;
    GPUHandle depth_rb;
    int width;
    int height;
} FBO;

FBO* fbo_create(int width, int height, int has_depth);
void fbo_bind(FBO* fbo);
void fbo_unbind(void);
GPUHandle fbo_get_color_texture(FBO* fbo);
void fbo_destroy(FBO* fbo);

#endif /* FBO_H */
