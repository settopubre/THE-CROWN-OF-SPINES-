#define _POSIX_C_SOURCE 200809L
#include "crown_game.h"
#include "crown_log.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdbool.h>

void crown_game_init(CrownGame* game) {
    CrownDisplayDesc disp_desc = {"3DAI Engine", 800, 600};
    game->display = crown_display_create(&disp_desc);
    
    game->render_system = crown_render_system_create();
    
    CrownSwapChainDesc sc_desc = {
        .bounds = crown_rect_create(0, 0, 800, 600),
        .buffer_count = 2,
        .vsync = 1
    };
    game->swap_chain = crown_render_system_create_swap_chain(game->render_system, game->display, &sc_desc);
}

void crown_game_run(CrownGame* game) {
    if (!game || !game->display) return;
    
    bool running = true;
    CROWN_LOG_INFO("Game loop started");

    while (running) {
        /* 1. Poll X11 Events */
        while (XPending(game->display->x_display)) {
            XEvent ev;
            XNextEvent(game->display->x_display, &ev);
            if (ev.type == ClientMessage && 
                (Atom)ev.xclient.data.l[0] == XInternAtom(game->display->x_display, "WM_DELETE_WINDOW", False)) {
                running = false;
            } else if (ev.type == KeyPress) {
                KeySym ks = XLookupKeysym(&ev.xkey, 0);
                if (ks == XK_Escape) running = false;
            }
        }

        /* 2. Update Logic (Placeholder) */

        /* 3. Render */
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        CROWN_CHECK_GL();

        /* 4. Present */
        crown_display_swap_buffers(game->display);
    }
    
    CROWN_LOG_INFO("Game loop exited");
}

void crown_game_shutdown(CrownGame* game) {
    if (!game) return;
    if (game->swap_chain && game->render_system) {
        crown_render_system_destroy_swap_chain(game->render_system, game->swap_chain);
    }
    if (game->render_system) {
        crown_render_system_destroy(game->render_system);
    }
    if (game->display) {
        crown_display_destroy(game->display);
    }
}
