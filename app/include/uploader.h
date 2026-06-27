#ifndef __UPLOADER_H
#define __UPLOADER_H

#include "sample.h"
#include <sqlite3.h>

typedef struct {
    sqlite3 *db;
    const char *device_id;
    const char *server_host;
    int server_port;
    int upload_period_ms;
    int upload_batch_max;
    int upload_retry_max;
    unsigned long uploaded_count;
    unsigned long failed_count;
    volatile int shutdown;
} uploader_ctx_t;

void *uploader_thread(void *arg);

#endif
