#ifndef KALMAN_H
#define KALMAN_H
#pragma once

#include "vector3d.h"

typedef struct {
    double x;  /* Estimate */
    double p;  /* Estimate covariance */
    double q;  /* Process noise */
    double r;  /* Measurement noise */
} KalmanFilter1D;

typedef struct {
    KalmanFilter1D x, y, z;
} KalmanFilter3D;

#ifdef __cplusplus
extern "C" {
#endif

void kalman1d_init(KalmanFilter1D* kf, double initial_value, double process_noise, double measurement_noise);
double kalman1d_update(KalmanFilter1D* kf, double measurement);
void kalman3d_init(KalmanFilter3D* kf, Vec3 initial, double process_noise, double measurement_noise);
Vec3 kalman3d_update(KalmanFilter3D* kf, Vec3 measurement);

#ifdef __cplusplus
}
#endif

#endif /* KALMAN_H */
