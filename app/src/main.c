#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "../include/alarm_client.h"
#include "../include/config.h"
#include "../include/logger.h"
#include "../include/modbus_rtu.h"
#include "../include/processor.h"
#include "../include/sample.h"
#include "../include/sample_queue.h"
#include "../include/serial_port.h"
#include "../include/sqlite_store.h"
#include "../include/uploader.h"

#define DEFAULT_CONFIG_PATH "../config/gateway.conf"
#define WATER_GATEWAY_VERSION "0.4.0"
#define DEFAULT_QUEUE_CAPACITY 64
#define DEFAULT_TEST_ITERATIONS 10

static volatile sig_atomic_t g_shutdown = 0;

static void signal_handler(int sig)
{
    (void)sig;
    g_shutdown = 1;
}

static void print_usage(const char *program)
{
    printf("Usage: %s [options]\n", program);
    printf("  -c, --config <path>  config file path (default: %s)\n",
           DEFAULT_CONFIG_PATH);
    printf("  -h, --help           show help\n");
    printf("  -v, --version        show version\n");
    printf("  --test [N]           run thread pipeline test with N mock samples "
           "(default: %d)\n",
           DEFAULT_TEST_ITERATIONS);
}

static int str_to_int(const char *str, int default_val)
{
    int val;
    char *end = NULL;

    if (str == NULL) {
        return default_val;
    }

    val = (int)strtol(str, &end, 10);
    if (end == str || *end != '\0') {
        return default_val;
    }

    return val;
}

static void sleep_ms(int ms)
{
    if (ms <= 0) {
        return;
    }

#if defined(_WIN32)
    Sleep((DWORD)ms);
#else
    usleep((unsigned int)ms * 1000U);
#endif
}

typedef struct {
    gateway_config_t *cfg;
    int serial_fd;
    sample_queue_t *raw_queue;
} collect_ctx_t;

static void *collect_thread(void *arg)
{
    collect_ctx_t *ctx = (collect_ctx_t *)arg;
    int modbus_ok = 0;

    if (ctx == NULL || ctx->raw_queue == NULL) {
        return NULL;
    }

    log_info("collect thread started");

    while (!g_shutdown) {
        water_sample_t sample;
        unsigned short values[7];
        int reg_count = -1;

        if (ctx->serial_fd >= 0) {
            reg_count = modbus_read_registers(ctx->serial_fd,
                                              (unsigned char)ctx->cfg->modbus_slave_addr,
                                              0x0000, 7, values, 7, 500);
        }

        if (reg_count == 7) {
            sample_from_modbus_regs(&sample, values, 7);
            if (!modbus_ok) {
                log_info("modbus acquisition established");
                modbus_ok = 1;
            }
        } else {
            if (modbus_ok) {
                log_warn("modbus read lost, falling back to mock data");
                modbus_ok = 0;
            }
            sample_generate_mock(&sample);
        }

        sample_queue_push(ctx->raw_queue, &sample, 100);

        sleep_ms(ctx->cfg->sample_period_ms);
    }

    log_info("collect thread stopped");
    return NULL;
}

static void *collect_thread_mock(void *arg)
{
    collect_ctx_t *ctx = (collect_ctx_t *)arg;

    if (ctx == NULL || ctx->raw_queue == NULL) {
        return NULL;
    }

    log_info("collect thread started (mock only)");

    while (!g_shutdown) {
        water_sample_t sample;

        sample_generate_mock(&sample);

        sample_queue_push(ctx->raw_queue, &sample, 100);

        sleep_ms(ctx->cfg->sample_period_ms);
    }

    log_info("collect thread stopped");
    return NULL;
}

