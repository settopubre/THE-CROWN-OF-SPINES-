#ifndef EDITOR_UI_H
#define EDITOR_UI_H

#include "vector3d.h"
#include <stdint.h>

typedef struct {
    char title[64];
    Vec3 position;
    float width, height;
    int visible;
    int collapsed;
    char text[512];
} UIPanel;

typedef struct {
    UIPanel* panels;
    uint32_t panel_count;
    uint32_t panel_capacity;
    uint32_t active_panel;
} EditorUI;

#ifdef __cplusplus
extern "C" {
#endif

EditorUI* editor_ui_create(void);
void editor_ui_destroy(EditorUI* ui);
void editor_ui_add_panel(EditorUI* ui, const char* title, float x, float y, float w, float h);
void editor_ui_render(EditorUI* ui, Mat4 view, Mat4 proj);
void editor_ui_toggle_panel(EditorUI* ui, uint32_t panel_index);
void editor_ui_set_panel_text(EditorUI* ui, uint32_t panel_index, const char* text);
int editor_ui_pick_panel(EditorUI* ui, float x, float y);

#ifdef __cplusplus
}
#endif

#endif /* EDITOR_UI_H */
