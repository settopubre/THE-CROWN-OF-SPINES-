#ifndef EDITOR_TOOLBAR_H
#define EDITOR_TOOLBAR_H

#include "vector3d.h"
#include <stdint.h>

typedef enum {
    TOOL_SELECT,
    TOOL_MOVE,
    TOOL_ROTATE,
    TOOL_SCALE,
    TOOL_PLACE,
    TOOL_COUNT
} ToolMode;

typedef struct {
    ToolMode current_tool;
    int snap_enabled;
    float snap_size;
    int show_grid;
    /* UI positioning */
    float x, y, width, height;
} EditorToolbar;

#ifdef __cplusplus
extern "C" {
#endif

EditorToolbar* editor_toolbar_create(void);
void editor_toolbar_destroy(EditorToolbar* tb);
void editor_toolbar_set_tool(EditorToolbar* tb, ToolMode mode);
ToolMode editor_toolbar_get_tool(EditorToolbar* tb);
void editor_toolbar_toggle_snap(EditorToolbar* tb);
void editor_toolbar_render(EditorToolbar* tb);

#ifdef __cplusplus
}
#endif

#endif /* EDITOR_TOOLBAR_H */
