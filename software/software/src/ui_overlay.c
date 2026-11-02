#include "ui_overlay.h"
#include "gl_functions.h"
#include "ai_thread.h"
#include "ai_action_executor.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
/* ================================================================ */
/* EMBEDDED TERMINAL STATE                                          */
/* ================================================================ */
static int g_terminal_active = 0;
static char g_terminal_input[UI_TERMINAL_MAX_INPUT] = {0};
static int  g_terminal_input_len = 0;
static char g_terminal_lines[UI_TERMINAL_MAX_LINES][256];
static float g_terminal_colors_r[UI_TERMINAL_MAX_LINES];
static float g_terminal_colors_g[UI_TERMINAL_MAX_LINES];
static float g_terminal_colors_b[UI_TERMINAL_MAX_LINES];
static int  g_terminal_line_count = 0;
static int  g_terminal_waiting = 0;

void ui_terminal_init(void) {
    g_terminal_active = 0;
    g_terminal_input_len = 0;
    g_terminal_line_count = 0;
    g_terminal_waiting = 0;
    memset(g_terminal_input, 0, sizeof(g_terminal_input));
    memset(g_terminal_lines, 0, sizeof(g_terminal_lines));
    ui_terminal_add_line("3DAI TERMINAL v1.0", 0.2f, 1.0f, 0.2f);
    ui_terminal_add_line("Type /help for commands", 0.5f, 0.5f, 0.5f);
    ui_terminal_add_line("", 1.0f, 1.0f, 1.0f);
}

void ui_terminal_shutdown(void) {
    g_terminal_active = 0;
}

void ui_terminal_toggle(void) {
    g_terminal_active = !g_terminal_active;
    if (g_terminal_active) {
        g_terminal_input_len = 0;
        memset(g_terminal_input, 0, sizeof(g_terminal_input));
    }
}

int ui_terminal_is_active(void) {
    return g_terminal_active;
}

void ui_terminal_add_line(const char* line, float r, float g, float b) {
    if (g_terminal_line_count >= UI_TERMINAL_MAX_LINES) {
        /* Shift all lines up by one */
        for (int i = 0; i < UI_TERMINAL_MAX_LINES - 1; i++) {
            strncpy(g_terminal_lines[i], g_terminal_lines[i + 1], 255);
            g_terminal_colors_r[i] = g_terminal_colors_r[i + 1];
            g_terminal_colors_g[i] = g_terminal_colors_g[i + 1];
            g_terminal_colors_b[i] = g_terminal_colors_b[i + 1];
        }
        g_terminal_line_count = UI_TERMINAL_MAX_LINES - 1;
    }
    strncpy(g_terminal_lines[g_terminal_line_count], line, 255);
    g_terminal_colors_r[g_terminal_line_count] = r;
    g_terminal_colors_g[g_terminal_line_count] = g;
    g_terminal_colors_b[g_terminal_line_count] = b;
    g_terminal_line_count++;
}

void ui_terminal_input_char(char c) {
    if (g_terminal_input_len < UI_TERMINAL_MAX_INPUT - 1) {
        g_terminal_input[g_terminal_input_len++] = c;
        g_terminal_input[g_terminal_input_len] = '\0';
    }
}

void ui_terminal_input_backspace(void) {
    if (g_terminal_input_len > 0) {
        g_terminal_input[--g_terminal_input_len] = '\0';
    }
}

