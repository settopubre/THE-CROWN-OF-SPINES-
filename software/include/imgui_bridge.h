#ifndef IMGUI_BRIDGE_H
#define IMGUI_BRIDGE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool imgui_bridge_init(void* x11_display, unsigned long x11_window, void* gl_context);
void imgui_bridge_shutdown(void);
void imgui_bridge_new_frame(void);
void imgui_bridge_render(void);
void imgui_bridge_on_mouse_move(int x, int y);
void imgui_bridge_on_mouse_button(int button, bool down);
void imgui_bridge_on_scroll(float dy);
void imgui_bridge_on_key(unsigned int keysym, bool down);
void imgui_bridge_on_text(unsigned int codepoint);
void imgui_bridge_set_display_size(int width, int height);
bool imgui_bridge_wants_mouse(void);
bool imgui_bridge_wants_keyboard(void);
void imgui_bridge_draw_editor_shell(void);

#ifdef __cplusplus
}
#endif
#endif
