#ifndef UI_OVERLAY_H
#define UI_OVERLAY_H

#include <stdint.h>
#include <GL/gl.h>

#define UI_TERMINAL_MAX_LINES 12
#define UI_TERMINAL_MAX_INPUT 256
#define UI_TERMINAL_WIDTH 400
#define UI_TERMINAL_HEIGHT 250

/* FIX: Expose the struct definition so engine.c can access vbo/vao */
typedef struct UIOverlay {
    int width, height;
    GLuint vao, vbo;
} UIOverlay;

UIOverlay* ui_overlay_create(int width, int height);
void ui_overlay_text(UIOverlay* ui, float x, float y, const char* text, float r, float g, float b);
void ui_overlay_begin(UIOverlay* ui);
void ui_overlay_end(UIOverlay* ui);
void ui_overlay_destroy(UIOverlay* ui);

/* Embedded terminal functions */
void ui_terminal_init(void);
void ui_terminal_shutdown(void);
void ui_terminal_render(UIOverlay* ui);
void ui_terminal_input_char(char c);
void ui_terminal_input_backspace(void);
void ui_terminal_input_enter(void);
int  ui_terminal_is_active(void);
void ui_terminal_toggle(void);
void ui_terminal_add_line(const char* line, float r, float g, float b);

#endif /* UI_OVERLAY_H */
