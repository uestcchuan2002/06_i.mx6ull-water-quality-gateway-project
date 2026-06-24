/*
 * @Author: uestcchuan2002 1992735052@qq.com
 * @Date: 2026-06-22 21:28:29
 * @LastEditors: uestcchuan2002 1992735052@qq.com
 * @LastEditTime: 2026-06-24 19:36:41
 * @FilePath: /03_water_quality_gateway_project/app/src/main.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "../include/config.h"
#include "../include/logger.h"
#include "../include/sample.h"
#include "../include/serial_port.h"

#define DEFAULT_CONFIG_PATH "../config/gateway.conf"
#define WATER_GATEWAY_VERSION "0.1.0"

static void print_usage(const char *program)
{
    printf("Usage: %s [-c config_path] [-h] [-v]\n", program);
    printf("  -c, --config   config file path, default: %s\n", DEFAULT_CONFIG_PATH);
    printf("  -h, --help     show help\n");
    printf("  -v, --version  show version\n");
}

static int parse_args(int argc, char *argv[], const char **config_path)
{
    int i;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--config") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "missing config path after %s\n", argv[i]);
                return -1;
            }

            *config_path = argv[++i];
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 1;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("water_gateway version %s\n", WATER_GATEWAY_VERSION);
            return 1;
        } else {
            fprintf(stderr, "unknown argument: %s\n", argv[i]);
            print_usage(argv[0]);
            return -1;
        }
    }

    return 0;
}

static void sleep_ms(int ms)
{
    if (ms <= 0) {
        return;
    }

#if defined(_WIN32)
    Sleep((DWORD)ms);
#else
    usleep((unsigned int)ms * 1000U);
#endif
}

int main(int argc, char *argv[])
{
    gateway_config_t cfg;
    const char *config_path = DEFAULT_CONFIG_PATH;
    int parse_result;
    water_sample_t sample;
    char sample_buf[256];
    int i;

    parse_result = parse_args(argc, argv, &config_path);
    if (parse_result > 0) {
        return 0;
    }

    if (parse_result < 0) {
        return 1;
    }

    logger_init("info");
    log_info("water gateway start");
    log_info("config_path=%s", config_path);

    if (config_load(config_path, &cfg) != 0) {
        log_error("load config failed: %s", config_path);
        return 1;
    }

    logger_init(cfg.log_level);

    log_info("config loaded");
    log_info("device_id=%s", cfg.device_id);
    log_info("sample_period_ms=%d", cfg.sample_period_ms);
    log_info("serial_device=%s", cfg.serial_device);
    log_info("baudrate=%d", cfg.baudrate);

    for (i = 0; i < 5; i++) {
        sample_generate_mock(&sample);
        sample_to_string(&sample, sample_buf, sizeof(sample_buf));
        log_info("%s", sample_buf);
        sleep_ms(cfg.sample_period_ms);
    }

    int fd = serial_open(cfg.serial_device, cfg.baudrate);
    if (fd < 0)
    {
        log_warn("serial open failed: %s", cfg.serial_device);
    }
    else
    {
        log_info("serial open success: %s", cfg.serial_device);
        serial_close(fd);
    }

    return 0;
}

