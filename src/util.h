#ifndef KVDB_UTIL_H
#define KVDB_UTIL_H

#include <stddef.h>

/* Simple leveled logging — Phase 2+ mein server ke andar use hoga */
typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} log_level;

void log_set_level(log_level lvl);
void log_msg(log_level lvl, const char *fmt, ...);

#define log_debug(...) log_msg(LOG_DEBUG, __VA_ARGS__)
#define log_info(...)  log_msg(LOG_INFO,  __VA_ARGS__)
#define log_warn(...)  log_msg(LOG_WARN,  __VA_ARGS__)
#define log_error(...) log_msg(LOG_ERROR, __VA_ARGS__)

#endif