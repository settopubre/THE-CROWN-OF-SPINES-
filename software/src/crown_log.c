#define _POSIX_C_SOURCE 200809L
#include "crown_log.h"
#include <stdarg.h>
#include <string.h>

void crown_log(CrownLogLevel level, const char* file, int line, const char* fmt, ...) {
    const char* level_str = "INFO";
    FILE* stream = stdout;
    if (level == CROWN_LOG_WARN) { level_str = "WARN"; stream = stderr; }
    else if (level == CROWN_LOG_ERROR) { level_str = "ERROR"; stream = stderr; }
    else if (level == CROWN_LOG_FATAL) { level_str = "FATAL"; stream = stderr; }

    const char* filename = strrchr(file, '/');
    filename = filename ? filename + 1 : file;

    fprintf(stream, "[CROWN] [%s] %s:%d: ", level_str, filename, line);
    
    va_list args;
    va_start(args, fmt);
    vfprintf(stream, fmt, args);
    va_end(args);
    fprintf(stream, "\n");

    if (level == CROWN_LOG_FATAL) {
        exit(EXIT_FAILURE);
    }
}
