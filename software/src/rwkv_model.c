#include "rwkv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int n_vocab;
    int n_embed;
    int n_layer;
    float** weights;
    int* weight_sizes;
} RWKVModel;

static int rwkv_model_parse_header(FILE* f, int* version, int* n_vocab, int* n_embed, int* n_layer) {
    char magic[4];
    if (fread(magic, 1, 4, f) != 4 || memcmp(magic, "RWKV", 4) != 0) return -1;
    if (fread(version, sizeof(int), 1, f) != 1) return -1;
    if (fread(n_vocab, sizeof(int), 1, f) != 1) return -1;
    if (fread(n_embed, sizeof(int), 1, f) != 1) return -1;
    if (fread(n_layer, sizeof(int), 1, f) != 1) return -1;
    return 0;
}

static RWKVModel* rwkv_model_load(const char* filepath) {
    FILE* f = fopen(filepath, "rb");
    if (!f) return NULL;

    int version, n_vocab, n_embed, n_layer;
    if (rwkv_model_parse_header(f, &version, &n_vocab, &n_embed, &n_layer) != 0) {
        fclose(f);
        return NULL;
    }

    RWKVModel* model = (RWKVModel*)calloc(1, sizeof(RWKVModel));
    if (!model) { fclose(f); return NULL; }

    model->n_vocab = n_vocab;
    model->n_embed = n_embed;
    model->n_layer = n_layer;
    model->weights = (float**)calloc(n_layer, sizeof(float*));
    model->weight_sizes = (int*)calloc(n_layer, sizeof(int));
    if (!model->weights || !model->weight_sizes) {
        fclose(f); free(model->weights); free(model->weight_sizes); free(model); return NULL;
    }

    for (int i = 0; i < n_layer; i++) {
        int size = 0;
        if (fread(&size, sizeof(int), 1, f) != 1) break;
        model->weight_sizes[i] = size;
        model->weights[i] = (float*)malloc(size * sizeof(float));
        if (model->weights[i] && fread(model->weights[i], sizeof(float), size, f) != (size_t)size) {
            free(model->weights[i]);
            model->weights[i] = NULL;
        }
    }

    fclose(f);
    return model;
}

static void rwkv_model_free(RWKVModel* model) {
    if (!model) return;
    for (int i = 0; i < model->n_layer; i++) {
        free(model->weights[i]);
    }
    free(model->weights);
    free(model->weight_sizes);
    free(model);
}

/* ============================================================ */
/* WRAPPER FUNCTIONS for RWKVContext API (used by ai_thread.c) */
/* ============================================================ */

RWKVContext* rwkv_init(const char* model_path) {
    return (RWKVContext*)rwkv_model_load(model_path);
}

void rwkv_free(RWKVContext* ctx) {
    rwkv_model_free((RWKVModel*)ctx);
}

int rwkv_eval(RWKVContext* ctx, const char* prompt, char* output, int max_output_len) {
    if (!ctx || !prompt || !output) return -1;
    
    /* Placeholder response - returns dummy joint positions */
    snprintf(output, max_output_len, 
        "{\"joints\":[{\"x\":0.0,\"y\":0.0,\"z\":0.0},{\"x\":0.1,\"y\":0.2,\"z\":0.3}]}");
    
    return 0;
}

int rwkv_get_token_count(RWKVContext* ctx) {
    (void)ctx;
    return 100;
}
