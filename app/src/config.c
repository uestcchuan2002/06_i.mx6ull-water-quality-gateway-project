/*
 * @Author: uestcchuan2002 1992735052@qq.com
 * @Date: 2026-06-23 19:03:53
 * @LastEditors: uestcchuan2002 1992735052@qq.com
 * @LastEditTime: 2026-06-23 19:16:34
 * @FilePath: /03_water_quality_gateway_project/app/src/config.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "../include/config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *trim(char *str)
{
    char *end;

    while (isspace((unsigned char)*str)) {
        str++;
    }

    if (*str == '\0') {
        return str;
    }

    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }

    *(end + 1) = '\0';
    return str;
}

static void copy_string(char *dst, size_t dst_size, const char *src)
{
    if (dst_size == 0) {
        return;
    }

    snprintf(dst, dst_size, "%s", src);
}

void config_set_default(gateway_config_t *cfg)
{
    if (cfg == NULL) {
        return;
    }

    copy_string(cfg->device_id, sizeof(cfg->device_id), "water_gateway_001");
    cfg->sample_period_ms = 1000;
    copy_string(cfg->log_level, sizeof(cfg->log_level), "info");
    copy_string(cfg->serial_device, sizeof(cfg->serial_device), "/dev/ttyUSB0");
    cfg->baudrate = 9600;
    cfg->modbus_slave_addr = 1;
    copy_string(cfg->db_path, sizeof(cfg->db_path), "water_gateway.db");
    cfg->max_cache_count = 100000;
}

int config_load(const char *path, gateway_config_t *cfg)
{
    FILE *fp;
    char line[256];
    int line_no = 0;

    if (path == NULL || cfg == NULL) {
        return -1;
    }

    config_set_default(cfg);

    fp = fopen(path, "r");
    if (fp == NULL) {
        perror("open config file failed");
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        char *key;
        char *value;
        char *equal;

        line_no++;

        key = trim(line);

        if (key[0] == '\0' || key[0] == '#' || key[0] == ';') {
            continue;
        }

        equal = strchr(key, '=');
        if (equal == NULL) {
            fprintf(stderr, "invalid config line %d: %s\n", line_no, key);
            continue;
        }

        *equal = '\0';
        value = equal + 1;

        key = trim(key);
        value = trim(value);

        if (strcmp(key, "device_id") == 0) {
            copy_string(cfg->device_id, sizeof(cfg->device_id), value);
        } else if (strcmp(key, "sample_period_ms") == 0) {
            cfg->sample_period_ms = atoi(value);
        } else if (strcmp(key, "log_level") == 0) {
            copy_string(cfg->log_level, sizeof(cfg->log_level), value);
        } else if (strcmp(key, "serial_device") == 0) {
            copy_string(cfg->serial_device, sizeof(cfg->serial_device), value);
        } else if (strcmp(key, "baudrate") == 0) {
            cfg->baudrate = atoi(value);
        } else if (strcmp(key, "modbus_slave_addr") == 0) {
            cfg->modbus_slave_addr = atoi(value);
        } else if (strcmp(key, "db_path") == 0) {
            copy_string(cfg->db_path, sizeof(cfg->db_path), value);
        } else if (strcmp(key, "max_cache_count") == 0) {
            cfg->max_cache_count = atoi(value);
        } else {
            fprintf(stderr, "unknown config key at line %d: %s\n", line_no, key);
        }
    }

    fclose(fp);

    if (cfg->sample_period_ms <= 0) {
        fprintf(stderr, "invalid sample_period_ms, use default 1000\n");
        cfg->sample_period_ms = 1000;
    }

    if (cfg->baudrate <= 0) {
        fprintf(stderr, "invalid baudrate, use default 9600\n");
        cfg->baudrate = 9600;
    }

    if (cfg->modbus_slave_addr < 1 || cfg->modbus_slave_addr > 247) {
        fprintf(stderr, "invalid modbus_slave_addr, use default 1\n");
        cfg->modbus_slave_addr = 1;
    }

    if (cfg->max_cache_count <= 0) {
        cfg->max_cache_count = 100000;
    }

    return 0;
}

void config_print(const gateway_config_t *cfg)
{
    if (cfg == NULL) {
        return;
    }

    printf("config:\n");
    printf("  device_id        = %s\n", cfg->device_id);
    printf("  sample_period_ms = %d\n", cfg->sample_period_ms);
    printf("  log_level        = %s\n", cfg->log_level);
    printf("  serial_device    = %s\n", cfg->serial_device);
    printf("  baudrate         = %d\n", cfg->baudrate);
    printf("  modbus_slave_addr= %d\n", cfg->modbus_slave_addr);
    printf("  db_path         = %s\n", cfg->db_path);
    printf("  max_cache_count = %d\n", cfg->max_cache_count);
}




