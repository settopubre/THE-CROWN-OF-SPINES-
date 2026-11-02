#ifndef EROSION_H
#define EROSION_H
#include <stdint.h>

typedef struct Erosion Erosion;

Erosion* erosion_create(int width, int depth, float scale);
void erosion_thermal(Erosion* e, float talus_angle, float amount);
void erosion_hydraulic(Erosion* e, float rain, float evaporation, float sediment);
void erosion_apply_to_heightmap(Erosion* e, float* heightmap, int width, int depth);
void erosion_destroy(Erosion* e);

#endif /* EROSION_H */