static int run_test(int iterations)
{
    sample_queue_t *raw_queue;
    sample_queue_t *store_queue;
    gateway_config_t cfg;
    collect_ctx_t collect_ctx;
    processor_ctx_t proc_ctx;
    sqlite_store_ctx_t store_ctx;
    uploader_ctx_t upload_ctx;
    sqlite3 *db = NULL;
    pthread_t collect_tid;
    pthread_t proc_tid;
    pthread_t store_tid;
    pthread_t upload_tid;
    int i;

    config_set_default(&cfg);
    cfg.sample_period_ms = 50;
    cfg.upload_period_ms = 200;
    cfg.upload_batch_max = 100;

    log_info("=== thread pipeline test: %d iterations ===", iterations);

    raw_queue = sample_queue_create(DEFAULT_QUEUE_CAPACITY);
    if (raw_queue == NULL) {
        log_error("failed to create raw_queue");
        return 1;
    }

    store_queue = sample_queue_create(DEFAULT_QUEUE_CAPACITY);
    if (store_queue == NULL) {
        log_error("failed to create store_queue");
        sample_queue_destroy(raw_queue);
        return 1;
    }

    if (sqlite_store_open(":memory:", &db) != 0) {
        log_error("failed to open in-memory database");
        sample_queue_destroy(raw_queue);
        sample_queue_destroy(store_queue);
        return 1;
    }

    if (sqlite_store_create_table(db) != 0) {
        log_error("failed to create table");
        sqlite_store_close(db);
        sample_queue_destroy(raw_queue);
        sample_queue_destroy(store_queue);
        return 1;
    }

    memset(&collect_ctx, 0, sizeof(collect_ctx));
    collect_ctx.cfg = &cfg;
    collect_ctx.serial_fd = -1;
    collect_ctx.raw_queue = raw_queue;

    memset(&proc_ctx, 0, sizeof(proc_ctx));
    proc_ctx.raw_queue = raw_queue;
    proc_ctx.store_queue = store_queue;
    proc_ctx.alarm_fd = -1;
    proc_ctx.shutdown = 0;
    processor_threshold_default(&proc_ctx.thresholds);

    memset(&store_ctx, 0, sizeof(store_ctx));
    store_ctx.store_queue = store_queue;
    store_ctx.db = db;
    store_ctx.device_id = cfg.device_id;
    store_ctx.max_cache_count = cfg.max_cache_count;
    store_ctx.shutdown = 0;

    if (pthread_create(&collect_tid, NULL, collect_thread_mock, &collect_ctx) != 0) {
        log_error("failed to create collect thread");
        sqlite_store_close(db);
        sample_queue_destroy(raw_queue);
        sample_queue_destroy(store_queue);
        return 1;
    }

    if (pthread_create(&proc_tid, NULL, processor_thread, &proc_ctx) != 0) {
        log_error("failed to create processor thread");
        g_shutdown = 1;
        pthread_join(collect_tid, NULL);
        sqlite_store_close(db);
        sample_queue_destroy(raw_queue);
        sample_queue_destroy(store_queue);
        return 1;
    }

    if (pthread_create(&store_tid, NULL, sqlite_store_thread, &store_ctx) != 0) {
        log_error("failed to create store thread");
        g_shutdown = 1;
        proc_ctx.shutdown = 1;
        sample_queue_shutdown(raw_queue);
        sample_queue_shutdown(store_queue);
        pthread_join(collect_tid, NULL);
        pthread_join(proc_tid, NULL);
        sqlite_store_close(db);
        sample_queue_destroy(raw_queue);
        sample_queue_destroy(store_queue);
        return 1;
    }

    memset(&upload_ctx, 0, sizeof(upload_ctx));
    upload_ctx.db = db;
    upload_ctx.device_id = cfg.device_id;
    upload_ctx.server_host = cfg.upload_server_host;
    upload_ctx.server_port = cfg.upload_server_port;
    upload_ctx.upload_period_ms = cfg.upload_period_ms;
    upload_ctx.upload_batch_max = cfg.upload_batch_max;
    upload_ctx.upload_retry_max = cfg.upload_retry_max;
    upload_ctx.shutdown = 0;

    if (pthread_create(&upload_tid, NULL, uploader_thread, &upload_ctx) != 0) {
        log_error("failed to create upload thread");
    }

    for (i = 0; i < iterations; i++) {
        sleep_ms(100);
    }

    g_shutdown = 1;
    proc_ctx.shutdown = 1;
    store_ctx.shutdown = 1;
    upload_ctx.shutdown = 1;
    sample_queue_shutdown(raw_queue);
    sample_queue_shutdown(store_queue);

    pthread_join(collect_tid, NULL);
    pthread_join(proc_tid, NULL);
    pthread_join(store_tid, NULL);
    pthread_join(upload_tid, NULL);

    log_info("=== test complete ===");
    log_info("raw_queue:    pushed=%lu overflow=%lu",
             sample_queue_push_count(raw_queue),
             sample_queue_overflow_count(raw_queue));
    log_info("store_queue:  pushed=%lu overflow=%lu final_count=%d",
             sample_queue_push_count(store_queue),
             sample_queue_overflow_count(store_queue),
             sample_queue_count(store_queue));
    log_info("store_thread: written=%lu failed=%lu",
             store_ctx.write_count, store_ctx.write_fail_count);
    log_info("db:           total_samples=%d unuploaded=%d",
             sqlite_store_get_total_count(db),
             sqlite_store_get_unuploaded_count(db));
    log_info("upload_thread: uploaded=%lu failed=%lu",
             upload_ctx.uploaded_count, upload_ctx.failed_count);

    sqlite_store_close(db);
    sample_queue_destroy(raw_queue);
    sample_queue_destroy(store_queue);

    return 0;
}

