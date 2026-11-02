#ifndef CROWN_RENDER_TYPES_H
#define CROWN_RENDER_TYPES_H

#include "crown_math.h"

typedef struct {
    CrownRect bounds;
    uint32_t buffer_count;
    int vsync;
} CrownSwapChainDesc;

/* Forward declarations for opaque pointers */
typedef struct CrownSwapChain CrownSwapChain;
typedef struct CrownRenderSystem CrownRenderSystem;
typedef struct CrownDisplay CrownDisplay;

#endif /* CROWN_RENDER_TYPES_H */
