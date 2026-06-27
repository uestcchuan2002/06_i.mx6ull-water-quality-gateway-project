#include "../include/sqlite_store.h"
#include "../include/logger.h"

#include <stdio.h>
#include <string.h>

int sqlite_store_open(const char *db_path, sqlite3 **db)
{
    int rc;

    if (db_path == NULL || db == NULL) {
        return -1;
    }

    rc = sqlite3_open(db_path, db);
    if (rc != SQLITE_OK) {
        log_error("sqlite3_open failed: %s", sqlite3_errmsg(*db));
        sqlite3_close(*db);
        *db = NULL;
        return -1;
    }

    sqlite3_exec(*db, "PRAGMA journal_mode=WAL;", NULL, NULL, NULL);
    sqlite3_exec(*db, "PRAGMA synchronous=NORMAL;", NULL, NULL, NULL);

    log_info("sqlite database opened: %s", db_path);
    return 0;
}

int sqlite_store_create_table(sqlite3 *db)
{
    const char *sql;
    char *errmsg = NULL;
    int rc;

    if (db == NULL) {
        return -1;
    }

    sql = "CREATE TABLE IF NOT EXISTS samples ("
          "id INTEGER PRIMARY KEY AUTOINCREMENT,"
          "device_id TEXT NOT NULL,"
          "timestamp_ms INTEGER NOT NULL,"
          "ph REAL NOT NULL,"
          "temperature REAL NOT NULL,"
          "turbidity REAL NOT NULL,"
          "conductivity REAL NOT NULL,"
          "sensor_status INTEGER NOT NULL,"
          "alarm_status INTEGER NOT NULL,"
          "sequence INTEGER NOT NULL,"
          "uploaded INTEGER NOT NULL DEFAULT 0,"
          "upload_retry INTEGER NOT NULL DEFAULT 0,"
          "created_at TEXT DEFAULT CURRENT_TIMESTAMP"
          ");";

    rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        log_error("create table failed: %s", errmsg);
        sqlite3_free(errmsg);
        return -1;
    }

    sql = "CREATE INDEX IF NOT EXISTS idx_samples_uploaded "
          "ON samples(uploaded);";
    rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        log_error("create index failed: %s", errmsg);
        sqlite3_free(errmsg);
        return -1;
    }

    sql = "CREATE INDEX IF NOT EXISTS idx_samples_timestamp "
          "ON samples(timestamp_ms);";
    sqlite3_exec(db, sql, NULL, NULL, &errmsg);

    log_info("sqlite table 'samples' ready");
    return 0;
}

int sqlite_store_insert(sqlite3 *db, const char *device_id,
                        const water_sample_t *sample)
{
    const char *sql;
    sqlite3_stmt *stmt = NULL;
    int rc;

    if (db == NULL || device_id == NULL || sample == NULL) {
        return -1;
    }

    sql = "INSERT INTO samples "
          "(device_id, timestamp_ms, ph, temperature, turbidity, "
          "conductivity, sensor_status, alarm_status, sequence) "
          "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        log_error("sqlite3_prepare_v2 (insert) failed: %s", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_text(stmt, 1, device_id, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, sample->timestamp_ms);
    sqlite3_bind_double(stmt, 3, (double)sample->ph);
    sqlite3_bind_double(stmt, 4, (double)sample->temperature);
    sqlite3_bind_double(stmt, 5, (double)sample->turbidity);
    sqlite3_bind_double(stmt, 6, (double)sample->conductivity);
    sqlite3_bind_int(stmt, 7, (int)sample->sensor_status);
    sqlite3_bind_int(stmt, 8, (int)sample->alarm_status);
    sqlite3_bind_int(stmt, 9, (int)sample->sequence);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        log_error("sqlite insert failed: %s", sqlite3_errmsg(db));
        return -1;
    }

    return 0;
}

int sqlite_store_get_unuploaded_count(sqlite3 *db)
{
    const char *sql;
    sqlite3_stmt *stmt = NULL;
    int count = 0;

    if (db == NULL) {
        return -1;
    }

    sql = "SELECT COUNT(*) FROM samples WHERE uploaded = 0;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return count;
}

