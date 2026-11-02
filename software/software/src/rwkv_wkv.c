#include "rwkv.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

float rwkv_wkv_forward(float k, float v, float u, float w, float* state_num, float* state_den) {
    *state_num = w * (*state_num) + k * v;
    *state_den = w * (*state_den) + k;
    return (u * k * v + *state_num) / (u * k + *state_den + 1e-5f);
}

void rwkv_wkv_reset(float* state_num, float* state_den) {
    *state_num = 0.0f;
    *state_den = 0.0f;
}
