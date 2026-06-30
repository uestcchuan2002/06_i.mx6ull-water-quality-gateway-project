#include "../include/alarm_client.h"
#include "../include/logger.h"

#include <fcntl.h>
#include <unistd.h>

int alarm_client_open(const char *device)
{
    int fd;

    if (device == NULL) {
        log_warn("alarm: device path is NULL, hardware alarm disabled");
        return -1;
    }

    fd = open(device, O_RDWR);
    if (fd < 0) {
        log_warn("alarm: open %s failed, hardware alarm disabled "
                 "(driver not loaded?)", device);
        return -1;
    }

    log_info("alarm: %s opened, LED alarm ready", device);
    return fd;
}

void alarm_client_set(int fd, int on)
{
    unsigned char cmd;

    if (fd < 0) {
        return;
    }

    cmd = on ? 1 : 0;

    if (write(fd, &cmd, 1) < 0) {
        log_warn("alarm: write failed");
    }
}

void alarm_client_close(int fd)
{
    if (fd >= 0) {
        close(fd);
    }
}
