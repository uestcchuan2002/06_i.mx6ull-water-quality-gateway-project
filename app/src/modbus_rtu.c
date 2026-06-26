#include "../include/modbus_rtu.h"
#include "../include/serial_port.h"

#include <stdio.h>
#include <string.h>

unsigned short modbus_crc16(const unsigned char *buf, int len)
{
    unsigned short crc = 0xFFFF;
    int i, j;

    if (buf == NULL || len <= 0) {
        return 0;
    }

    for (i = 0; i < len; i++) {
        crc ^= (unsigned short)buf[i];
        for (j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (unsigned short)((crc >> 1) ^ 0xA001);
            } else {
                crc >>= 1;
            }
        }
    }

    return crc;
}

int modbus_build_read_request(unsigned char slave_addr,
                              unsigned short start_addr,
                              unsigned short quantity,
                              unsigned char *buf,
                              int buf_size)
{
    unsigned short crc;

    if (buf == NULL || buf_size < 8) {
        fprintf(stderr, "modbus build request: buffer too small\n");
        return -1;
    }

    if (quantity == 0 || quantity > MODBUS_MAX_REGISTERS) {
        fprintf(stderr, "modbus build request: invalid quantity %u\n",
                (unsigned int)quantity);
        return -1;
    }

    buf[0] = slave_addr;
    buf[1] = MODBUS_FUNC_READ_HOLDING;
    buf[2] = (unsigned char)(start_addr >> 8);
    buf[3] = (unsigned char)(start_addr & 0xFF);
    buf[4] = (unsigned char)(quantity >> 8);
    buf[5] = (unsigned char)(quantity & 0xFF);

    crc = modbus_crc16(buf, 6);
    buf[6] = (unsigned char)(crc & 0xFF);
    buf[7] = (unsigned char)(crc >> 8);

    return 8;
}

int modbus_parse_response(const unsigned char *buf,
                          int len,
                          unsigned short *values,
                          int max_values)
{
    int byte_count;
    int reg_count;
    int i;
    unsigned short crc;
    unsigned short expected_crc;

    if (buf == NULL || values == NULL || max_values <= 0) {
        return -1;
    }

    if (len < MODBUS_RESPONSE_MIN) {
        fprintf(stderr, "modbus parse: frame too short (%d bytes)\n", len);
        return -1;
    }

    if (buf[1] & 0x80) {
        fprintf(stderr, "modbus exception: func=0x%02X code=%u\n",
                (unsigned int)buf[1], (unsigned int)buf[2]);
        return -1;
    }

    if (buf[1] != MODBUS_FUNC_READ_HOLDING) {
        fprintf(stderr, "modbus parse: unexpected func 0x%02X\n",
                (unsigned int)buf[1]);
        return -1;
    }

    byte_count = (int)buf[2];

    if (len != byte_count + 5) {
        fprintf(stderr, "modbus parse: length mismatch (got %d, expected %d)\n",
                len, byte_count + 5);
        return -1;
    }

    expected_crc = modbus_crc16(buf, len - 2);
    crc = (unsigned short)buf[len - 1] << 8 | (unsigned short)buf[len - 2];
    if (crc != expected_crc) {
        fprintf(stderr, "modbus parse: crc error (got 0x%04X, expected 0x%04X)\n",
                (unsigned int)crc, (unsigned int)expected_crc);
        return -1;
    }

    reg_count = byte_count / 2;
    if (reg_count > max_values) {
        reg_count = max_values;
    }

    for (i = 0; i < reg_count; i++) {
        values[i] = ((unsigned short)buf[3 + i * 2] << 8)
                  | (unsigned short)buf[4 + i * 2];
    }

    return reg_count;
}

void modbus_hex_dump(const unsigned char *buf, int len, char *out, int out_size)
{
    int i;
    int pos;

    if (buf == NULL || out == NULL || out_size <= 0) {
        return;
    }

    pos = 0;
    out[0] = '\0';

    for (i = 0; i < len && pos < out_size - 4; i++) {
        pos += snprintf(out + pos, (size_t)(out_size - pos),
                        "%s%02X", i == 0 ? "" : " ", buf[i]);
    }
}

/*
 * 通过 Modbus RTU 协议读取保持寄存器 (功能码 0x03)
 * fd:          串口文件描述符
 * slave_addr:  从机地址 (1-247)
 * start_addr:  起始寄存器地址
 * quantity:    要读取的寄存器数量
 * values:      输出缓冲区，存放读取到的寄存器值
 * max_values:  values 缓冲区最大容量
 * timeout_ms:  接收超时时间 (毫秒)
 * 返回值: 成功返回读取到的寄存器个数，失败返回 -1
 */
