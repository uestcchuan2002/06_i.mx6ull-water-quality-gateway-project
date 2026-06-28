#ifndef __LOGGER_H
#define __LOGGER_H

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} log_level_t;

void logger_init(const char *level);
void logger_set_level(log_level_t level);
void logger_open_log_file(const char *path);
void logger_close_log_file(void);
void logger_log(log_level_t level, const char *fmt, ...);

#define log_debug(...) logger_log(LOG_LEVEL_DEBUG, __VA_ARGS__)
#define log_info(...)  logger_log(LOG_LEVEL_INFO,  __VA_ARGS__)
#define log_warn(...)  logger_log(LOG_LEVEL_WARN,  __VA_ARGS__)
#define log_error(...) logger_log(LOG_LEVEL_ERROR, __VA_ARGS__)

#endif
