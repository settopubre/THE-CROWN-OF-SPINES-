#ifndef INPUT_SERVER_H
#define INPUT_SERVER_H

#include "vector3d.h"
#include <stdint.h>
#include <X11/Xlib.h>

#define INPUT_QUEUE_CAPACITY 256

typedef enum {
    ENGINE_KEY_ESCAPE = 0xFF1B,
    ENGINE_KEY_G = 'g',
    ENGINE_KEY_B = 'b',
    ENGINE_KEY_S = 's',
    ENGINE_KEY_E = 'e',
    ENGINE_KEY_F = 'f',
    ENGINE_KEY_R = 'r',
    ENGINE_KEY_TAB = 0xFF09,
    ENGINE_KEY_1 = '1', ENGINE_KEY_2 = '2', ENGINE_KEY_3 = '3',
    ENGINE_KEY_4 = '4', ENGINE_KEY_5 = '5', ENGINE_KEY_6 = '6', ENGINE_KEY_7 = '7',
    ENGINE_KEY_UNKNOWN = 0
} EngineKey;

typedef enum {
    MOUSE_LEFT = 1,
    MOUSE_MIDDLE = 2,
    MOUSE_RIGHT = 3
} MouseButton;

typedef struct {
    int type; /* 0=key, 1=mouse_press, 2=mouse_release, 3=mouse_move, 4=scroll */
    int key;
    int button;
    int x, y;
    float scroll_delta;
} InputEvent;

typedef struct InputServer InputServer;

#ifdef __cplusplus
extern "C" {
#endif

InputServer* input_server_get(void);
void input_server_init(void);
void input_server_shutdown(void);
void input_server_poll_events(Display* dpy);
int input_server_get_event(InputEvent* out);
int input_server_is_key_pressed(int key);
void input_server_get_mouse(int* x, int* y);
int input_server_is_mouse_down(int button);

#ifdef __cplusplus
}
#endif

#endif /* INPUT_SERVER_H */
