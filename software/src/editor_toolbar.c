#include "editor_toolbar.h"
#include "render_server.h"
#include <stdlib.h>
#include <string.h>

EditorToolbar* editor_toolbar_create(void) {
    EditorToolbar* tb = (EditorToolbar*)calloc(1, sizeof(EditorToolbar));
    if (!tb) return NULL;
    tb->current_tool = TOOL_SELECT;
    tb->snap_enabled = 1;
    tb->snap_size = 1.0f;
    tb->show_grid = 1;
    tb->y = 0; tb->x = 0; tb->width = 800; tb->height = 30;
    return tb;
}

void editor_toolbar_destroy(EditorToolbar* tb) {
    free(tb);
}

void editor_toolbar_set_tool(EditorToolbar* tb, ToolMode mode) {
    if (tb && mode < TOOL_COUNT) tb->current_tool = mode;
}

ToolMode editor_toolbar_get_tool(EditorToolbar* tb) {
    return tb ? tb->current_tool : TOOL_SELECT;
}

void editor_toolbar_toggle_snap(EditorToolbar* tb) {
    if (tb) tb->snap_enabled = !tb->snap_enabled;
}

void editor_toolbar_render(EditorToolbar* tb) {
    if (!tb) return;
    
    /* Draw toolbar background */
    float x = tb->x, y = tb->y, w = tb->width, h = tb->height;
    float bg[] = {
        x, y, 0, 0.2, 0.2, 0.2, x+w, y, 0, 0.2, 0.2, 0.2,
        x+w, y, 0, 0.2, 0.2, 0.2, x+w, y+h, 0, 0.2, 0.2, 0.2,
        x+w, y+h, 0, 0.2, 0.2, 0.2, x, y+h, 0, 0.2, 0.2, 0.2,
        x, y+h, 0, 0.2, 0.2, 0.2, x, y, 0, 0.2, 0.2, 0.2
    };
    /* render_server_draw_lines(bg, 4, vec3_create(0.2,0.2,0.2)); */

    /* Buttons */
    const char* tools[] = {"Sel", "Mov", "Rot", "Scl", "Place"};
    float btn_w = 60;
    float margin = 10;
    
    for (int i = 0; i < TOOL_COUNT; i++) {
        float bx = margin + i * (btn_w + 5);
        float active = (i == tb->current_tool);
        float color = active ? 0.8 : 0.5;
        
        /* Button border */
        float btn[] = {
            bx, y + 5, 0, color, color, color, bx+btn_w, y+5, 0, color, color, color,
            bx+btn_w, y+5, 0, color, color, color, bx+btn_w, y+h-5, 0, color, color, color,
            bx+btn_w, y+h-5, 0, color, color, color, bx, y+h-5, 0, color, color, color,
            bx, y+h-5, 0, color, color, color, bx, y+5, 0, color, color, color
        };
        /* render_server_draw_lines(btn, 4, vec3_create(color,color,color)); */
        
        /* Label line placeholder */
        float label[] = { bx + 5, y + h/2, 0, 1,1,1, bx + 30, y + h/2, 0, 1,1,1 };
        /* render_server_draw_lines(label, 1, vec3_create(1,1,1)); */
    }
    
    /* Snap indicator */
    if (tb->snap_enabled) {
        float snap_txt[] = { w - 60, y + 5, 0, 0, 1, 0, w - 20, y + 5, 0, 0, 1, 0 };
        /* render_server_draw_lines(snap_txt, 1, vec3_create(0,1,0)); */
    }
}
