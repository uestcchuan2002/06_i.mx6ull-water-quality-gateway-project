#ifndef __ALARM_CLIENT_H
#define __ALARM_CLIENT_H

int alarm_client_open(const char *device);
void alarm_client_set(int fd, int on);
void alarm_client_close(int fd);

#endif
