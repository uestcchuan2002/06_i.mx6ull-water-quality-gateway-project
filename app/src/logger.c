#include "logger.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static log_level_t g_log_level = LOG_LEVEL_INFO;
static FILE *g_log_file = NULL;

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

void logger_open_log_file(const char *path)
{
    if (g_log_file != NULL && g_log_file != stdout) {
        fclose(g_log_file);
        g_log_file = NULL;
    }

    if (path == NULL || path[0] == '\0' ||
        strcmp(path, "stdout") == 0 || strcmp(path, "STDOUT") == 0) {
        g_log_file = NULL;
        return;
    }

    g_log_file = fopen(path, "a");
    if (g_log_file == NULL) {
        fprintf(stderr, "logger: failed to open log file: %s\n", path);
    }
}

void logger_close_log_file(void)
{
    if (g_log_file != NULL && g_log_file != stdout) {
        fclose(g_log_file);
        g_log_file = NULL;
    }
}

void logger_log(log_level_t level, const char *fmt, ...)
{
    time_t now;
    struct tm tm_now;
    char time_buf[32];
    va_list args;
    va_list args_file;

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

    if (g_log_file != NULL) {
        fprintf(g_log_file, "[%s] [%s] ", time_buf, logger_level_name(level));

        va_start(args_file, fmt);
        vfprintf(g_log_file, fmt, args_file);
        va_end(args_file);

        fputc('\n', g_log_file);
        fflush(g_log_file);
    }
}

