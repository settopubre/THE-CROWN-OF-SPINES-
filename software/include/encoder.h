#ifndef ENCODER_H
#define ENCODER_H
#pragma once

#include <stdint.h>

typedef struct Encoder Encoder;

#ifdef __cplusplus
extern "C" {
#endif

Encoder* encoder_create(const char* filename, int width, int height, int fps);
void encoder_destroy(Encoder* enc);
void encoder_write_frame(Encoder* enc, const uint8_t* rgb_data);
int encoder_is_available(void);

#ifdef __cplusplus
}
#endif

#endif /* ENCODER_H */