int sqlite_store_get_total_count(sqlite3 *db)
{
    const char *sql;
    sqlite3_stmt *stmt = NULL;
    int count = 0;

    if (db == NULL) {
        return -1;
    }

    sql = "SELECT COUNT(*) FROM samples;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return count;
}

int sqlite_store_mark_uploaded(sqlite3 *db, int sample_id)
{
    const char *sql;
    sqlite3_stmt *stmt = NULL;
    int rc;

    if (db == NULL) {
        return -1;
    }

    sql = "UPDATE samples SET uploaded = 1 WHERE id = ?;";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_int(stmt, 1, sample_id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int sqlite_store_inc_retry(sqlite3 *db, int sample_id)
{
    const char *sql;
    sqlite3_stmt *stmt = NULL;
    int rc;

    if (db == NULL) {
        return -1;
    }

    sql = "UPDATE samples SET upload_retry = upload_retry + 1 WHERE id = ?;";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_int(stmt, 1, sample_id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int sqlite_store_trim_cache(sqlite3 *db, int max_count)
{
    char sql[256];
    char *errmsg = NULL;
    int total;
    int to_delete;

    if (db == NULL || max_count <= 0) {
        return -1;
    }

    total = sqlite_store_get_total_count(db);
    if (total < 0 || total <= max_count) {
        return 0;
    }

    to_delete = total - max_count;

    snprintf(sql, sizeof(sql),
             "DELETE FROM samples WHERE id IN "
             "(SELECT id FROM samples WHERE uploaded = 1 "
             "ORDER BY id ASC LIMIT %d);",
             to_delete);

    if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
        log_error("trim cache failed: %s", errmsg);
        sqlite3_free(errmsg);
        return -1;
    }

    total = sqlite_store_get_total_count(db);
    if (total > max_count) {
        to_delete = total - max_count;
        snprintf(sql, sizeof(sql),
                 "DELETE FROM samples WHERE id IN "
                 "(SELECT id FROM samples ORDER BY id ASC LIMIT %d);",
                 to_delete);
        sqlite3_exec(db, sql, NULL, NULL, NULL);
    }

    return 0;
}

void sqlite_store_close(sqlite3 *db)
{
    if (db == NULL) {
        return;
    }

    sqlite3_close(db);
    log_info("sqlite database closed");
}

void *sqlite_store_thread(void *arg)
{
    sqlite_store_ctx_t *ctx = (sqlite_store_ctx_t *)arg;

    if (ctx == NULL || ctx->store_queue == NULL || ctx->db == NULL) {
        return NULL;
    }

    log_info("store thread started");

    while (!ctx->shutdown) {
        water_sample_t sample;

        if (sample_queue_pop(ctx->store_queue, &sample, 500) != 0) {
            continue;
        }

        ctx->sample_counter++;

        if (sqlite_store_insert(ctx->db, ctx->device_id, &sample) == 0) {
            ctx->write_count++;
        } else {
            ctx->write_fail_count++;
        }

        if (ctx->max_cache_count > 0 && ctx->sample_counter % 100 == 0) {
            sqlite_store_trim_cache(ctx->db, ctx->max_cache_count);
        }
    }

    while (sample_queue_count(ctx->store_queue) > 0) {
        water_sample_t sample;

        if (sample_queue_pop(ctx->store_queue, &sample, 100) != 0) {
            break;
        }

        ctx->sample_counter++;

        if (sqlite_store_insert(ctx->db, ctx->device_id, &sample) == 0) {
            ctx->write_count++;
        } else {
            ctx->write_fail_count++;
        }
    }

    sqlite_store_trim_cache(ctx->db, ctx->max_cache_count);

    log_info("store thread stopped (processed %d samples, written %lu, failed %lu)",
             ctx->sample_counter, ctx->write_count, ctx->write_fail_count);

    return NULL;
}
