#ifndef SPATIAL_MEMORY_H
#define SPATIAL_MEMORY_H

#include <stdint.h>

typedef struct {
    int32_t gx, gz;
    uint32_t asset_id;
    float timestamp;
    char metadata[256];
} SpatialEntry;

#ifdef __cplusplus
extern "C" {
#endif

void spatial_store(int32_t gx, int32_t gz, uint32_t asset_id, const char* metadata);
const SpatialEntry* spatial_recall(int32_t gx, int32_t gz);
int spatial_recall_radius(int32_t cx, int32_t cz, int radius, SpatialEntry* out, int max_out);
void spatial_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* SPATIAL_MEMORY_H */
