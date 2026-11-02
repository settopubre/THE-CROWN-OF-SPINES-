#include <stdint.h>
#include <immintrin.h>
#include <cpuid.h>

static int avx2_supported = -1;

int rwkv_simd_available(void) {
    if (avx2_supported != -1) return avx2_supported;
    unsigned int eax, ebx, ecx, edx;
    if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) {
        avx2_supported = (ebx & (1U << 5)) ? 1 : 0;
    } else {
        avx2_supported = 0;
    }
    return avx2_supported;
}

float rwkv_dot_product_avx2(const float* a, const float* b, int n) {
    if (!rwkv_simd_available() || n < 8) {
        float sum = 0.0f;
        for (int i = 0; i < n; i++) sum += a[i] * b[i];
        return sum;
    }
    __m256 sum_vec = _mm256_setzero_ps();
    int i = 0;
    for (; i <= n - 8; i += 8) {
        __m256 va = _mm256_loadu_ps(&a[i]);
        __m256 vb = _mm256_loadu_ps(&b[i]);
        sum_vec = _mm256_fmadd_ps(va, vb, sum_vec);
    }
    __m256 hsum = _mm256_hadd_ps(sum_vec, sum_vec);
    hsum = _mm256_hadd_ps(hsum, hsum);
    float res[8];
    _mm256_storeu_ps(res, hsum);
    float sum = res[0] + res[4];
    for (; i < n; i++) sum += a[i] * b[i];
    return sum;
}

void rwkv_vector_add_avx2(float* out, const float* a, const float* b, int n) {
    if (!rwkv_simd_available() || n < 8) {
        for (int i = 0; i < n; i++) out[i] = a[i] + b[i];
        return;
    }
    int i = 0;
    for (; i <= n - 8; i += 8) {
        __m256 va = _mm256_loadu_ps(&a[i]);
        __m256 vb = _mm256_loadu_ps(&b[i]);
        _mm256_storeu_ps(&out[i], _mm256_add_ps(va, vb));
    }
    for (; i < n; i++) out[i] = a[i] + b[i];
}

void rwkv_vector_scale_avx2(float* out, const float* a, float scale, int n) {
    if (!rwkv_simd_available() || n < 8) {
        for (int i = 0; i < n; i++) out[i] = a[i] * scale;
        return;
    }
    __m256 vscale = _mm256_set1_ps(scale);
    int i = 0;
    for (; i <= n - 8; i += 8) {
        __m256 va = _mm256_loadu_ps(&a[i]);
        _mm256_storeu_ps(&out[i], _mm256_mul_ps(va, vscale));
    }
    for (; i < n; i++) out[i] = a[i] * scale;
}
