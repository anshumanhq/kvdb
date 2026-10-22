#include "util.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

static log_level g_level = LOG_INFO;

void log_set_level(log_level lvl) { g_level = lvl; }

void log_msg(log_level lvl, const char *fmt, ...)
{
    if (lvl < g_level) return;

    static const char *names[] = { "DEBUG", "INFO", "WARN", "ERROR" };

    time_t now = time(NULL);
    struct tm tm_buf;
    localtime_r(&now, &tm_buf);

    char tbuf[32];
    strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", &tm_buf);

    fprintf(stderr, "[%s] %-5s ", tbuf, names[lvl]);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fputc('\n', stderr);
}