#include "logger.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static log_level_t g_log_level = LOG_LEVEL_INFO;

static const char *logger_level_name(log_level_t level)
{
    switch (level) {
    case LOG_LEVEL_DEBUG:
        return "DEBUG";
    case LOG_LEVEL_INFO:
        return "INFO";
    case LOG_LEVEL_WARN:
        return "WARN";
    case LOG_LEVEL_ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

static int logger_level_from_string(const char *level, log_level_t *out_level)
{
    if (level == NULL || out_level == NULL) {
        return -1;
    }

    if (strcmp(level, "debug") == 0 || strcmp(level, "DEBUG") == 0) {
        *out_level = LOG_LEVEL_DEBUG;
        return 0;
    }

    if (strcmp(level, "info") == 0 || strcmp(level, "INFO") == 0) {
        *out_level = LOG_LEVEL_INFO;
        return 0;
    }

    if (strcmp(level, "warn") == 0 || strcmp(level, "WARN") == 0) {
        *out_level = LOG_LEVEL_WARN;
        return 0;
    }

    if (strcmp(level, "error") == 0 || strcmp(level, "ERROR") == 0) {
        *out_level = LOG_LEVEL_ERROR;
        return 0;
    }

    return -1;
}

void logger_init(const char *level)
{
    log_level_t parsed_level;

    if (logger_level_from_string(level, &parsed_level) == 0) {
        g_log_level = parsed_level;
    } else {
        g_log_level = LOG_LEVEL_INFO;
    }
}

void logger_set_level(log_level_t level)
{
    g_log_level = level;
}

void logger_log(log_level_t level, const char *fmt, ...)
{
    time_t now;
    struct tm tm_now;
    char time_buf[32];
    va_list args;

    if (level < g_log_level) {
        return;
    }

    now = time(NULL);

#if defined(_WIN32)
    localtime_s(&tm_now, &now);
#else
    localtime_r(&now, &tm_now);
#endif

    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_now);

    fprintf(stdout, "[%s] [%s] ", time_buf, logger_level_name(level));

    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);

    fputc('\n', stdout);
    fflush(stdout);
}

