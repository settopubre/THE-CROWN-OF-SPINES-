#define _POSIX_C_SOURCE 200809L
#include "crown_display.h"
#include "crown_log.h"
#include <stdlib.h>
#include <string.h>

CrownDisplay* crown_display_create(const CrownDisplayDesc* desc) {
    if (!desc) return NULL;

    CrownDisplay* display = (CrownDisplay*)calloc(1, sizeof(CrownDisplay));
    if (!display) return NULL;

    display->x_display = XOpenDisplay(NULL);
    if (!display->x_display) {
        CROWN_LOG_FATAL("Failed to open X display");
    }

    int glx_attribs[] = {
        GLX_X_RENDERABLE, True,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
        GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 24,
        GLX_DOUBLEBUFFER, True,
        None
    };

    int fb_count;
    GLXFBConfig* fbc = glXChooseFBConfig(display->x_display, DefaultScreen(display->x_display), glx_attribs, &fb_count);
    if (!fbc || fb_count == 0) {
        CROWN_LOG_FATAL("Failed to find a suitable GLXFBConfig");
    }

    XVisualInfo* vi = glXGetVisualFromFBConfig(display->x_display, fbc[0]);
    if (!vi) {
        CROWN_LOG_FATAL("Failed to get XVisualInfo");
    }

    XSetWindowAttributes swa;
    swa.colormap = XCreateColormap(display->x_display, RootWindow(display->x_display, vi->screen), vi->visual, AllocNone);
    swa.event_mask = ExposureMask | KeyPressMask | StructureNotifyMask;

    display->x_window = XCreateWindow(display->x_display, RootWindow(display->x_display, vi->screen),
                                      0, 0, desc->width, desc->height, 0, vi->depth, InputOutput,
                                      vi->visual, CWColormap | CWEventMask, &swa);

    int glx_ctx_attribs[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
        GLX_CONTEXT_MINOR_VERSION_ARB, 3,
        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
        None
    };

    PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = 
        (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddressARB((const GLubyte*)"glXCreateContextAttribsARB");
    
    display->glx_context = glXCreateContextAttribsARB(display->x_display, fbc[0], NULL, True, glx_ctx_attribs);
    if (!display->glx_context) {
        CROWN_LOG_FATAL("Failed to create GLX 3.3 Core context");
    }

    display->glx_drawable = (GLXDrawable)display->x_window;
    
    XFree(vi);
    XFree(fbc);

    XMapWindow(display->x_display, display->x_window);
    glXMakeCurrent(display->x_display, display->glx_drawable, display->glx_context);

    CROWN_LOG_INFO("Display created: %s (%ux%u)", desc->title, desc->width, desc->height);
    return display;
}

void crown_display_destroy(CrownDisplay* display) {
    if (!display) return;
    if (display->glx_context) {
        glXMakeCurrent(display->x_display, None, NULL);
        glXDestroyContext(display->x_display, display->glx_context);
    }
    if (display->x_window) {
        XDestroyWindow(display->x_display, display->x_window);
    }
    if (display->x_display) {
        XCloseDisplay(display->x_display);
    }
    free(display);
    CROWN_LOG_INFO("Display destroyed");
}

void crown_display_swap_buffers(CrownDisplay* display) {
    if (display && display->x_display && display->glx_drawable) {
        glXSwapBuffers(display->x_display, display->glx_drawable);
    }
}
