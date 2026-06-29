#include "../include/alarm_client.h"
#include "../include/logger.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define ALARM_GPIO   0
#define GPIO_PATH    "/sys/class/gpio"

int alarm_client_open(const char *device)
{
    char buf[64];
    int fd_export, fd_direction, fd_value;

    (void)device;

    fd_export = open(GPIO_PATH "/export", O_WRONLY);
    if (fd_export < 0) {
        log_warn("alarm: open %s/export failed, hardware alarm disabled",
                 GPIO_PATH);
        return -1;
    }

    snprintf(buf, sizeof(buf), "%d", ALARM_GPIO);
    if (write(fd_export, buf, strlen(buf)) < 0) {
        log_warn("alarm: export GPIO%d failed (may already be exported)",
                 ALARM_GPIO);
    }
    close(fd_export);

    snprintf(buf, sizeof(buf), GPIO_PATH "/gpio%d/direction", ALARM_GPIO);
    fd_direction = open(buf, O_WRONLY);
    if (fd_direction < 0) {
        log_warn("alarm: open %s failed, hardware alarm disabled", buf);
        return -1;
    }

    if (write(fd_direction, "out", 3) < 0) {
        log_warn("alarm: set direction failed");
        close(fd_direction);
        return -1;
    }
    close(fd_direction);

    snprintf(buf, sizeof(buf), GPIO_PATH "/gpio%d/value", ALARM_GPIO);
    fd_value = open(buf, O_RDWR);
    if (fd_value < 0) {
        log_warn("alarm: open %s failed, hardware alarm disabled", buf);
        return -1;
    }

    if (write(fd_value, "1", 1) < 0) {
        log_warn("alarm: init write failed");
        close(fd_value);
        return -1;
    }

    log_info("alarm: GPIO%d exported via sysfs, buzzer ready", ALARM_GPIO);
    return fd_value;
}

void alarm_client_set(int fd, int on)
{
    char cmd;

    if (fd < 0) {
        return;
    }

    cmd = on ? '0' : '1';

    if (write(fd, &cmd, 1) < 0) {
        log_warn("alarm: write failed");
    }
}

void alarm_client_close(int fd)
{
    char buf[64];
    int fd_unexport;

    if (fd >= 0) {
        close(fd);
    }

    fd_unexport = open(GPIO_PATH "/unexport", O_WRONLY);
    if (fd_unexport < 0) {
        return;
    }

    snprintf(buf, sizeof(buf), "%d", ALARM_GPIO);
    if (write(fd_unexport, buf, strlen(buf)) < 0) {
        /* unexport best-effort, ignore failure */
    }
    close(fd_unexport);

    log_info("alarm: GPIO%d unexported", ALARM_GPIO);
}
