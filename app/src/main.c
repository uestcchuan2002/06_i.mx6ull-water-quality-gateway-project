/*
 * @Author: uestcchuan2002 1992735052@qq.com
 * @Date: 2026-06-22 21:28:29
 * @LastEditors: uestcchuan2002 1992735052@qq.com
 * @LastEditTime: 2026-06-23 19:40:16
 * @FilePath: /03_water_quality_gateway_project/app/src/main.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include <stdio.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "../include/config.h"
#include "../include/logger.h"
#include "../include/sample.h"

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

int main(void)
{
    gateway_config_t cfg;
    const char *config_path = "../config/gateway.conf";

    logger_init("info");
    log_info("water gateway start");

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

    water_sample_t sample;
    char sample_buf[256];

    for (int i = 0; i < 5; i++)
    {
        sample_generate_mock(&sample);
        sample_to_string(&sample, sample_buf, sizeof(sample_buf));
        log_info("%s", sample_buf);
        sleep_ms(cfg.sample_period_ms);
    }

    return 0;
}
