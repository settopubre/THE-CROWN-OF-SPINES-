#ifndef DECIMATE_H
#define DECIMATE_H
#pragma once

#include "mesh.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

Mesh* decimate_mesh(Mesh* mesh, uint32_t target_tris);

#ifdef __cplusplus
}
#endif

#endif /* DECIMATE_H */
