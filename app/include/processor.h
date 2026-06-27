#ifndef __PROCESSOR_H
#define __PROCESSOR_H

#include "sample.h"
#include "sample_queue.h"

typedef struct {
    float ph_min;
    float ph_max;
    float temp_min;
    float temp_max;
    float turb_max;
    float cond_max;
} processor_threshold_t;

void processor_threshold_default(processor_threshold_t *th);

unsigned int processor_compute_alarm(const water_sample_t *sample,
                                     const processor_threshold_t *th);

typedef struct {
    sample_queue_t *raw_queue;
    sample_queue_t *store_queue;
    processor_threshold_t thresholds;
    int sample_counter;
    volatile int shutdown;
} processor_ctx_t;

void *processor_thread(void *arg);

#endif
