/*
 * @Author: uestcchuan2002 1992735052@qq.com
 * @Date: 2026-06-23 18:05:54
 * @LastEditors: uestcchuan2002 1992735052@qq.com
 * @LastEditTime: 2026-06-23 19:21:09
 * @FilePath: /03_water_quality_gateway_project/app/include/config.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __CONFIG_H
#define __CONFIG_H

typedef struct {
    char device_id[64];
    int sample_period_ms;
    char log_level[16];
    char serial_device[128];
    int baudrate;
    int modbus_slave_addr;
} gateway_config_t;


void config_set_default(gateway_config_t *cfg);
int config_load(const char *path, gateway_config_t *cfg);
void config_print(const gateway_config_t *cfg);



#endif
