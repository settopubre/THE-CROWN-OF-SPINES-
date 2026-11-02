#include "spatial_memory.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <pthread.h>

#define BUCKET_COUNT 4096

static SpatialEntry* table = NULL;
static uint8_t* occupied = NULL;
static pthread_mutex_t spatial_lock = PTHREAD_MUTEX_INITIALIZER;

static void ensure_initialized(void) {
    if (!table) {
        table = (SpatialEntry*)malloc(BUCKET_COUNT * sizeof(SpatialEntry));
        occupied = (uint8_t*)calloc(1, BUCKET_COUNT);
    }
}

static uint32_t hash_func(int32_t gx, int32_t gz) {
    return ((gx * 73856093) ^ (gz * 19349663)) % 4096;
}

void spatial_store(int32_t gx, int32_t gz, uint32_t asset_id, const char* metadata) {
    ensure_initialized();
    if (!metadata) metadata = "";

    pthread_mutex_lock(&spatial_lock);
    uint32_t h = hash_func(gx, gz);
    for (int i = 0; i < BUCKET_COUNT; i++) {
        uint32_t idx = (h + i) % BUCKET_COUNT;
        if (!occupied[idx]) {
            occupied[idx] = 1;
            table[idx].gx = gx;
            table[idx].gz = gz;
            table[idx].asset_id = asset_id;
            table[idx].timestamp = (float)time(NULL);
            strncpy(table[idx].metadata, metadata, sizeof(table[idx].metadata) - 1);
            table[idx].metadata[sizeof(table[idx].metadata) - 1] = '\0';
            pthread_mutex_unlock(&spatial_lock);
            return;
        }
        if (table[idx].gx == gx && table[idx].gz == gz) {
            table[idx].asset_id = asset_id;
            table[idx].timestamp = (float)time(NULL);
            strncpy(table[idx].metadata, metadata, sizeof(table[idx].metadata) - 1);
            table[idx].metadata[sizeof(table[idx].metadata) - 1] = '\0';
            pthread_mutex_unlock(&spatial_lock);
            return;
        }
    }
    pthread_mutex_unlock(&spatial_lock);
}

const SpatialEntry* spatial_recall(int32_t gx, int32_t gz) {
    ensure_initialized();
    pthread_mutex_lock(&spatial_lock);
    uint32_t h = hash_func(gx, gz);
    for (int i = 0; i < BUCKET_COUNT; i++) {
        uint32_t idx = (h + i) % BUCKET_COUNT;
        if (!occupied[idx]) {
            pthread_mutex_unlock(&spatial_lock);
            return NULL;
        }
        if (table[idx].gx == gx && table[idx].gz == gz) {
            const SpatialEntry* result = &table[idx];
            pthread_mutex_unlock(&spatial_lock);
            return result;
        }
    }
    pthread_mutex_unlock(&spatial_lock);
    return NULL;
}

int spatial_recall_radius(int32_t cx, int32_t cz, int radius, SpatialEntry* out, int max_out) {
    if (!out || max_out <= 0) return 0;
    ensure_initialized();
    pthread_mutex_lock(&spatial_lock);
    int count = 0;
    for (uint32_t i = 0; i < BUCKET_COUNT && count < max_out; i++) {
        if (occupied[i]) {
            int dist = abs(table[i].gx - cx) + abs(table[i].gz - cz);
            if (dist <= radius) {
                out[count++] = table[i];
            }
        }
    }
    pthread_mutex_unlock(&spatial_lock);
    return count;
}

void spatial_clear(void) {
    pthread_mutex_lock(&spatial_lock);
    if (occupied) {
        memset(occupied, 0, BUCKET_COUNT);
    }
    if (table) {
        free(table);
        free(occupied);
        table = NULL;
        occupied = NULL;
    }
    pthread_mutex_unlock(&spatial_lock);
}
