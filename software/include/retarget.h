#ifndef RETARGET_H
#define RETARGET_H
#pragma once

#include "skeleton.h"
#include "ai_thread.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void retarget_pose(Skeleton* target_skel, const AIPose* src_pose, const uint32_t* bone_map);

#ifdef __cplusplus
}
#endif

#endif /* RETARGET_H */