int main(int argc, char *argv[])
{
    gateway_config_t cfg;
    const char *config_path = DEFAULT_CONFIG_PATH;
    int i;
    int test_iterations = 0;
    int fd = -1;
    int alarm_fd = -1;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--config") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "missing config path after %s\n", argv[i]);
                return 1;
            }
            config_path = argv[++i];
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("water_gateway version %s\n", WATER_GATEWAY_VERSION);
            return 0;
        } else if (strcmp(argv[i], "--test") == 0) {
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                test_iterations = str_to_int(argv[++i], DEFAULT_TEST_ITERATIONS);
            } else {
                test_iterations = DEFAULT_TEST_ITERATIONS;
            }
        } else {
            fprintf(stderr, "unknown argument: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    logger_init("info");
    log_info("water gateway start (v%s)", WATER_GATEWAY_VERSION);
    log_info("config_path=%s", config_path);

    if (config_load(config_path, &cfg) != 0) {
        log_error("load config failed: %s", config_path);
        return 1;
    }

    logger_init(cfg.log_level);

    log_info("config loaded");
    log_info("device_id=%s", cfg.device_id);
    log_info("sample_period_ms=%d", cfg.sample_period_ms);
    log_info("serial_device=%s", cfg.serial_device);
    log_info("baudrate=%d", cfg.baudrate);
    log_info("modbus_slave_addr=%d", cfg.modbus_slave_addr);
    log_info("db_path=%s", cfg.db_path);
    log_info("max_cache_count=%d", cfg.max_cache_count);
    log_info("upload_enabled=%d", cfg.upload_enabled);
    log_info("upload_protocol=%s", cfg.upload_protocol);
    log_info("upload_server=%s:%d", cfg.upload_server_host, cfg.upload_server_port);
    log_info("upload_period_ms=%d batch=%d retry=%d",
             cfg.upload_period_ms, cfg.upload_batch_max, cfg.upload_retry_max);
    log_info("log_file=%s", cfg.log_file);
    log_info("alarm_device=%s", cfg.alarm_device);

    if (test_iterations > 0) {
        return run_test(test_iterations);
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN);

    logger_open_log_file(cfg.log_file);

    fd = serial_open(cfg.serial_device, cfg.baudrate);
    if (fd < 0) {
        log_warn("serial open failed: %s, running with mock data",
                 cfg.serial_device);
    } else {
        log_info("serial open success: %s", cfg.serial_device);
    }

    alarm_fd = alarm_client_open(cfg.alarm_device);

    {
        sample_queue_t *raw_queue;
        sample_queue_t *store_queue;
        collect_ctx_t collect_ctx;
        processor_ctx_t proc_ctx;
        sqlite_store_ctx_t store_ctx;
        uploader_ctx_t upload_ctx;
        sqlite3 *db = NULL;
        pthread_t collect_tid;
        pthread_t proc_tid;
        pthread_t store_tid;
        pthread_t upload_tid;

        raw_queue = sample_queue_create(DEFAULT_QUEUE_CAPACITY);
        store_queue = sample_queue_create(DEFAULT_QUEUE_CAPACITY);

        if (raw_queue == NULL || store_queue == NULL) {
            log_error("failed to create queues");
            if (fd >= 0) serial_close(fd);
            alarm_client_close(alarm_fd);
            return 1;
        }

        if (sqlite_store_open(cfg.db_path, &db) != 0) {
            log_error("failed to open database: %s", cfg.db_path);
            sample_queue_destroy(raw_queue);
            sample_queue_destroy(store_queue);
            if (fd >= 0) serial_close(fd);
            alarm_client_close(alarm_fd);
            return 1;
        }

        if (sqlite_store_create_table(db) != 0) {
            log_error("failed to create table");
            sqlite_store_close(db);
            sample_queue_destroy(raw_queue);
            sample_queue_destroy(store_queue);
            if (fd >= 0) serial_close(fd);
            alarm_client_close(alarm_fd);
            return 1;
        }

        memset(&collect_ctx, 0, sizeof(collect_ctx));
        collect_ctx.cfg = &cfg;
        collect_ctx.serial_fd = fd;
        collect_ctx.raw_queue = raw_queue;

        memset(&proc_ctx, 0, sizeof(proc_ctx));
        proc_ctx.raw_queue = raw_queue;
        proc_ctx.store_queue = store_queue;
        proc_ctx.alarm_fd = alarm_fd;
        proc_ctx.shutdown = 0;
        processor_threshold_default(&proc_ctx.thresholds);

        memset(&store_ctx, 0, sizeof(store_ctx));
        store_ctx.store_queue = store_queue;
        store_ctx.db = db;
        store_ctx.device_id = cfg.device_id;
        store_ctx.max_cache_count = cfg.max_cache_count;
        store_ctx.shutdown = 0;

        log_info("=== starting multi-threaded pipeline ===");

        if (pthread_create(&collect_tid, NULL, collect_thread, &collect_ctx) != 0) {
            log_error("failed to create collect thread");
            sqlite_store_close(db);
            sample_queue_destroy(raw_queue);
            sample_queue_destroy(store_queue);
            if (fd >= 0) serial_close(fd);
            alarm_client_close(alarm_fd);
            return 1;
        }

        if (pthread_create(&proc_tid, NULL, processor_thread, &proc_ctx) != 0) {
            log_error("failed to create processor thread");
            g_shutdown = 1;
            sample_queue_shutdown(raw_queue);
            pthread_join(collect_tid, NULL);
            sqlite_store_close(db);
            sample_queue_destroy(raw_queue);
            sample_queue_destroy(store_queue);
            if (fd >= 0) serial_close(fd);
            alarm_client_close(alarm_fd);
            return 1;
        }

        if (pthread_create(&store_tid, NULL, sqlite_store_thread, &store_ctx) != 0) {
            log_error("failed to create store thread");
            g_shutdown = 1;
            proc_ctx.shutdown = 1;
            sample_queue_shutdown(raw_queue);
            sample_queue_shutdown(store_queue);
            pthread_join(collect_tid, NULL);
            pthread_join(proc_tid, NULL);
            sqlite_store_close(db);
            sample_queue_destroy(raw_queue);
            sample_queue_destroy(store_queue);
            if (fd >= 0) serial_close(fd);
            alarm_client_close(alarm_fd);
            return 1;
        }

        memset(&upload_ctx, 0, sizeof(upload_ctx));
        upload_ctx.db = db;
        upload_ctx.device_id = cfg.device_id;
        upload_ctx.server_host = cfg.upload_server_host;
        upload_ctx.server_port = cfg.upload_server_port;
        upload_ctx.upload_period_ms = cfg.upload_period_ms;
        upload_ctx.upload_batch_max = cfg.upload_batch_max;
        upload_ctx.upload_retry_max = cfg.upload_retry_max;
        upload_ctx.shutdown = 0;

        if (cfg.upload_enabled) {
            if (pthread_create(&upload_tid, NULL, uploader_thread, &upload_ctx) != 0) {
                log_error("failed to create upload thread");
                g_shutdown = 1;
                proc_ctx.shutdown = 1;
                store_ctx.shutdown = 1;
                sample_queue_shutdown(raw_queue);
                sample_queue_shutdown(store_queue);
                pthread_join(collect_tid, NULL);
                pthread_join(proc_tid, NULL);
                pthread_join(store_tid, NULL);
                sqlite_store_close(db);
                sample_queue_destroy(raw_queue);
                sample_queue_destroy(store_queue);
                if (fd >= 0) serial_close(fd);
                alarm_client_close(alarm_fd);
                return 1;
            }
        }

        log_info("pid=%d", (int)getpid());

        {
            int heartbeat_count = 0;

            while (!g_shutdown) {
                sleep_ms(1000);
                heartbeat_count++;

                if (heartbeat_count % 60 == 0) {
                    log_info("heartbeat: db_total=%d db_unuploaded=%d "
                             "store_written=%lu store_failed=%lu "
                             "uploaded=%lu upload_failed=%lu",
                             sqlite_store_get_total_count(db),
                             sqlite_store_get_unuploaded_count(db),
                             store_ctx.write_count, store_ctx.write_fail_count,
                             upload_ctx.uploaded_count, upload_ctx.failed_count);
                }
            }
        }

        log_info("shutting down...");
        proc_ctx.shutdown = 1;
        store_ctx.shutdown = 1;
        sample_queue_shutdown(raw_queue);
        sample_queue_shutdown(store_queue);

        pthread_join(collect_tid, NULL);
        pthread_join(proc_tid, NULL);
        pthread_join(store_tid, NULL);

        upload_ctx.shutdown = 1;
        if (cfg.upload_enabled) {
            pthread_join(upload_tid, NULL);
        }

        log_info("=== pipeline stopped ===");
        log_info("raw_queue:    pushed=%lu overflow=%lu remaining=%d",
                 sample_queue_push_count(raw_queue),
                 sample_queue_overflow_count(raw_queue),
                 sample_queue_count(raw_queue));
        log_info("store_queue:  pushed=%lu overflow=%lu remaining=%d",
                 sample_queue_push_count(store_queue),
                 sample_queue_overflow_count(store_queue),
                 sample_queue_count(store_queue));
        log_info("store_thread:  written=%lu failed=%lu",
                 store_ctx.write_count, store_ctx.write_fail_count);
        log_info("db:            total_samples=%d unuploaded=%d",
                 sqlite_store_get_total_count(db),
                 sqlite_store_get_unuploaded_count(db));
        log_info("upload_thread: uploaded=%lu failed=%lu",
                 upload_ctx.uploaded_count, upload_ctx.failed_count);

        sqlite_store_close(db);
        sample_queue_destroy(raw_queue);
        sample_queue_destroy(store_queue);
    }

    if (fd >= 0) {
        serial_close(fd);
    }

    alarm_client_close(alarm_fd);

    logger_close_log_file();

    return 0;
}

