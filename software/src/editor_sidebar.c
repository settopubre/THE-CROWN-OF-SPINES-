#include "editor_sidebar.h"
#include "render_server.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

EditorSidebar* editor_sidebar_create(float x, float y, float w, float h) {
    EditorSidebar* sb = (EditorSidebar*)calloc(1, sizeof(EditorSidebar));
    if (!sb) return NULL;
    sb->x = x; sb->y = y; sb->width = w; sb->height = h;
    sb->ui = editor_ui_create();
    sb->active_category = CAT_SHAPES;
    sb->selected_shape = SHAPE_SQUARE;
    
    editor_ui_add_panel(sb->ui, "Sidebar", x, y, w, h);
    return sb;
}

void editor_sidebar_destroy(EditorSidebar* sb) {
    if (sb) {
        editor_ui_destroy(sb->ui);
        free(sb);
    }
}

void editor_sidebar_set_category(EditorSidebar* sb, uint32_t category) {
    if (sb && category < CAT_COUNT) {
        sb->active_category = category;
        /* In a full app, this would rebuild the UI content for the category */
    }
}

ShapeType editor_sidebar_get_selected_shape(EditorSidebar* sb) {
    return sb ? sb->selected_shape : SHAPE_SQUARE;
}

void editor_sidebar_next_shape(EditorSidebar* sb) {
    if (sb) sb->selected_shape = (ShapeType)((sb->selected_shape + 1) % 5);
}

void editor_sidebar_prev_shape(EditorSidebar* sb) {
    if (sb) sb->selected_shape = (sb->selected_shape == 0) ? 4 : (sb->selected_shape - 1);
}

void editor_sidebar_render(EditorSidebar* sb) {
    if (!sb) return;
    
    /* Render panel background */
    float x = sb->x, y = sb->y, w = sb->width, h = sb->height;
    /* Draw border */
    float border[] = {
        x, y, 0, 0.3, 0.3, 0.3, x+w, y, 0, 0.3, 0.3, 0.3,
        x+w, y, 0, 0.3, 0.3, 0.3, x+w, y+h, 0, 0.3, 0.3, 0.3,
        x+w, y+h, 0, 0.3, 0.3, 0.3, x, y+h, 0, 0.3, 0.3, 0.3,
        x, y+h, 0, 0.3, 0.3, 0.3, x, y, 0, 0.3, 0.3, 0.3
    };
    
    /* We use an identity model/view matrix logic here for screen space UI if render_server supports it,
       but for now we assume render_server_draw_lines handles coordinates relative to view.
       For simplicity, we just draw the panel outline. */
       
    /* Draw category tabs at top */
    float tab_y = y;
    float tab_h = 20;
    float tab_w = w / CAT_COUNT;
    
    const char* labels[] = {"Shapes", "Mats", "Sett", "Assets"};
    for (int i = 0; i < CAT_COUNT; i++) {
        float tx = x + i * tab_w;
        float t_verts[] = {
            tx, tab_y, 0, (i==sb->active_category)?1.0:0.5, 0.5, 0.5, tx+tab_w, tab_y, 0, (i==sb->active_category)?1.0:0.5, 0.5, 0.5
        };
        /* render_server_draw_lines(t_verts, 1, vec3_create(1,0.5,0.5)); */ /* Placeholder line */
    }

    /* Draw Shape List */
    if (sb->active_category == CAT_SHAPES) {
        float item_y = tab_y + tab_h + 10;
        float item_h = 25;
        const char* shape_names[] = {"Square", "Circle", "Rectangle", "Polygon", "Line"};
        
        for (int i = 0; i < 5; i++) {
            float iy = item_y + i * (item_h + 5);
            if (iy > y + h - 30) break;
            
            /* Selection highlight */
            if (i == sb->selected_shape) {
                float hl_verts[] = {
                    x + 5, iy, 0, 0.0, 1.0, 0.0, x + w - 5, iy, 0, 0.0, 1.0, 0.0,
                    x + w - 5, iy, 0, 0.0, 1.0, 0.0, x + w - 5, iy + item_h, 0, 0.0, 1.0, 0.0,
                    x + w - 5, iy + item_h, 0, 0.0, 1.0, 0.0, x + 5, iy + item_h, 0, 0.0, 1.0, 0.0,
                    x + 5, iy + item_h, 0, 0.0, 1.0, 0.0, x + 5, iy, 0, 0.0, 1.0, 0.0
                };
                /* render_server_draw_lines(hl_verts, 4, vec3_create(0,1,0)); */
            }
            
            /* Text placeholder line */
            float text_verts[] = { x + 10, iy + 5, 0, 1,1,1, x + 60, iy + 5, 0, 1,1,1 };
            /* render_server_draw_lines(text_verts, 1, vec3_create(1,1,1)); */
        }
    }
}
