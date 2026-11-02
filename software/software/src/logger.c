#define _POSIX_C_SOURCE 200809L
#include "logger.h"
#include <stdlib.h>
#include <string.h>

struct Logger {
    char name[64];
    LogLevel min_level;
    FILE* output_stream;
};

Logger* logger_create(const char* name, LogLevel min_level) {
    Logger* logger = (Logger*)calloc(1, sizeof(Logger));
    if (!logger) return NULL;
    
    /* Strict C11: Explicit, guaranteed null-termination without POSIX strdup/snprintf */
    const char* safe_name = name ? name : "Engine";
    size_t len = strlen(safe_name);
    if (len >= sizeof(logger->name)) {
        len = sizeof(logger->name) - 1;
    }
    memcpy(logger->name, safe_name, len);
    logger->name[len] = '\0'; /* Explicit null-termination */
    
    logger->min_level = min_level;
    logger->output_stream = stderr;
    return logger;
}

void logger_destroy(Logger* logger) {
    if (logger) free(logger);
}

void logger_log(Logger* logger, LogLevel level, const char* file, int line, const char* fmt, ...) {
    /* Strict null checks on critical pointers */
    if (!logger || !file || !fmt) return;
    if (level < logger->min_level) return;

    const char* level_str = "UNKNOWN";
    switch (level) {
        case LOG_LEVEL_DEBUG:   level_str = "DEBUG"; break;
        case LOG_LEVEL_INFO:    level_str = "INFO"; break;
        case LOG_LEVEL_WARNING: level_str = "WARN"; break;
        case LOG_LEVEL_ERROR:   level_str = "ERROR"; break;
        case LOG_LEVEL_FATAL:   level_str = "FATAL"; break;
    }

    const char* filename = file;
    const char* slash = strrchr(file, '/');
    if (slash) filename = slash + 1;

    fprintf(logger->output_stream, "[%s] [%s] %s:%d: ", logger->name, level_str, filename, line);
    
    va_list args;
    va_start(args, fmt);
    vfprintf(logger->output_stream, fmt, args);
    va_end(args);
    fprintf(logger->output_stream, "\n");
    
    if (level == LOG_LEVEL_FATAL) exit(EXIT_FAILURE);
}
