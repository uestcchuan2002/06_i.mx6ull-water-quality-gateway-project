#ifndef __SAMPLE_QUEUE_H
#define __SAMPLE_QUEUE_H

#include "sample.h"
#include <pthread.h>

typedef struct {
    water_sample_t *samples;
    int capacity;
    int head;
    int tail;
    int count;

    pthread_mutex_t mutex;
    pthread_cond_t cond_not_empty;
    pthread_cond_t cond_not_full;

    int shutdown;
    unsigned long overflow_count;
    unsigned long push_count;
    unsigned long pop_count;
} sample_queue_t;

sample_queue_t *sample_queue_create(int capacity);
void sample_queue_destroy(sample_queue_t *q);

int sample_queue_push(sample_queue_t *q, const water_sample_t *sample,
                      int timeout_ms);

int sample_queue_pop(sample_queue_t *q, water_sample_t *sample,
                     int timeout_ms);

void sample_queue_shutdown(sample_queue_t *q);

int sample_queue_count(sample_queue_t *q);
unsigned long sample_queue_overflow_count(sample_queue_t *q);
unsigned long sample_queue_push_count(sample_queue_t *q);
unsigned long sample_queue_pop_count(sample_queue_t *q);

#endif
