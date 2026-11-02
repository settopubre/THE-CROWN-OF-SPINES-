#ifndef FDEEP_TENSOR_HPP
#define FDEEP_TENSOR_HPP

#include <cstdlib>

struct tensor {
    float* data;
    int dims[4];
    int ndim;
};

inline tensor* tensor_create(int d0, int d1, int d2, int d3) {
    tensor* t = (tensor*)malloc(sizeof(tensor));
    if (!t) return nullptr;
    
    t->dims[0] = d0; t->dims[1] = d1; t->dims[2] = d2; t->dims[3] = d3;
    t->ndim = 4;
    int size = d0 * d1 * d2 * d3;
    t->data = (float*)malloc(size * sizeof(float));
    return t;
}

inline void tensor_destroy(tensor* t) {
    if (t) {
        free(t->data);
        free(t);
    }
}

inline float tensor_get(tensor* t, int i0, int i1, int i2, int i3) {
    if (!t || !t->data) return 0.0f;
    int stride3 = t->dims[3];
    int stride2 = t->dims[2] * stride3;
    int stride1 = t->dims[1] * stride2;
    int idx = i0 * stride1 + i1 * stride2 + i2 * stride3 + i3;
    return t->data[idx];
}

#endif /* FDEEP_TENSOR_HPP */
