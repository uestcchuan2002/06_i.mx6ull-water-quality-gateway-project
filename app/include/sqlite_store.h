#ifndef __SQLITE_STORE_H
#define __SQLITE_STORE_H

#include "sample.h"
#include "sample_queue.h"
#include <sqlite3.h>

typedef struct {
    sample_queue_t *store_queue;
    sqlite3 *db;
    const char *device_id;
    int max_cache_count;
    int sample_counter;
    unsigned long write_count;
    unsigned long write_fail_count;
    volatile int shutdown;
} sqlite_store_ctx_t;

int sqlite_store_open(const char *db_path, sqlite3 **db);
int sqlite_store_create_table(sqlite3 *db);
int sqlite_store_insert(sqlite3 *db, const char *device_id,
                        const water_sample_t *sample);
int sqlite_store_get_unuploaded_count(sqlite3 *db);
int sqlite_store_get_total_count(sqlite3 *db);
int sqlite_store_mark_uploaded(sqlite3 *db, int sample_id);
int sqlite_store_inc_retry(sqlite3 *db, int sample_id);
int sqlite_store_trim_cache(sqlite3 *db, int max_count);
void sqlite_store_close(sqlite3 *db);

void *sqlite_store_thread(void *arg);

#endif
