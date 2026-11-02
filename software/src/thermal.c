#define _POSIX_C_SOURCE 200809L
#include "thermal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

/* Phase 14: Improved Thermal Zone Detection */
static float _read_temp_file(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) return -1.0f;
    int temp_raw;
    if (fscanf(f, "%d", &temp_raw) != 1) { fclose(f); return -1.0f; }
    fclose(f);
    return temp_raw / 1000.0f;
}

/* Try to find CPU specific thermal zone */
static float _scan_for_cpu_temp(void) {
    DIR* d = opendir("/sys/class/thermal");
    if (!d) return -1.0f;

    struct dirent* entry;
    float best_temp = -1.0f;

    while ((entry = readdir(d)) != NULL) {
        if (strncmp(entry->d_name, "thermal_zone", 12) == 0) {
            char type_path[512], temp_path[512];
            snprintf(type_path, sizeof(type_path), "/sys/class/thermal/%s/type", entry->d_name);
            snprintf(temp_path, sizeof(temp_path), "/sys/class/thermal/%s/temp", entry->d_name);
            
            FILE* type_file = fopen(type_path, "r");
            if (type_file) {
                char type_buf[64] = {0};
                fgets(type_buf, sizeof(type_buf), type_file);
                fclose(type_file);
                
                /* Check for CPU keywords */
                if (strstr(type_buf, "x86_pkg_temp") != NULL || 
                    strstr(type_buf, "coretemp") != NULL || 
                    strstr(type_buf, "cpu") != NULL) {
                    float t = _read_temp_file(temp_path);
                    if (t >= 0) {
                        closedir(d);
                        return t; /* Found best match */
                    }
                } else if (best_temp < 0) {
                    /* Fallback to any valid zone */
                    best_temp = _read_temp_file(temp_path);
                }
            }
        }
    }
    closedir(d);
    return best_temp;
}

float thermal_get_temperature(void) {
    return _scan_for_cpu_temp();
}

int thermal_get_zones(char** zones_out, int max_zones) {
    DIR* d = opendir("/sys/class/thermal");
    if (!d) return 0;
    
    int count = 0;
    struct dirent* entry;
    while ((entry = readdir(d)) != NULL && count < max_zones) {
        if (strncmp(entry->d_name, "thermal_zone", 12) == 0) {
            char* path = (char*)malloc(512);
            if (path) {
                snprintf(path, 512, "/sys/class/thermal/%s/type", entry->d_name);
                zones_out[count++] = path;
            }
        }
    }
    closedir(d);
    return count;
}

void thermal_free_zones(char** zones, int count) {
    if (!zones) return;
    for (int i = 0; i < count; i++) {
        free(zones[i]);
    }
}

ThermalMonitor* thermal_monitor_create(void) {
    ThermalMonitor* tm = (ThermalMonitor*)calloc(1, sizeof(ThermalMonitor));
    if (!tm) return NULL;
    tm->current_temp = thermal_get_temperature();
    tm->max_temp = 75.0f;
    tm->throttle_active = 0;
    return tm;
}

void thermal_monitor_destroy(ThermalMonitor* tm) {
    free(tm);
}

void thermal_monitor_update(ThermalMonitor* tm) {
    if (!tm) return;
    float t = thermal_get_temperature();
    if (t >= 0) tm->current_temp = t;
    tm->throttle_active = (tm->current_temp >= tm->max_temp);
}

int thermal_monitor_should_throttle(ThermalMonitor* tm, float threshold) {
    return tm && tm->current_temp >= threshold;
}
