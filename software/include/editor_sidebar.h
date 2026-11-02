#ifndef EDITOR_SIDEBAR_H
#define EDITOR_SIDEBAR_H

#include "editor_ui.h"
#include "shape_palette.h"
#include <stdint.h>

typedef enum {
    CAT_SHAPES,
    CAT_MATERIALS,
    CAT_SETTINGS,
    CAT_ASSETS,
    CAT_COUNT
} SidebarCategory;

typedef struct {
    EditorUI* ui;
    uint32_t active_category;
    float x, y, width, height;
    ShapeType selected_shape;
} EditorSidebar;

#ifdef __cplusplus
extern "C" {
#endif

EditorSidebar* editor_sidebar_create(float x, float y, float w, float h);
void editor_sidebar_destroy(EditorSidebar* sb);
void editor_sidebar_set_category(EditorSidebar* sb, uint32_t category);
ShapeType editor_sidebar_get_selected_shape(EditorSidebar* sb);
void editor_sidebar_next_shape(EditorSidebar* sb);
void editor_sidebar_prev_shape(EditorSidebar* sb);
void editor_sidebar_render(EditorSidebar* sb);

#ifdef __cplusplus
}
#endif

#endif /* EDITOR_SIDEBAR_H */
