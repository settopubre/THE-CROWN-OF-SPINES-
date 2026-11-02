#ifndef THERMAL_H
#define THERMAL_H
#pragma once

typedef struct ThermalMonitor {
    float current_temp;
    float max_temp;
    int throttle_active;
} ThermalMonitor;

#ifdef __cplusplus
extern "C" {
#endif

float thermal_get_temperature(void);
int thermal_get_zones(char** zones_out, int max_zones);
void thermal_free_zones(char** zones, int count);
ThermalMonitor* thermal_monitor_create(void);
void thermal_monitor_destroy(ThermalMonitor* tm);
void thermal_monitor_update(ThermalMonitor* tm);
int thermal_monitor_should_throttle(ThermalMonitor* tm, float threshold);

#ifdef __cplusplus
}
#endif

#endif /* THERMAL_H */
