#include "ai_vision.h"
#include "editor.h"
#include "spatial_memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern Editor* g_editor; /* Global editor from engine.c */

char* ai_vision_get_world_description(void) {
    char* desc = (char*)malloc(8192);
    if (!desc) return NULL;
    
    int offset = 0;
    offset += snprintf(desc + offset, 8192 - offset, "World State:\n");
    
    if (g_editor && g_editor->object_count > 0) {
        offset += snprintf(desc + offset, 8192 - offset, "Objects present:\n");
        for (uint32_t i = 0; i < g_editor->object_count && offset < 8191; i++) {
            AssetInstance* inst = g_editor->placed_objects[i];
            if (inst) {
                offset += snprintf(desc + offset, 8192 - offset, " - %s at (%.2f, %.2f, %.2f)\n",
                                   "object", inst->position.x, inst->position.y, inst->position.z);
            }
        }
    } else {
        offset += snprintf(desc + offset, 8192 - offset, "Objects present: None\n");
    }
    
    return desc;
}

void ai_vision_free_description(char* desc) {
    free(desc);
}

char* ai_vision_get_empty_spaces(void) {
    char* spaces = (char*)malloc(4096);
    if (!spaces) return NULL;
    
    int offset = 0;
    offset += snprintf(spaces + offset, 4096 - offset, "Available grid spaces:\n");
    
    /* Scan a grid from -50 to 50 in steps of 5 */
    for (int x = -50; x <= 50; x += 5) {
        for (int z = -50; z <= 50; z += 5) {
            const SpatialEntry* entry = spatial_recall(x, z);
            if (!entry && offset < 4095) {
                offset += snprintf(spaces + offset, 4096 - offset, " (%d, %d) is empty\n", x, z);
            }
        }
    }
    return spaces;
}

char* ai_vision_get_object_at_position(Vec3 pos) {
    static char result[256];
    /* Simplified: check spatial memory */
    const SpatialEntry* entry = spatial_recall((int32_t)pos.x, (int32_t)pos.z);
    if (entry) {
        snprintf(result, sizeof(result), "Object found at grid (%d, %d)", entry->gx, entry->gz);
        return result;
    }
    return NULL;
}
