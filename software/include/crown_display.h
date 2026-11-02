#ifndef CROWN_DISPLAY_H
#define CROWN_DISPLAY_H

#include <X11/Xlib.h>
#include <GL/glx.h>
#include "crown_render_types.h"

typedef struct {
    const char* title;
    uint32_t width;
    uint32_t height;
} CrownDisplayDesc;

struct CrownDisplay {
    Display* x_display;
    Window x_window;
    GLXContext glx_context;
    GLXDrawable glx_drawable;
};

CrownDisplay* crown_display_create(const CrownDisplayDesc* desc);
void crown_display_destroy(CrownDisplay* display);
void crown_display_swap_buffers(CrownDisplay* display);

#endif /* CROWN_DISPLAY_H */
