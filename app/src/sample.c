#include "sample.h"

#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/time.h>
#endif

static unsigned int g_mock_sequence = 0;

static long long sample_now_ms(void)
{
#if defined(_WIN32)
    FILETIME file_time;
    ULARGE_INTEGER value;
    const unsigned long long windows_to_unix_epoch = 116444736000000000ULL;

    GetSystemTimeAsFileTime(&file_time);
    value.LowPart = file_time.dwLowDateTime;
    value.HighPart = file_time.dwHighDateTime;

    return (long long)((value.QuadPart - windows_to_unix_epoch) / 10000ULL);
#else
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000LL + (long long)tv.tv_usec / 1000LL;
#endif
}

void sample_init(water_sample_t *sample)
{
    if (sample == NULL) {
        return;
    }

    memset(sample, 0, sizeof(*sample));
    sample->timestamp_ms = sample_now_ms();
}

void sample_generate_mock(water_sample_t *sample)
{
    unsigned int seq;

    if (sample == NULL) {
        return;
    }

    seq = ++g_mock_sequence;

    sample->timestamp_ms = sample_now_ms();
    sample->ph = 7.00f + (float)(seq % 20) * 0.01f;
    sample->temperature = 25.00f + (float)(seq % 50) * 0.01f;
    sample->turbidity = 3.00f + (float)(seq % 30) * 0.01f;
    sample->conductivity = 800.0f + (float)(seq % 40);
    sample->sensor_status = 0;
    sample->alarm_status = 0;
    sample->sequence = seq;
}

int sample_to_string(const water_sample_t *sample, char *buf, size_t buf_size)
{
    if (sample == NULL || buf == NULL || buf_size == 0) {
        return -1;
    }

    return snprintf(buf,
                    buf_size,
                    "timestamp_ms=%lld ph=%.2f temperature=%.2f turbidity=%.2f "
                    "conductivity=%.2f sensor_status=0x%X alarm_status=0x%X seq=%u",
                    sample->timestamp_ms,
                    sample->ph,
                    sample->temperature,
                    sample->turbidity,
                    sample->conductivity,
                    sample->sensor_status,
                    sample->alarm_status,
                    sample->sequence);
}
