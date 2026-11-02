#ifndef CROWN_LOG_H
#define CROWN_LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <GL/gl.h>

typedef enum {
    CROWN_LOG_INFO,
    CROWN_LOG_WARN,
    CROWN_LOG_ERROR,
    CROWN_LOG_FATAL
} CrownLogLevel;

void crown_log(CrownLogLevel level, const char* file, int line, const char* fmt, ...);

#define CROWN_LOG_INFO(fmt, ...)  crown_log(CROWN_LOG_INFO,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define CROWN_LOG_WARN(fmt, ...)  crown_log(CROWN_LOG_WARN,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define CROWN_LOG_ERROR(fmt, ...) crown_log(CROWN_LOG_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define CROWN_LOG_FATAL(fmt, ...) crown_log(CROWN_LOG_FATAL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define CROWN_CHECK_GL() do { \
    GLenum err = glGetError(); \
    if (err != GL_NO_ERROR) { \
        CROWN_LOG_ERROR("OpenGL Error: 0x%x at %s:%d", err, __FILE__, __LINE__); \
    } \
} while(0)

#endif /* CROWN_LOG_H */
