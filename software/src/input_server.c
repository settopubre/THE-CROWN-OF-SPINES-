#define _POSIX_C_SOURCE 200809L
#include "input_server.h"
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct InputServer {
    InputEvent queue[INPUT_QUEUE_CAPACITY];
    int head;
    int tail;
    int count;
    int key_states[256];
    int mouse_x, mouse_y;
    int mouse_buttons[8];
};

static InputServer* g_is = NULL;

InputServer* input_server_get(void) {
    if (!g_is) {
        g_is = (InputServer*)calloc(1, sizeof(InputServer));
    }
    return g_is;
}

void input_server_init(void) {
    InputServer* is = input_server_get();
    memset(is->key_states, 0, sizeof(is->key_states));
    memset(is->mouse_buttons, 0, sizeof(is->mouse_buttons));
    is->head = is->tail = is->count = 0;
}

void input_server_shutdown(void) {
    free(g_is);
    g_is = NULL;
}

void input_server_poll_events(Display* dpy) {
    InputServer* is = input_server_get();
    while (XPending(dpy)) {
        XEvent ev;
        XNextEvent(dpy, &ev);
        if (is->count >= INPUT_QUEUE_CAPACITY) {
            is->tail = (is->tail + 1) % INPUT_QUEUE_CAPACITY;
            is->count--;
        }
        InputEvent* evt = &is->queue[is->head];
        memset(evt, 0, sizeof(InputEvent));
        
        if (ev.type == KeyPress) {
            KeySym ks = XLookupKeysym(&ev.xkey, 0);
            evt->type = 0;
            evt->key = ks;
            is->key_states[ks & 0xFF] = 1;
        } else if (ev.type == ButtonPress) {
            if (ev.xbutton.button == 4 || ev.xbutton.button == 5) {
                evt->type = 4;
                evt->scroll_delta = (ev.xbutton.button == 4) ? 1.0f : -1.0f;
            } else {
                evt->type = 1;
                evt->button = ev.xbutton.button;
                is->mouse_buttons[ev.xbutton.button] = 1;
            }
            evt->x = ev.xbutton.x; evt->y = ev.xbutton.y;
        } else if (ev.type == ButtonRelease) {
            evt->type = 2;
            evt->button = ev.xbutton.button;
            is->mouse_buttons[ev.xbutton.button] = 0;
            evt->x = ev.xbutton.x; evt->y = ev.xbutton.y;
        } else if (ev.type == MotionNotify) {
            evt->type = 3;
            evt->x = ev.xmotion.x; evt->y = ev.xmotion.y;
            is->mouse_x = evt->x; is->mouse_y = evt->y;
        } else {
            continue;
        }
        
        is->head = (is->head + 1) % INPUT_QUEUE_CAPACITY;
        is->count++;
    }
}

int input_server_get_event(InputEvent* out) {
    InputServer* is = input_server_get();
    if (is->count == 0) return 0;
    *out = is->queue[is->tail];
    is->tail = (is->tail + 1) % INPUT_QUEUE_CAPACITY;
    is->count--;
    return 1;
}

int input_server_is_key_pressed(int key) {
    return input_server_get()->key_states[key & 0xFF];
}

void input_server_get_mouse(int* x, int* y) {
    InputServer* is = input_server_get();
    if (x) *x = is->mouse_x;
    if (y) *y = is->mouse_y;
}

int input_server_is_mouse_down(int button) {
    InputServer* is = input_server_get();
    return (button >= 0 && button < 8) ? is->mouse_buttons[button] : 0;
}
