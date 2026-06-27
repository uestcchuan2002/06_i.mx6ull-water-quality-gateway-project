#include "sample_queue.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

sample_queue_t *sample_queue_create(int capacity)
{
    sample_queue_t *q;

    if (capacity <= 0) {
        return NULL;
    }

    q = (sample_queue_t *)calloc(1, sizeof(*q));
    if (q == NULL) {
        return NULL;
    }

    q->samples = (water_sample_t *)calloc((size_t)capacity, sizeof(water_sample_t));
    if (q->samples == NULL) {
        free(q);
        return NULL;
    }

    q->capacity = capacity;
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    q->shutdown = 0;
    q->overflow_count = 0;
    q->push_count = 0;
    q->pop_count = 0;

    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond_not_empty, NULL);
    pthread_cond_init(&q->cond_not_full, NULL);

    return q;
}

void sample_queue_destroy(sample_queue_t *q)
{
    if (q == NULL) {
        return;
    }

    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond_not_empty);
    pthread_cond_destroy(&q->cond_not_full);

    free(q->samples);
    free(q);
}

static void timespec_add_ms(struct timespec *ts, int timeout_ms)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    ts->tv_sec = tv.tv_sec + (tv.tv_usec / 1000000) + (timeout_ms / 1000);
    ts->tv_nsec = ((tv.tv_usec % 1000000) * 1000) + ((timeout_ms % 1000) * 1000000);
    if (ts->tv_nsec >= 1000000000) {
        ts->tv_sec += 1;
        ts->tv_nsec -= 1000000000;
    }
}

int sample_queue_push(sample_queue_t *q, const water_sample_t *sample,
                      int timeout_ms)
{
    int ret = -1;

    if (q == NULL || sample == NULL) {
        return -1;
    }

    pthread_mutex_lock(&q->mutex);

    while (q->count >= q->capacity && !q->shutdown) {
        struct timespec ts;

        if (timeout_ms <= 0) {
            break;
        }

        timespec_add_ms(&ts, timeout_ms);
        if (pthread_cond_timedwait(&q->cond_not_full, &q->mutex, &ts) == ETIMEDOUT) {
            break;
        }
    }

    if (q->shutdown) {
        pthread_mutex_unlock(&q->mutex);
        return -1;
    }

    if (q->count >= q->capacity) {
        q->tail = (q->tail + 1) % q->capacity;
        q->count--;
        q->overflow_count++;
    }

    memcpy(&q->samples[q->head], sample, sizeof(water_sample_t));
    q->head = (q->head + 1) % q->capacity;
    q->count++;
    q->push_count++;
    ret = 0;

    pthread_cond_signal(&q->cond_not_empty);
    pthread_mutex_unlock(&q->mutex);

    return ret;
}

int sample_queue_pop(sample_queue_t *q, water_sample_t *sample,
                     int timeout_ms)
{
    int ret = -1;

    if (q == NULL || sample == NULL) {
        return -1;
    }

    pthread_mutex_lock(&q->mutex);

    while (q->count == 0 && !q->shutdown) {
        struct timespec ts;

        if (timeout_ms <= 0) {
            pthread_mutex_unlock(&q->mutex);
            return -1;
        }

        timespec_add_ms(&ts, timeout_ms);
        if (pthread_cond_timedwait(&q->cond_not_empty, &q->mutex, &ts) == ETIMEDOUT) {
            pthread_mutex_unlock(&q->mutex);
            return -1;
        }
    }

    if (q->shutdown && q->count == 0) {
        pthread_mutex_unlock(&q->mutex);
        return -1;
    }

    memcpy(sample, &q->samples[q->tail], sizeof(water_sample_t));
    q->tail = (q->tail + 1) % q->capacity;
    q->count--;
    q->pop_count++;
    ret = 0;

    pthread_cond_signal(&q->cond_not_full);
    pthread_mutex_unlock(&q->mutex);

    return ret;
}

void sample_queue_shutdown(sample_queue_t *q)
{
    if (q == NULL) {
        return;
    }

    pthread_mutex_lock(&q->mutex);
    q->shutdown = 1;
    pthread_cond_broadcast(&q->cond_not_empty);
    pthread_cond_broadcast(&q->cond_not_full);
    pthread_mutex_unlock(&q->mutex);
}

int sample_queue_count(sample_queue_t *q)
{
    int count;

    if (q == NULL) {
        return 0;
    }

    pthread_mutex_lock(&q->mutex);
    count = q->count;
    pthread_mutex_unlock(&q->mutex);

    return count;
}

unsigned long sample_queue_overflow_count(sample_queue_t *q)
{
    unsigned long count;

    if (q == NULL) {
        return 0;
    }

    pthread_mutex_lock(&q->mutex);
    count = q->overflow_count;
    pthread_mutex_unlock(&q->mutex);

    return count;
}

unsigned long sample_queue_push_count(sample_queue_t *q)
{
    unsigned long count;

    if (q == NULL) {
        return 0;
    }

    pthread_mutex_lock(&q->mutex);
    count = q->push_count;
    pthread_mutex_unlock(&q->mutex);

    return count;
}

unsigned long sample_queue_pop_count(sample_queue_t *q)
{
    unsigned long count;

    if (q == NULL) {
        return 0;
    }

    pthread_mutex_lock(&q->mutex);
    count = q->pop_count;
    pthread_mutex_unlock(&q->mutex);

    return count;
}
