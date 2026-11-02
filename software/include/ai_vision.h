#ifndef AI_VISION_H
#define AI_VISION_H

#include "vector3d.h"

#ifdef __cplusplus
extern "C" {
#endif

char* ai_vision_get_world_description(void);
void ai_vision_free_description(char* desc);
char* ai_vision_get_empty_spaces(void);
char* ai_vision_get_object_at_position(Vec3 pos);

#ifdef __cplusplus
}
#endif

#endif /* AI_VISION_H */
