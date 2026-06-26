#ifndef __SAMPLE_H
#define __SAMPLE_H

#include <stddef.h>

typedef struct {
    long long timestamp_ms;
    float ph;
    float temperature;
    float turbidity;
    float conductivity;
    unsigned int sensor_status;
    unsigned int alarm_status;
    unsigned int sequence;
} water_sample_t;

void sample_init(water_sample_t *sample);
void sample_generate_mock(water_sample_t *sample);
void sample_from_modbus_regs(water_sample_t *sample, const unsigned short *regs, int reg_count);
int sample_to_string(const water_sample_t *sample, char *buf, size_t buf_size);

#endif
