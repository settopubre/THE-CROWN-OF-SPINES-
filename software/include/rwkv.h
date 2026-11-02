#ifndef RWKV_H
#define RWKV_H
#pragma once

#include <stdint.h>

typedef struct RWKVContext RWKVContext;

#ifdef __cplusplus
extern "C" {
#endif

RWKVContext* rwkv_init(const char* model_path);
int rwkv_eval(RWKVContext* ctx, const char* prompt, char* output, int max_output_len);
void rwkv_free(RWKVContext* ctx);
int rwkv_get_token_count(RWKVContext* ctx);

#ifdef __cplusplus
}
#endif

#endif /* RWKV_H */