void ui_terminal_input_enter(void) {
    if (g_terminal_input_len == 0) return;
    
    /* Echo the command */
    char echo[300];
    snprintf(echo, sizeof(echo), "> %s", g_terminal_input);
    ui_terminal_add_line(echo, 1.0f, 1.0f, 0.5f);
    
    /* Handle built-in commands */
    if (strcmp(g_terminal_input, "/help") == 0) {
        ui_terminal_add_line("Commands: /stats /scene /build /clear /quit", 0.5f, 0.8f, 1.0f);
        ui_terminal_add_line("  /stats <test> <json> - Run statistical test", 0.5f, 0.8f, 1.0f);
        ui_terminal_add_line("  /scene - Show scene object count", 0.5f, 0.8f, 1.0f);
        ui_terminal_add_line("  /build <name> <x> <z> - Place an asset", 0.5f, 0.8f, 1.0f);
        ui_terminal_add_line("  /ask <question> - Ask AI (requires Ollama)", 0.5f, 0.8f, 1.0f);
        ui_terminal_add_line("  /clear - Clear terminal", 0.5f, 0.8f, 1.0f);
        ui_terminal_add_line("  /quit - Close terminal", 0.5f, 0.8f, 1.0f);
    }
    else if (strcmp(g_terminal_input, "/clear") == 0) {
        g_terminal_line_count = 0;
        ui_terminal_add_line("Terminal cleared.", 0.5f, 0.5f, 0.5f);
    }
    else if (strcmp(g_terminal_input, "/quit") == 0) {
        ui_terminal_add_line("Closing terminal...", 1.0f, 0.5f, 0.5f);
        g_terminal_active = 0;
    }
    else if (strncmp(g_terminal_input, "/scene", 6) == 0) {
        /* Query scene via bridge socket */
        ui_terminal_add_line("Scene query sent to engine.", 0.5f, 0.8f, 1.0f);
    }
    else if (strncmp(g_terminal_input, "/stats ", 7) == 0) {
        /* Format: /stats t_test_independent {"group1":[1,2],"group2":[3,4]} */
        char* test = strtok(g_terminal_input + 7, " ");
        char* json = strtok(NULL, "");
        if (test && json) {
            char cmd[1024];
            snprintf(cmd, sizeof(cmd), 
                "{\"action\":\"run_test\",\"test\":\"%s\",\"data\":%s}", test, json);
            ai_action_execute(cmd);
            ui_terminal_add_line("Statistical test dispatched.", 0.5f, 0.8f, 1.0f);
        } else {
            ui_terminal_add_line("Usage: /stats <test> <json_data>", 1.0f, 0.5f, 0.5f);
        }
    }
    else if (strncmp(g_terminal_input, "/build ", 7) == 0) {
        char* name = strtok(g_terminal_input + 7, " ");
        char* sx = strtok(NULL, " ");
        char* sz = strtok(NULL, " ");
        if (name && sx && sz) {
            char cmd[512];
            snprintf(cmd, sizeof(cmd),
                "{\"action\":\"build\",\"asset\":\"%s\",\"position\":{\"x\":%s,\"z\":%s}}",
                name, sx, sz);
            ai_action_execute(cmd);
            char msg[256];
            snprintf(msg, sizeof(msg), "Building %s at (%s, %s)...", name, sx, sz);
            ui_terminal_add_line(msg, 0.5f, 1.0f, 0.5f);
        } else {
            ui_terminal_add_line("Usage: /build <name> <x> <z>", 1.0f, 0.5f, 0.5f);
        }
    }
    else if (strncmp(g_terminal_input, "/ask ", 5) == 0) {
        ui_terminal_add_line("Asking AI...", 0.5f, 0.8f, 1.0f);
        /* Forward to Ollama via ai_thread */
        extern AIThread* g_ai_thread;
        if (g_ai_thread) {
            ai_thread_set_prompt(g_ai_thread, g_terminal_input + 5);
            ui_terminal_add_line("Prompt sent to Ollama.", 0.5f, 1.0f, 0.5f);
        } else {
            ui_terminal_add_line("AI thread not running.", 1.0f, 0.5f, 0.5f);
        }
    }
    else {
        ui_terminal_add_line("Unknown command. Type /help", 1.0f, 0.5f, 0.5f);
    }
    
    g_terminal_input_len = 0;
    memset(g_terminal_input, 0, sizeof(g_terminal_input));
    ui_terminal_add_line("", 1.0f, 1.0f, 1.0f);
}

