#ifndef CROWN_MATH_H
#define CROWN_MATH_H

#include <stdint.h>

typedef struct {
    int32_t x, y;
    uint32_t width, height;
} CrownRect;

static inline CrownRect crown_rect_create(int32_t x, int32_t y, uint32_t w, uint32_t h) {
    CrownRect r = {x, y, w, h};
    return r;
}

#endif /* CROWN_MATH_H */
