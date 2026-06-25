#ifndef __MODBUS_RTU_H
#define __MODBUS_RTU_H

#define MODBUS_FUNC_READ_HOLDING    0x03

#define MODBUS_EXCEPTION_ILLEGAL_FUNC      0x01
#define MODBUS_EXCEPTION_ILLEGAL_ADDR      0x02
#define MODBUS_EXCEPTION_ILLEGAL_DATA      0x03
#define MODBUS_EXCEPTION_SLAVE_FAIL        0x04

#define MODBUS_MIN_FRAME_LEN   4
#define MODBUS_MAX_REGISTERS   125
#define MODBUS_RESPONSE_MIN    5

unsigned short modbus_crc16(const unsigned char *buf, int len);

int modbus_build_read_request(unsigned char slave_addr,
                              unsigned short start_addr,
                              unsigned short quantity,
                              unsigned char *buf,
                              int buf_size);

int modbus_parse_response(const unsigned char *buf,
                          int len,
                          unsigned short *values,
                          int max_values);

void modbus_hex_dump(const unsigned char *buf, int len, char *out, int out_size);

int modbus_read_registers(int fd, unsigned char slave_addr,
                          unsigned short start_addr, unsigned short quantity,
                          unsigned short *values, int max_values,
                          int timeout_ms);

void modbus_test_serial_read(int fd);

void modbus_run_sensor_tests(void);

#endif