int modbus_read_registers(int fd, unsigned char slave_addr,
                          unsigned short start_addr, unsigned short quantity,
                          unsigned short *values, int max_values,
                          int timeout_ms)
{
    unsigned char request[8];
    unsigned char response[256];
    int req_len, resp_len;
    char hex[256];

    // 构建 Modbus 读寄存器请求帧
    req_len = modbus_build_read_request(slave_addr, start_addr, quantity,
                                        request, (int)sizeof(request));
    if (req_len < 0) {
        return -1;
    }

    // 打印发送帧 (十六进制)
    modbus_hex_dump(request, req_len, hex, sizeof(hex));
    printf("  TX: %s\n", hex);

    // 发送请求帧到串口
    if (serial_write(fd, request, req_len) != req_len) {
        fprintf(stderr, "modbus: serial write failed\n");
        return -1;
    }

    // 等待并读取从机响应
    resp_len = serial_read(fd, response, (int)sizeof(response), timeout_ms);
    if (resp_len <= 0) {
        fprintf(stderr, "modbus: no response (timeout)\n");
        return -1;
    }

    // 打印接收帧 (十六进制)
    modbus_hex_dump(response, resp_len, hex, sizeof(hex));
    printf("  RX: %s\n", hex);

    // 解析响应帧，提取寄存器值
    return modbus_parse_response(response, resp_len, values, max_values);
}

void modbus_test_serial_read(int fd)
{
    unsigned short values[16];
    int count;

    printf("\n--- Serial Modbus Read Test ---\n");
    printf("  requesting 7 registers from slave 0x01...\n");

    count = modbus_read_registers(fd, 0x01, 0x0000, 7, values, 16, 1000);
    if (count > 0) {
        int i;
        printf("  success: %d registers read\n", count);
        for (i = 0; i < count; i++) {
            printf("    reg[0x%04X] = %u (0x%04X)\n",
                   i, (unsigned int)values[i], (unsigned int)values[i]);
        }
        if (count >= 7) {
            printf("  water quality: pH=%.2f  temp=%.2f C  turb=%.2f NTU  cond=%u us/cm\n",
                   values[0] * 0.01f,
                   (float)((signed short)values[1]) * 0.01f,
                   values[2] * 0.01f,
                   (unsigned int)values[3]);
            printf("  status=0x%04X  alarm=0x%04X  seq=%u\n",
                   (unsigned int)values[4],
                   (unsigned int)values[5],
                   (unsigned int)values[6]);
        }
    } else {
        printf("  no slave response (expected if no device connected)\n");
        printf("  falling back to simulation tests...\n");
        modbus_run_sensor_tests();
    }
}

static void build_test_response(unsigned char *buf, int *out_len,
                                const unsigned short *regs, int reg_count)
{
    int byte_count = reg_count * 2;
    unsigned short crc;
    int i;

    buf[0] = 0x01;
    buf[1] = MODBUS_FUNC_READ_HOLDING;
    buf[2] = (unsigned char)byte_count;
    for (i = 0; i < reg_count; i++) {
        buf[3 + i * 2] = (unsigned char)(regs[i] >> 8);
        buf[4 + i * 2] = (unsigned char)(regs[i] & 0xFF);
    }
    crc = modbus_crc16(buf, 3 + byte_count);
    buf[3 + byte_count] = (unsigned char)(crc & 0xFF);
    buf[4 + byte_count] = (unsigned char)(crc >> 8);
    *out_len = 5 + byte_count;
}