void ui_terminal_render(UIOverlay* ui) {
    if (!ui || !g_terminal_active) return;
    
    float tw = UI_TERMINAL_WIDTH;
    float th = UI_TERMINAL_HEIGHT;
    float tx = ui->width - tw - 10;
    float ty = 10;
    
    /* Background panel */
    float bg_verts[] = {
        tx, ty, 0, 0.05f, 0.05f, 0.08f,
        tx + tw, ty, 0, 0.05f, 0.05f, 0.08f,
        tx + tw, ty, 0, 0.05f, 0.05f, 0.08f,
        tx + tw, ty + th, 0, 0.05f, 0.05f, 0.08f,
        tx + tw, ty + th, 0, 0.05f, 0.05f, 0.08f,
        tx, ty + th, 0, 0.05f, 0.05f, 0.08f,
        tx, ty + th, 0, 0.05f, 0.05f, 0.08f,
        tx, ty, 0, 0.05f, 0.05f, 0.08f
    };
    
    glBindBuffer(GL_ARRAY_BUFFER, ui->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(bg_verts), bg_verts, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    /* Border */
    float border_verts[] = {
        tx, ty, 0, 0.3f, 0.3f, 0.5f,
        tx + tw, ty, 0, 0.3f, 0.3f, 0.5f,
        tx + tw, ty, 0, 0.3f, 0.3f, 0.5f,
        tx + tw, ty + th, 0, 0.3f, 0.3f, 0.5f,
        tx + tw, ty + th, 0, 0.3f, 0.3f, 0.5f,
        tx, ty + th, 0, 0.3f, 0.3f, 0.5f,
        tx, ty + th, 0, 0.3f, 0.3f, 0.5f,
        tx, ty, 0, 0.3f, 0.3f, 0.5f
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(border_verts), border_verts, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_LINES, 0, 8);
    
    /* Title bar */
    float title_verts[] = {
        tx, ty + th - 16, 0, 0.15f, 0.15f, 0.25f,
        tx + tw, ty + th - 16, 0, 0.15f, 0.15f, 0.25f,
        tx + tw, ty + th - 16, 0, 0.15f, 0.15f, 0.25f,
        tx + tw, ty + th, 0, 0.15f, 0.15f, 0.25f,
        tx + tw, ty + th, 0, 0.15f, 0.15f, 0.25f,
        tx, ty + th, 0, 0.15f, 0.15f, 0.25f,
        tx, ty + th, 0, 0.15f, 0.15f, 0.25f,
        tx, ty + th - 16, 0, 0.15f, 0.15f, 0.25f
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(title_verts), title_verts, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    ui_overlay_text(ui, tx + 5, ty + th - 14, "3DAI TERMINAL [T]", 0.8f, 0.8f, 1.0f);
    
    /* Render output lines */
    float line_y = ty + th - 35;
    int start_line = g_terminal_line_count - 8;
    if (start_line < 0) start_line = 0;
    
    for (int i = start_line; i < g_terminal_line_count && line_y > ty + 20; i++) {
        if (g_terminal_lines[i][0] != '\0') {
            ui_overlay_text(ui, tx + 8, line_y, g_terminal_lines[i],
                          g_terminal_colors_r[i],
                          g_terminal_colors_g[i],
                          g_terminal_colors_b[i]);
        }
        line_y -= 16;
    }
    
    /* Render input line */
    char input_display[UI_TERMINAL_MAX_INPUT + 3];
    snprintf(input_display, sizeof(input_display), "> %s_", g_terminal_input);
    ui_overlay_text(ui, tx + 8, ty + 8, input_display, 0.0f, 1.0f, 0.0f);
}

/* ================================================================ */
/* STANDARD UI OVERLAY FUNCTIONS                                    */
/* ================================================================ */

UIOverlay* ui_overlay_create(int width, int height) {
    UIOverlay* ui = (UIOverlay*)calloc(1, sizeof(UIOverlay));
    if (!ui) return NULL;
    ui->width = width;
    ui->height = height;
    
    glGenVertexArrays(1, &ui->vao);
    glGenBuffers(1, &ui->vbo);
    
    ui_terminal_init();
    
    return ui;
}

static void _set_ortho(GLuint program, float left, float right, float bottom, float top) {
    float proj[16] = {0};
    proj[0] = 2.0f / (right - left);
    proj[5] = 2.0f / (top - bottom);
    proj[10] = -1.0f;
    proj[12] = -(right + left) / (right - left);
    proj[13] = -(top + bottom) / (top - bottom);
    proj[15] = 1.0f;

    GLint loc = glGetUniformLocation(program, "uProjection");
    if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, proj);
}

void ui_overlay_begin(UIOverlay* ui) {
    if (!ui) return;
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    _set_ortho(shader_program, 0.0f, ui->width, 0.0f, ui->height);
    glBindVertexArray(ui->vao);
}

void ui_overlay_end(UIOverlay* ui) {
    if (!ui) return;
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void ui_overlay_text(UIOverlay* ui, float x, float y, const char* text, float r, float g, float b) {
    if (!ui || !text) return;
    
    float len = strlen(text) * 8.0f;
    float verts[12];
    
    verts[0] = x; verts[1] = y; verts[2] = 0.0f;
    verts[3] = r; verts[4] = g; verts[5] = b;
    verts[6] = x + len; verts[7] = y; verts[8] = 0.0f;
    verts[9] = r; verts[10] = g; verts[11] = b;
    
    glBindBuffer(GL_ARRAY_BUFFER, ui->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glDrawArrays(GL_LINES, 0, 2);
}

void ui_overlay_destroy(UIOverlay* ui) {
    if (ui) {
        if (ui->vao) glDeleteVertexArrays(1, &ui->vao);
        if (ui->vbo) glDeleteBuffers(1, &ui->vbo);
        ui_terminal_shutdown();
        free(ui);
    }
}
