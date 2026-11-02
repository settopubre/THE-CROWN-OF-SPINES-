#ifndef ENGINE_CORE_H
#define ENGINE_CORE_H

#include "logger.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    const char* title;
    uint32_t width;
    uint32_t height;
    bool fullscreen;
} EngineDescriptor;

typedef struct EngineCore EngineCore;

#ifdef __cplusplus
extern "C" {
#endif

EngineCore* engine_core_create(const EngineDescriptor* desc, Logger* logger);
void engine_core_destroy(EngineCore* engine);

/* 
 * NOTE: Returns a BORROWED pointer. 
 * The caller must NOT free this. Lifetime is strictly tied to the EngineCore instance.
 */
Logger* engine_core_borrow_logger(EngineCore* engine);

int engine_core_run(EngineCore* engine);
void engine_core_request_quit(EngineCore* engine);

#ifdef __cplusplus
}
#endif
#endif /* ENGINE_CORE_H */