void modbus_run_sensor_tests(void)
{
    unsigned char buf[256];
    unsigned short values[16];
    char hex[256];
    int i, count, len;
    int pass, fail, total_pass, total_fail;

    total_pass = 0;
    total_fail = 0;

    printf("\n========================================\n");
    printf("  Modbus RTU Sensor Response Tests\n");
    printf("========================================\n\n");

    /* ---- Test 1: Normal ---- */
    {
        unsigned short regs[7] = {712, 2534, 320, 820, 0, 0, 1280};

        printf("[Test 1] Normal sensor data\n");
        build_test_response(buf, &len, regs, 7);
        modbus_hex_dump(buf, len, hex, sizeof(hex));
        printf("  frame: %s\n", hex);

        count = modbus_parse_response(buf, len, values, 16);
        pass = 0; fail = 0;

        if (count == 7) pass++; else { fail++; printf("  FAIL: count=%d\n", count); }
        if (count == 7) {
            if (values[0] == 712) pass++; else { fail++; printf("  FAIL: pH raw=%u != 712\n", (unsigned int)values[0]); }
            if ((signed short)values[1] == 2534) pass++; else { fail++; printf("  FAIL: temp raw=%u\n", (unsigned int)values[1]); }
            if (values[2] == 320) pass++; else { fail++; printf("  FAIL: turb raw=%u\n", (unsigned int)values[2]); }
            if (values[3] == 820) pass++; else { fail++; printf("  FAIL: cond raw=%u\n", (unsigned int)values[3]); }
            if (values[4] == 0) pass++; else { fail++; printf("  FAIL: status=0x%04X\n", (unsigned int)values[4]); }
            if (values[5] == 0) pass++; else { fail++; printf("  FAIL: alarm=0x%04X\n", (unsigned int)values[5]); }
            if (values[6] == 1280) pass++; else { fail++; printf("  FAIL: seq=%u\n", (unsigned int)values[6]); }
        }
        printf("  result: %d passed, %d failed\n\n", pass, fail);
        total_pass += pass;
        total_fail += fail;
    }

    /* ---- Test 2: Sensor fault ---- */
    {
        unsigned short regs[7] = {0, 32767, 320, 820, 0x0003, 0, 2048};

        printf("[Test 2] Sensor fault (pH+temp fault, status bit0+bit1)\n");
        build_test_response(buf, &len, regs, 7);
        modbus_hex_dump(buf, len, hex, sizeof(hex));
        printf("  frame: %s\n", hex);

        count = modbus_parse_response(buf, len, values, 16);
        pass = 0; fail = 0;

        if (count == 7) pass++; else { fail++; printf("  FAIL: count=%d\n", count); }
        if (count == 7) {
            if (values[4] == 0x0003) pass++;
            else { fail++; printf("  FAIL: status=0x%04X, expected 0x0003\n", (unsigned int)values[4]); }
            if (values[0] == 0) pass++;
            else { fail++; printf("  FAIL: pH=raw %u != 0\n", (unsigned int)values[0]); }
            if (values[5] == 0) pass++;
            else { fail++; printf("  FAIL: alarm should be 0\n"); }
        }
        if (fail == 0) {
            printf("  interpretation: pH fault(raw=0), temp saturated(raw=32767->327.67 C)\n");
            printf("  sensor_status=0x0003: bit0=pH fault, bit1=temp fault\n");
        }
        printf("  result: %d passed, %d failed\n\n", pass, fail);
        total_pass += pass;
        total_fail += fail;
    }

    /* ---- Test 3: Threshold alarm ---- */
    {
        unsigned short regs[7] = {450, 2500, 5000, 1500, 0, 0x0005, 3072};

        printf("[Test 3] Threshold alarm (pH low + turbidity high, alarm bit0+bit2)\n");
        build_test_response(buf, &len, regs, 7);
        modbus_hex_dump(buf, len, hex, sizeof(hex));
        printf("  frame: %s\n", hex);

        count = modbus_parse_response(buf, len, values, 16);
        pass = 0; fail = 0;

        if (count == 7) pass++; else { fail++; printf("  FAIL: count=%d\n", count); }
        if (count == 7) {
            if (values[5] == 0x0005) pass++;
            else { fail++; printf("  FAIL: alarm=0x%04X, expected 0x0005\n", (unsigned int)values[5]); }
            if (values[0] * 0.01f < 7.0f) pass++;
            else { fail++; printf("  FAIL: pH=%.2f should be < 7.0\n", values[0]*0.01f); }
            if (values[2] * 0.01f > 10.0f) pass++;
            else { fail++; printf("  FAIL: turb=%.2f should be > 10.0\n", values[2]*0.01f); }
        }
        if (fail == 0) {
            printf("  interpretation: pH=4.50 (low), turb=50.00 NTU (high)\n");
            printf("  alarm_status=0x0005: bit0=pH low, bit2=turb high\n");
        }
        printf("  result: %d passed, %d failed\n\n", pass, fail);
        total_pass += pass;
        total_fail += fail;
    }

    /* ---- Test 4: Boundary minimum ---- */
    {
        unsigned short regs[7] = {0, 0, 0, 0, 0, 0, 0};

        printf("[Test 4] Boundary: all registers zero\n");
        build_test_response(buf, &len, regs, 7);
        modbus_hex_dump(buf, len, hex, sizeof(hex));
        printf("  frame: %s\n", hex);

        count = modbus_parse_response(buf, len, values, 16);
        pass = 0; fail = 0;

        if (count == 7) pass++; else { fail++; printf("  FAIL: count=%d\n", count); }
        if (count == 7) {
            for (i = 0; i < 7; i++) {
                if (values[i] == 0) pass++; else { fail++; printf("  FAIL: reg[%d]=%u\n", i, (unsigned int)values[i]); }
            }
        }
        printf("  result: %d passed, %d failed\n\n", pass, fail);
        total_pass += pass;
        total_fail += fail;
    }

    /* ---- Test 5: Boundary maximum ---- */
    {
        unsigned short regs[7] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};

        printf("[Test 5] Boundary: all registers 0xFFFF\n");
        build_test_response(buf, &len, regs, 7);
        modbus_hex_dump(buf, len, hex, sizeof(hex));
        printf("  frame: %s\n", hex);

        count = modbus_parse_response(buf, len, values, 16);
        pass = 0; fail = 0;

        if (count == 7) pass++; else { fail++; printf("  FAIL: count=%d\n", count); }
        if (count == 7) {
            if (values[0] == 0xFFFF) pass++; else fail++;
            if (values[1] == 0xFFFF) pass++; else fail++;
            if (values[2] == 0xFFFF) pass++; else fail++;
            if (values[3] == 0xFFFF) pass++; else fail++;
            if (values[4] == 0xFFFF) pass++; else fail++;
            if (values[5] == 0xFFFF) pass++; else fail++;
            if (values[6] == 0xFFFF) pass++; else fail++;
        }
        printf("  interpretation: pH=655.35 (overflow), temp=-0.01 (signed overflow)\n");
        printf("  result: %d passed, %d failed\n\n", pass, fail);
        total_pass += pass;
        total_fail += fail;
    }

    /* ---- Test 6: Exception response ---- */
    {
        unsigned short crc;
        printf("[Test 6] Exception response (illegal data address 0x02)\n");
        buf[0] = 0x01;
        buf[1] = 0x83;
        buf[2] = 0x02;
        crc = modbus_crc16(buf, 3);
        buf[3] = (unsigned char)(crc & 0xFF);
        buf[4] = (unsigned char)(crc >> 8);
        modbus_hex_dump(buf, 5, hex, sizeof(hex));
        printf("  frame: %s\n", hex);

        count = modbus_parse_response(buf, 5, values, 16);
        pass = 0; fail = 0;
        if (count < 0) {
            pass++;
            printf("  correctly detected: exception code=%u\n", (unsigned int)buf[2]);
        } else {
            fail++;
            printf("  FAIL: should have returned error\n");
        }
        printf("  result: %d passed, %d failed\n\n", pass, fail);
        total_pass += pass;
        total_fail += fail;
    }

    /* ---- Test 7: CRC error ---- */
    {
        unsigned short regs[7] = {712, 2534, 320, 820, 0, 0, 1280};

        printf("[Test 7] CRC error (corrupted checksum)\n");
        build_test_response(buf, &len, regs, 7);
        buf[len - 2] = 0x00;
        buf[len - 1] = 0x00;
        modbus_hex_dump(buf, len, hex, sizeof(hex));
        printf("  frame: %s (last 2 bytes corrupted)\n", hex);

        count = modbus_parse_response(buf, len, values, 16);
        pass = 0; fail = 0;
        if (count < 0) {
            pass++;
            printf("  correctly detected: CRC mismatch\n");
        } else {
            fail++;
            printf("  FAIL: should have detected CRC error\n");
        }
        printf("  result: %d passed, %d failed\n\n", pass, fail);
        total_pass += pass;
        total_fail += fail;
    }

    /* ---- Test 8: Short frame ---- */
    {
        printf("[Test 8] Short frame (only 4 bytes)\n");
        buf[0] = 0x01;
        buf[1] = 0x03;
        buf[2] = 0x0E;
        buf[3] = 0x02;
        modbus_hex_dump(buf, 4, hex, sizeof(hex));
        printf("  frame: %s\n", hex);

        count = modbus_parse_response(buf, 4, values, 16);
        pass = 0; fail = 0;
        if (count < 0) {
            pass++;
            printf("  correctly detected: frame too short\n");
        } else {
            fail++;
            printf("  FAIL: should have rejected short frame\n");
        }
        printf("  result: %d passed, %d failed\n\n", pass, fail);
        total_pass += pass;
        total_fail += fail;
    }

    printf("========================================\n");
    printf("  Total: %d passed, %d failed\n", total_pass, total_fail);
    printf("========================================\n");
}
