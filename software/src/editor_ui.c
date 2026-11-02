#include "editor_ui.h"
#include "render_server.h"
#include "gl_functions.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

EditorUI* editor_ui_create(void) {
    EditorUI* ui = (EditorUI*)calloc(1, sizeof(EditorUI));
    if (!ui) return NULL;
    ui->panel_capacity = 16;
    ui->panels = (UIPanel*)calloc(ui->panel_capacity, sizeof(UIPanel));
    return ui;
}

void editor_ui_destroy(EditorUI* ui) {
    if (ui) {
        free(ui->panels);
        free(ui);
    }
}

void editor_ui_add_panel(EditorUI* ui, const char* title, float x, float y, float w, float h) {
    if (!ui || !title) return;
    if (ui->panel_count >= ui->panel_capacity) {
        ui->panel_capacity *= 2;
        ui->panels = realloc(ui->panels, ui->panel_capacity * sizeof(UIPanel));
    }
    UIPanel* p = &ui->panels[ui->panel_count++];
    strncpy(p->title, title, sizeof(p->title)-1);
    p->position = vec3_create(x, y, 0.0f);
    p->width = w; p->height = h;
    p->visible = 1; p->collapsed = 0;
}

static void ui_set_ortho_projection(float left, float right, float bottom, float top) {
    float proj[16] = {0};
    proj[0] = 2.0f / (right - left);
    proj[5] = 2.0f / (top - bottom);
    proj[10] = -2.0f; 
    proj[12] = -(right + left) / (right - left);
    proj[13] = -(top + bottom) / (top - bottom);
    proj[14] = -1.0f;
    proj[15] = 1.0f;

    GLint loc = glGetUniformLocation(shader_program, "uProjection");
    if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, proj);
}

void editor_ui_render(EditorUI* ui, Mat4 view, Mat4 proj) {
    if (!ui) return;
    
    /* Set ortho projection for UI */
    ui_set_ortho_projection(0, 800, 600, 0);
    glDisable(GL_DEPTH_TEST);

    for (uint32_t i = 0; i < ui->panel_count; i++) {
        UIPanel* p = &ui->panels[i];
        if (!p->visible) continue;
        
        /* Draw border lines */
        float x = p->position.x, y = p->position.y, w = p->width, h = p->height;
        float verts[] = {
            x, y, 0, 0.5, 0.5, 0.5, x+w, y, 0, 0.5, 0.5, 0.5,
            x+w, y, 0, 0.5, 0.5, 0.5, x+w, y+h, 0, 0.5, 0.5, 0.5,
            x+w, y+h, 0, 0.5, 0.5, 0.5, x, y+h, 0, 0.5, 0.5, 0.5,
            x, y+h, 0, 0.5, 0.5, 0.5, x, y, 0, 0.5, 0.5, 0.5
        };
        
        render_server_draw_lines(verts, 4, vec3_create(0.0f, 0.0f, 0.0f));
        
        /* Placeholder text line */
        if (!p->collapsed && p->text[0]) {
            float tx = x + 10, ty = y + 20;
            /* Simplified: just draw a single horizontal line as "text" placeholder */
            float txt_vert[] = { tx, ty, 0, 1.0, 1.0, 1.0, tx + 50.0f, ty, 0, 1.0, 1.0, 1.0 };
            render_server_draw_lines(txt_vert, 1, vec3_create(1.0f, 1.0f, 1.0f));
        }
    }
    
    glEnable(GL_DEPTH_TEST);
}

void editor_ui_toggle_panel(EditorUI* ui, uint32_t panel_index) {
    if (!ui || panel_index >= ui->panel_count) return;
    ui->panels[panel_index].collapsed = !ui->panels[panel_index].collapsed;
    ui->active_panel = panel_index;
}

void editor_ui_set_panel_text(EditorUI* ui, uint32_t panel_index, const char* text) {
    if (!ui || panel_index >= ui->panel_count || !text) return;
    strncpy(ui->panels[panel_index].text, text, sizeof(ui->panels[panel_index].text)-1);
}

int editor_ui_pick_panel(EditorUI* ui, float x, float y) {
    if (!ui) return -1;
    for (int32_t i = ui->panel_count - 1; i >= 0; i--) {
        UIPanel* p = &ui->panels[i];
        if (p->visible && x >= p->position.x && x <= p->position.x + p->width &&
            y >= p->position.y && y <= p->position.y + p->height) {
            return i;
        }
    }
    return -1;
}
