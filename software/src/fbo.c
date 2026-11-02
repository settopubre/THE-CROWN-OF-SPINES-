#include "fbo.h"
#include "gl_functions.h"
#include <stdlib.h>

FBO* fbo_create(int width, int height, int has_depth) {
    FBO* fbo = (FBO*)calloc(1, sizeof(FBO));
    if (!fbo) return NULL;
    
    fbo->width = width;
    fbo->height = height;
    
    glGenFramebuffers(1, &fbo->fbo_id);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo->fbo_id);
    
    glGenTextures(1, &fbo->color_tex);
    glBindTexture(GL_TEXTURE_2D, fbo->color_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, 
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                          GL_TEXTURE_2D, fbo->color_tex, 0);
    
    if (has_depth) {
        glGenRenderbuffers(1, &fbo->depth_rb);
        glBindRenderbuffer(GL_RENDERBUFFER, fbo->depth_rb);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, 
                                 GL_RENDERBUFFER, fbo->depth_rb);
    }
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fbo_destroy(fbo);
        return NULL;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return fbo;
}

void fbo_bind(FBO* fbo) {
    if (!fbo) return;
    glBindFramebuffer(GL_FRAMEBUFFER, fbo->fbo_id);
    glViewport(0, 0, fbo->width, fbo->height);
}

void fbo_unbind(void) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GPUHandle fbo_get_color_texture(FBO* fbo) {
    return fbo ? fbo->color_tex : 0;
}

void fbo_destroy(FBO* fbo) {
    if (!fbo) return;
    if (fbo->fbo_id) glDeleteFramebuffers(1, &fbo->fbo_id);
    if (fbo->color_tex) glDeleteTextures(1, &fbo->color_tex);
    if (fbo->depth_rb) glDeleteRenderbuffers(1, &fbo->depth_rb);
    free(fbo);
}
