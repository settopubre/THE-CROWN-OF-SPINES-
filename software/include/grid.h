#ifndef GRID_H
#define GRID_H

#include "gpu_handle.h"
#include "vector3d.h"
#include <stdint.h>

typedef enum {
    GRID_XZ,
    GRID_XY,
    GRID_YZ
} GridPlane;

typedef struct {
    GridPlane plane;
    float spacing;
    float size;
    int visible;
    int snap_enabled;
    Vec3 origin;
    GPUHandle vao, vbo;
    int uploaded;
} Grid;

#ifdef __cplusplus
extern "C" {
#endif

Grid* grid_create(float spacing, float size);
void grid_destroy(Grid* g);
void grid_render(Grid* g, Mat4 view, Mat4 proj);
Vec3 grid_snap(const Grid* g, Vec3 point);
void grid_cycle_plane(Grid* g);
void grid_set_spacing(Grid* g, float spacing);
void grid_toggle_visible(Grid* g);
void grid_toggle_snap(Grid* g);

#ifdef __cplusplus
}
#endif

#endif /* GRID_H */
