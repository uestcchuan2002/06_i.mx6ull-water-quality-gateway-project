#ifndef __SERIAL_PORT_H
#define __SERIAL_PORT_H

int serial_open(const char *device, int baudrate);
int serial_close(int fd);
int serial_write(int fd, const unsigned char *buf, int len);
int serial_read(int fd, unsigned char *buf, int len, int timeout_ms);

#endif
