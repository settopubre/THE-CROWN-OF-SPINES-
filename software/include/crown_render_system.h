#ifndef CROWN_RENDER_SYSTEM_H
#define CROWN_RENDER_SYSTEM_H

#include "crown_render_types.h"
#include "crown_display.h"

#ifdef __cplusplus
extern "C" {
#endif

CrownRenderSystem* crown_render_system_create(void);
void crown_render_system_destroy(CrownRenderSystem* rs);

/* Factory method for creating swap chains */
CrownSwapChain* crown_render_system_create_swap_chain(CrownRenderSystem* rs, 
                                                      const CrownDisplay* display, 
                                                      const CrownSwapChainDesc* desc);
void crown_render_system_destroy_swap_chain(CrownRenderSystem* rs, CrownSwapChain* sc);

#ifdef __cplusplus
}
#endif

#endif /* CROWN_RENDER_SYSTEM_H */
