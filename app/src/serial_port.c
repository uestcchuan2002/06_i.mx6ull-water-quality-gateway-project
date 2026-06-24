#include "../include/serial_port.h"

#if defined(_WIN32)

#include <stdio.h>

int serial_open(const char *device, int baudrate)
{
    (void)device;
    (void)baudrate;
    fprintf(stderr, "serial port is not supported on Windows build\n");
    return -1;
}

int serial_close(int fd)
{
    (void)fd;
    return -1;
}

int serial_write(int fd, const unsigned char *buf, int len)
{
    (void)fd;
    (void)buf;
    (void)len;
    return -1;
}

int serial_read(int fd, unsigned char *buf, int len, int timeout_ms)
{
    (void)fd;
    (void)buf;
    (void)len;
    (void)timeout_ms;
    return -1;
}

#else

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

static int serial_baudrate_to_flag(int baudrate, speed_t *speed)
{
    if (speed == NULL) {
        return -1;
    }

    switch (baudrate) {
    case 9600:
        *speed = B9600;
        return 0;
    case 19200:
        *speed = B19200;
        return 0;
    case 38400:
        *speed = B38400;
        return 0;
    case 57600:
        *speed = B57600;
        return 0;
    case 115200:
        *speed = B115200;
        return 0;
    default:
        return -1;
    }
}

static int serial_configure(int fd, int baudrate)
{
    struct termios options;
    speed_t speed;

    if (serial_baudrate_to_flag(baudrate, &speed) != 0) {
        fprintf(stderr, "unsupported baudrate: %d\n", baudrate);
        return -1;
    }

    if (tcgetattr(fd, &options) != 0) {
        fprintf(stderr, "tcgetattr failed: %s\n", strerror(errno));
        return -1;
    }

    cfmakeraw(&options);

    if (cfsetispeed(&options, speed) != 0 || cfsetospeed(&options, speed) != 0) {
        fprintf(stderr, "set serial speed failed: %s\n", strerror(errno));
        return -1;
    }

    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CRTSCTS;

    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 0;

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &options) != 0) {
        fprintf(stderr, "tcsetattr failed: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

int serial_open(const char *device, int baudrate)
{
    int fd;

    if (device == NULL) {
        fprintf(stderr, "serial device is null\n");
        return -1;
    }

    fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        fprintf(stderr, "open serial device %s failed: %s\n", device, strerror(errno));
        return -1;
    }

    if (serial_configure(fd, baudrate) != 0) {
        close(fd);
        return -1;
    }

    if (fcntl(fd, F_SETFL, 0) != 0) {
        fprintf(stderr, "set serial blocking mode failed: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    return fd;
}

int serial_close(int fd)
{
    if (fd < 0) {
        return -1;
    }

    return close(fd);
}

int serial_write(int fd, const unsigned char *buf, int len)
{
    int total = 0;

    if (fd < 0 || buf == NULL || len <= 0) {
        return -1;
    }

    while (total < len) {
        ssize_t ret = write(fd, buf + total, (size_t)(len - total));

        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }

            fprintf(stderr, "serial write failed: %s\n", strerror(errno));
            return -1;
        }

        if (ret == 0) {
            break;
        }

        total += (int)ret;
    }

    return total;
}

int serial_read(int fd, unsigned char *buf, int len, int timeout_ms)
{
    fd_set read_set;
    struct timeval timeout;
    struct timeval *timeout_ptr = NULL;
    int ret;

    if (fd < 0 || buf == NULL || len <= 0) {
        return -1;
    }

    FD_ZERO(&read_set);
    FD_SET(fd, &read_set);

    if (timeout_ms >= 0) {
        timeout.tv_sec = timeout_ms / 1000;
        timeout.tv_usec = (timeout_ms % 1000) * 1000;
        timeout_ptr = &timeout;
    }

    ret = select(fd + 1, &read_set, NULL, NULL, timeout_ptr);
    if (ret < 0) {
        if (errno == EINTR) {
            return 0;
        }

        fprintf(stderr, "serial select failed: %s\n", strerror(errno));
        return -1;
    }

    if (ret == 0) {
        return 0;
    }

    ret = (int)read(fd, buf, (size_t)len);
    if (ret < 0) {
        if (errno == EINTR || errno == EAGAIN) {
            return 0;
        }

        fprintf(stderr, "serial read failed: %s\n", strerror(errno));
        return -1;
    }

    return ret;
}

#endif
