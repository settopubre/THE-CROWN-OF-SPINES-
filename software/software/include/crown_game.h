#ifndef CROWN_GAME_H
#define CROWN_GAME_H

#include "crown_render_system.h"

typedef struct {
    CrownRenderSystem* render_system;
    CrownDisplay* display;
    CrownSwapChain* swap_chain;
} CrownGame;

void crown_game_init(CrownGame* game);
void crown_game_run(CrownGame* game);
void crown_game_shutdown(CrownGame* game);

#endif /* CROWN_GAME_H */
