#define _POSIX_C_SOURCE 200809L
#include "crown_render_system.h"
#include "crown_log.h"
#include <stdlib.h>

struct CrownSwapChain {
    CrownSwapChainDesc desc;
    const CrownDisplay* display;
};

struct CrownRenderSystem {
    int initialized;
};

CrownRenderSystem* crown_render_system_create(void) {
    CrownRenderSystem* rs = (CrownRenderSystem*)calloc(1, sizeof(CrownRenderSystem));
    if (!rs) return NULL;
    rs->initialized = 1;
    CROWN_LOG_INFO("RenderSystem initialized");
    return rs;
}

void crown_render_system_destroy(CrownRenderSystem* rs) {
    if (rs) {
        CROWN_LOG_INFO("RenderSystem destroyed");
        free(rs);
    }
}

CrownSwapChain* crown_render_system_create_swap_chain(CrownRenderSystem* rs, 
                                                      const CrownDisplay* display, 
                                                      const CrownSwapChainDesc* desc) {
    if (!rs || !display || !desc) return NULL;

    CrownSwapChain* sc = (CrownSwapChain*)calloc(1, sizeof(CrownSwapChain));
    if (!sc) return NULL;

    sc->desc = *desc;
    sc->display = display;

    CROWN_LOG_INFO("SwapChain created: %ux%u, buffers: %u, vsync: %d", 
                   sc->desc.bounds.width, sc->desc.bounds.height, 
                   sc->desc.buffer_count, sc->desc.vsync);
    return sc;
}

void crown_render_system_destroy_swap_chain(CrownRenderSystem* rs, CrownSwapChain* sc) {
    /* TODO(rs): reserved for per-frame swap-chain state reset. Currently unused. */
    (void)rs;   /* Explicitly silence unused-parameter warning until wired */
    if (!sc) return;
    
    CROWN_LOG_INFO("SwapChain destroyed");
    free(sc);
}
