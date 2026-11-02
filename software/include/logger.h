#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

/* 
 * NOTE: This logger is NOT thread-safe. 
 * External synchronization (e.g., mutexes) is required if logging from multiple threads.
 */

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL
} LogLevel;

typedef struct Logger Logger;

#ifdef __cplusplus
extern "C" {
#endif

Logger* logger_create(const char* name, LogLevel min_level);
void logger_destroy(Logger* logger);
void logger_log(Logger* logger, LogLevel level, const char* file, int line, const char* fmt, ...);

#define LOG_DEBUG(logger, fmt, ...) logger_log(logger, LOG_LEVEL_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INFO(logger, fmt, ...)  logger_log(logger, LOG_LEVEL_INFO,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(logger, fmt, ...)  logger_log(logger, LOG_LEVEL_WARNING, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(logger, fmt, ...) logger_log(logger, LOG_LEVEL_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_FATAL(logger, fmt, ...) logger_log(logger, LOG_LEVEL_FATAL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif
#endif /* LOGGER_H */
