#define _POSIX_C_SOURCE 200809L
#include "engine_core.h"
#include <stdlib.h>
#include <string.h>

struct EngineCore {
    char* title;             /* Owned memory, strictly allocated */
    uint32_t width;
    uint32_t height;
    bool fullscreen;
    Logger* logger;          /* Owned dependency */
    bool is_running;
};

EngineCore* engine_core_create(const EngineDescriptor* desc, Logger* logger) {
    if (!desc || !logger) return NULL;

    EngineCore* engine = (EngineCore*)calloc(1, sizeof(EngineCore));
    if (!engine) return NULL;

    /* Strict C11: Manual allocation and copy, no POSIX strdup */
    const char* src_title = desc->title ? desc->title : "3DAI Engine";
    size_t title_len = strlen(src_title);
    engine->title = (char*)malloc(title_len + 1);
    if (engine->title) {
        strcpy(engine->title, src_title);
    }

    engine->width = desc->width > 0 ? desc->width : 800;
    engine->height = desc->height > 0 ? desc->height : 600;
    engine->fullscreen = desc->fullscreen;
    engine->logger = logger;
    engine->is_running = true;

    LOG_INFO(engine->logger, "EngineCore initialized: %s (%ux%u)", 
             engine->title, engine->width, engine->height);
    return engine;
}

void engine_core_destroy(EngineCore* engine) {
    if (!engine) return;
    LOG_INFO(engine->logger, "EngineCore shutting down...");

    if (engine->logger) {
        logger_destroy(engine->logger);
    }

    free(engine->title);
    free(engine);
}

Logger* engine_core_borrow_logger(EngineCore* engine) {
    return engine ? engine->logger : NULL;
}

int engine_core_run(EngineCore* engine) {
    if (!engine) return -1;
    LOG_INFO(engine->logger, "EngineCore run loop started.");

    while (engine->is_running) {
        /* TODO: Poll input, update physics, render frame */
    }

    LOG_INFO(engine->logger, "EngineCore run loop exited.");
    return 0;
}

void engine_core_request_quit(EngineCore* engine) {
    if (engine) {
        LOG_INFO(engine->logger, "Quit requested.");
        engine->is_running = false;
    }
}
