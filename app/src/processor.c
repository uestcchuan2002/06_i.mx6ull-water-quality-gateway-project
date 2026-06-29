#include "processor.h"
#include "alarm_client.h"
#include "logger.h"
#include "config.h"

#include <stdio.h>
#include <unistd.h>

#define ALARM_PH_LOW      (1u << 0)
#define ALARM_PH_HIGH     (1u << 1)
#define ALARM_TURB_HIGH   (1u << 2)
#define ALARM_COND_HIGH   (1u << 3)
#define ALARM_TEMP_HIGH   (1u << 4)

void processor_threshold_default(processor_threshold_t *th)
{
    if (th == NULL) {
        return;
    }

    th->ph_min = 6.5f;
    th->ph_max = 8.5f;
    th->temp_min = 0.0f;
    th->temp_max = 45.0f;
    th->turb_max = 5.0f;
    th->cond_max = 2000.0f;
}

unsigned int processor_compute_alarm(const water_sample_t *sample,
                                     const processor_threshold_t *th)
{
    unsigned int alarm = 0;

    if (sample == NULL || th == NULL) {
        return 0;
    }

    if (sample->ph < th->ph_min) {
        alarm |= ALARM_PH_LOW;
    } else if (sample->ph > th->ph_max) {
        alarm |= ALARM_PH_HIGH;
    }

    if (sample->temperature > th->temp_max) {
        alarm |= ALARM_TEMP_HIGH;
    }

    if (sample->turbidity > th->turb_max) {
        alarm |= ALARM_TURB_HIGH;
    }

    if (sample->conductivity > th->cond_max) {
        alarm |= ALARM_COND_HIGH;
    }

    return alarm;
}

void *processor_thread(void *arg)
{
    processor_ctx_t *ctx = (processor_ctx_t *)arg;

    if (ctx == NULL || ctx->raw_queue == NULL) {
        return NULL;
    }

    log_info("processor thread started");

    while (!ctx->shutdown) {
        water_sample_t sample;
        char sample_buf[256];
        unsigned int alarm;
        int alarm_count = 0;

        if (sample_queue_pop(ctx->raw_queue, &sample, 500) != 0) {
            continue;
        }

        ctx->sample_counter++;

        alarm = processor_compute_alarm(&sample, &ctx->thresholds);

        alarm_client_set(ctx->alarm_fd, alarm ? 1 : 0);

        if (alarm) {
            sample.alarm_status |= alarm;

            sample_buf[0] = '\0';
            if (alarm & ALARM_PH_LOW) {
                alarm_count += snprintf(sample_buf, sizeof(sample_buf), "PH_LOW ");
            }
            if (alarm & ALARM_PH_HIGH) {
                alarm_count += snprintf(sample_buf + alarm_count,
                                        sizeof(sample_buf) - (size_t)alarm_count,
                                        "PH_HIGH ");
            }
            if (alarm & ALARM_TURB_HIGH) {
                alarm_count += snprintf(sample_buf + alarm_count,
                                        sizeof(sample_buf) - (size_t)alarm_count,
                                        "TURB_HIGH ");
            }
            if (alarm & ALARM_COND_HIGH) {
                alarm_count += snprintf(sample_buf + alarm_count,
                                        sizeof(sample_buf) - (size_t)alarm_count,
                                        "COND_HIGH ");
            }
            if (alarm & ALARM_TEMP_HIGH) {
                alarm_count += snprintf(sample_buf + alarm_count,
                                        sizeof(sample_buf) - (size_t)alarm_count,
                                        "TEMP_HIGH ");
            }

            log_warn("ALARM triggered: %s| ph=%.2f temp=%.2f turb=%.2f cond=%.1f",
                     sample_buf,
                     sample.ph,
                     sample.temperature,
                     sample.turbidity,
                     sample.conductivity);
        }

        sample_to_string(&sample, sample_buf, sizeof(sample_buf));
        log_info("[proc] #%d %s", ctx->sample_counter, sample_buf);

        if (ctx->store_queue != NULL) {
            sample_queue_push(ctx->store_queue, &sample, 100);
        }
    }

    log_info("processor thread stopped (processed %d samples)",
             ctx->sample_counter);

    return NULL;
}
