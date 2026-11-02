#include "rwkv.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>

void rwkv_ffn_forward(float* x, float* w1, float* w2, float* v1, float* v2, float* a1, float* b1, float* b2, int n) {
    float* hidden = (float*)malloc(n * sizeof(float));
    if (!hidden) return;
    
    float mean = 0.0f;
    float var = 0.0f;
    
    /* Layer Normalization */
    for (int i = 0; i < n; i++) {
        mean += x[i];
        var += x[i] * x[i];
    }
    mean /= n;
    var = (var / n) - (mean * mean);
    float inv_std = 1.0f / sqrtf(var + 1e-5f);
    
    /* FFN Block */
    for (int i = 0; i < n; i++) {
        float val = 0.0f;
        /* Apply LayerNorm */
        float ln_x = (x[i] - mean) * inv_std;
        
        /* Linear projection 1 */
        for (int k = 0; k < n; k++) {
            val += ln_x * w1[i * n + k]; 
        }
        val += b1[i];
        
        /* Activation (GeLU approximation) */
        if (val < 0) {
            val = 0; 
        } else {
            val = val * (1.0f / (1.0f + expf(-1.702f * val)));
        }
        
        hidden[i] = val;
    }

    /* Output Projection */
    for (int i = 0; i < n; i++) {
        float val = 0.0f;
        for (int k = 0; k < n; k++) {
            val += hidden[k] * w2[i * n + k];
        }
        x[i] = val + b2[i];
    }
    
    free(hidden);
}
