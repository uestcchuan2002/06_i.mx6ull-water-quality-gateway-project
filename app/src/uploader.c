#include "../include/uploader.h"
#include "../include/logger.h"
#include "../include/sqlite_store.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

static int uploader_build_json(const char *device_id,
                               const water_sample_t *sample,
                               char *buf, size_t buf_size)
{
    return snprintf(buf, buf_size,
        "{\"device_id\":\"%s\",\"timestamp_ms\":%lld,"
        "\"ph\":%.2f,\"temperature\":%.2f,"
        "\"turbidity\":%.2f,\"conductivity\":%d,"
        "\"sensor_status\":%u,\"alarm_status\":%u,"
        "\"sequence\":%u}\n",
        device_id,
        (long long)sample->timestamp_ms,
        sample->ph,
        sample->temperature,
        sample->turbidity,
        (int)sample->conductivity,
        sample->sensor_status,
        sample->alarm_status,
        sample->sequence);
}

static int uploader_connect(const char *host, int port)
{
    int sock;
    struct sockaddr_in addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        log_warn("uploader: socket() failed: %s", strerror(errno));
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);

    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        log_warn("uploader: inet_pton() failed for %s", host);
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        log_warn("uploader: connect() failed: %s", strerror(errno));
        close(sock);
        return -1;
    }

    log_info("uploader: connected to %s:%d", host, port);
    return sock;
}

static int uploader_send_line(int sock, const char *data, int len)
{
    int sent = 0;
    int n;

    while (sent < len) {
        n = (int)send(sock, data + sent, (size_t)(len - sent), MSG_NOSIGNAL);
        if (n <= 0) {
            log_warn("uploader: send() failed: %s", strerror(errno));
            return -1;
        }
        sent += n;
    }

    return 0;
}

static int uploader_query_unuploaded(sqlite3 *db, int batch_max,
                                     int retry_max,
                                     sqlite3_stmt **stmt_out)
{
    const char *sql =
        "SELECT id, timestamp_ms, ph, temperature, turbidity, "
        "conductivity, sensor_status, alarm_status, sequence "
        "FROM samples "
        "WHERE uploaded = 0 AND upload_retry < ? "
        "ORDER BY id ASC LIMIT ?;";

    if (sqlite3_prepare_v2(db, sql, -1, stmt_out, NULL) != SQLITE_OK) {
        log_error("uploader: prepare query failed: %s", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_int(*stmt_out, 1, retry_max);
    sqlite3_bind_int(*stmt_out, 2, batch_max);

    return 0;
}

static int uploader_read_row(sqlite3_stmt *stmt, int *id_out,
                             water_sample_t *sample_out)
{
    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        return (rc == SQLITE_DONE) ? 0 : -1;
    }

    *id_out = sqlite3_column_int(stmt, 0);

    memset(sample_out, 0, sizeof(*sample_out));
    sample_out->timestamp_ms = (long long)sqlite3_column_int64(stmt, 1);
    sample_out->ph          = (float)sqlite3_column_double(stmt, 2);
    sample_out->temperature = (float)sqlite3_column_double(stmt, 3);
    sample_out->turbidity   = (float)sqlite3_column_double(stmt, 4);
    sample_out->conductivity = (float)sqlite3_column_double(stmt, 5);
    sample_out->sensor_status = (unsigned int)sqlite3_column_int(stmt, 6);
    sample_out->alarm_status  = (unsigned int)sqlite3_column_int(stmt, 7);
    sample_out->sequence      = (unsigned int)sqlite3_column_int(stmt, 8);

    return 1;
}

void *uploader_thread(void *arg)
{
    uploader_ctx_t *ctx = (uploader_ctx_t *)arg;
    int sock = -1;
    int connected = 0;

    if (ctx == NULL || ctx->db == NULL) {
        log_error("uploader: invalid context");
        return NULL;
    }

    log_info("upload thread started (host=%s:%d period=%d batch=%d retry=%d)",
             ctx->server_host, ctx->server_port,
             ctx->upload_period_ms, ctx->upload_batch_max,
             ctx->upload_retry_max);

    while (!ctx->shutdown) {
        if (!connected) {
            sock = uploader_connect(ctx->server_host, ctx->server_port);
            if (sock < 0) {
                usleep((unsigned int)ctx->upload_period_ms * 1000U);
                continue;
            }
            connected = 1;
        }

        {
            sqlite3_stmt *stmt = NULL;
            int has_data = 0;

            if (uploader_query_unuploaded(ctx->db,
                                          ctx->upload_batch_max,
                                          ctx->upload_retry_max,
                                          &stmt) != 0) {
                usleep((unsigned int)ctx->upload_period_ms * 1000U);
                continue;
            }

            while (!ctx->shutdown) {
                int sample_id;
                water_sample_t sample;
                char json[512];
                int len;

                int rc = uploader_read_row(stmt, &sample_id, &sample);
                if (rc == 0) {
                    break;
                }
                if (rc < 0) {
                    log_error("uploader: read row failed: %s",
                              sqlite3_errmsg(ctx->db));
                    break;
                }

                has_data = 1;
                len = uploader_build_json(ctx->device_id, &sample,
                                          json, sizeof(json));
                if (len < 0 || (size_t)len >= sizeof(json)) {
                    log_warn("uploader: json build overflow");
                    continue;
                }

                if (uploader_send_line(sock, json, len) != 0) {
                    sqlite_store_inc_retry(ctx->db, sample_id);
                    ctx->failed_count++;
                    connected = 0;
                    close(sock);
                    sock = -1;
                    log_warn("uploader: connection lost, will reconnect");
                    break;
                }

                sqlite_store_mark_uploaded(ctx->db, sample_id);
                ctx->uploaded_count++;
            }

            sqlite3_finalize(stmt);

            if (!has_data) {
                usleep((unsigned int)ctx->upload_period_ms * 1000U);
            }
        }
    }

    if (connected && sock >= 0) {
        log_info("uploader: draining remaining unuploaded records...");
        for (;;) {
            sqlite3_stmt *stmt = NULL;
            int drained = 0;

            if (uploader_query_unuploaded(ctx->db,
                                          ctx->upload_batch_max,
                                          ctx->upload_retry_max,
                                          &stmt) != 0) {
                break;
            }

            while (1) {
                int sample_id;
                water_sample_t sample;
                char json[512];
                int len;

                int rc = uploader_read_row(stmt, &sample_id, &sample);
                if (rc == 0) {
                    break;
                }
                if (rc < 0) {
                    break;
                }

                drained = 1;
                len = uploader_build_json(ctx->device_id, &sample,
                                          json, sizeof(json));
                if (len < 0 || (size_t)len >= sizeof(json)) {
                    continue;
                }

                if (uploader_send_line(sock, json, len) != 0) {
                    sqlite_store_inc_retry(ctx->db, sample_id);
                    ctx->failed_count++;
                    log_warn("uploader: send failed during drain");
                    break;
                }

                sqlite_store_mark_uploaded(ctx->db, sample_id);
                ctx->uploaded_count++;
            }

            sqlite3_finalize(stmt);

            if (!drained) {
                break;
            }
        }
    }

    if (connected && sock >= 0) {
        close(sock);
    }

    log_info("upload thread stopped (uploaded=%lu failed=%lu)",
             ctx->uploaded_count, ctx->failed_count);

    return NULL;
}
