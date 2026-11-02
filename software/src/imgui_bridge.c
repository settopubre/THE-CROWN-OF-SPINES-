#include "imgui_bridge.h"
#include <stddef.h>

extern bool imgui_glue_init(void* display, unsigned long window, void* gl_ctx);
extern void imgui_glue_shutdown(void);
extern void imgui_glue_new_frame(void);
extern void imgui_glue_render(void);
extern void imgui_glue_on_mouse_move(int x, int y);
extern void imgui_glue_on_mouse_button(int button, bool down);
extern void imgui_glue_on_scroll(float dy);
extern void imgui_glue_on_key(unsigned int keysym, bool down);
extern void imgui_glue_on_text(unsigned int codepoint);
extern void imgui_glue_set_display_size(int w, int h);
extern bool imgui_glue_wants_mouse(void);
extern bool imgui_glue_wants_keyboard(void);
extern void imgui_glue_draw_editor_shell(void);

bool imgui_bridge_init(void* d, unsigned long w, void* g) { return imgui_glue_init(d, w, g); }
void imgui_bridge_shutdown(void) { imgui_glue_shutdown(); }
void imgui_bridge_new_frame(void) { imgui_glue_new_frame(); }
void imgui_bridge_render(void) { imgui_glue_render(); }
void imgui_bridge_on_mouse_move(int x, int y) { imgui_glue_on_mouse_move(x, y); }
void imgui_bridge_on_mouse_button(int b, bool d) { imgui_glue_on_mouse_button(b, d); }
void imgui_bridge_on_scroll(float dy) { imgui_glue_on_scroll(dy); }
void imgui_bridge_on_key(unsigned int k, bool d) { imgui_glue_on_key(k, d); }
void imgui_bridge_on_text(unsigned int c) { imgui_glue_on_text(c); }
void imgui_bridge_set_display_size(int w, int h) { imgui_glue_set_display_size(w, h); }
bool imgui_bridge_wants_mouse(void) { return imgui_glue_wants_mouse(); }
bool imgui_bridge_wants_keyboard(void) { return imgui_glue_wants_keyboard(); }
void imgui_bridge_draw_editor_shell(void) { imgui_glue_draw_editor_shell(); }
