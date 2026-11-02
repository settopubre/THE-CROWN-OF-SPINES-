#include "kalman.h"

void kalman1d_init(KalmanFilter1D* kf, double initial_value, double process_noise, double measurement_noise) {
    if (!kf) return;
    kf->x = initial_value;
    kf->p = 1.0;
    kf->q = process_noise;
    kf->r = measurement_noise;
}

double kalman1d_update(KalmanFilter1D* kf, double measurement) {
    if (!kf) return 0.0;
    
    /* Prediction */
    kf->p = kf->p + kf->q;
    
    /* Update */
    double k = kf->p / (kf->p + kf->r);
    kf->x = kf->x + k * (measurement - kf->x);
    kf->p = (1.0 - k) * kf->p;
    
    return kf->x;
}

void kalman3d_init(KalmanFilter3D* kf, Vec3 initial, double process_noise, double measurement_noise) {
    if (!kf) return;
    kalman1d_init(&kf->x, initial.x, process_noise, measurement_noise);
    kalman1d_init(&kf->y, initial.y, process_noise, measurement_noise);
    kalman1d_init(&kf->z, initial.z, process_noise, measurement_noise);
}

Vec3 kalman3d_update(KalmanFilter3D* kf, Vec3 measurement) {
    Vec3 result;
    if (!kf) return measurement;
    result.x = kalman1d_update(&kf->x, measurement.x);
    result.y = kalman1d_update(&kf->y, measurement.y);
    result.z = kalman1d_update(&kf->z, measurement.z);
    return result;
}
