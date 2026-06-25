# 项目调试记录

## 2026-06-22 开发板启动与通信验证

### 1. 启动验证

开发板已能通过串口终端正常输出 U-Boot 和 Linux 启动信息，并成功进入 Linux root shell。

记录到的关键信息：

- 开发板型号：i.MX6U ALPHA/MINI
- CPU：Freescale i.MX6ULL rev1.1
- DRAM：512 MiB
- U-Boot 版本：U-Boot 2016.03
- 启动方式：Normal Boot
- 控制台：Serial
- Kernel 镜像：zImage
- 设备树文件：`imx6ull-14x14-emmc-7-1024x600-c.dtb`
- 串口控制台：`ttymxc0,115200`
- 根文件系统挂载设备：`/dev/mmcblk1p2`
- U-Boot 提示：`bad CRC, using default environment`，说明当前使用默认环境变量，后续可再处理

### 2. Linux 系统版本

执行命令：

```sh
uname -a
```

输出记录：

```text
Linux ATK-IMX6U 4.1.15-g3dc0a4b #1 SMP PREEMPT Thu Aug 18 09:27:40 CST 2022 armv7l armv7l armv7l GNU/Linux
```

结论：

- 主机名：`ATK-IMX6U`
- Kernel 版本：`4.1.15-g3dc0a4b`
- CPU 架构：`armv7l`
- 系统类型：GNU/Linux

### 3. 登录用户与权限

执行命令：

```sh
whoami
id
```

输出记录：

```text
root
uid=0(root) gid=0(root) groups=0(root)
```

结论：

- 当前用户为 `root`
- 当前环境具备访问设备文件、加载驱动、执行底层调试的权限

### 4. SSH 与串口连接

SSH 连接命令：

```sh
ssh -oHostKeyAlgorithms=+ssh-rsa -oPubkeyAcceptedAlgorithms=+ssh-rsa root@192.168.2.201
```

串口终端参数：

```text
baudrate: 115200
```

### 5. 第一版最小 demo

交叉编译命令：

```sh
arm-linux-gnueabihf-gcc -Wall -Wextra -O2 -static main.c -o water_gateway
```

上传命令：

```sh
scp -O -oHostKeyAlgorithms=+ssh-rsa -oPubkeyAcceptedAlgorithms=+ssh-rsa water_gateway root@192.168.2.201:/home/root/water_gateway
```

开发板运行命令：

```sh
./water_gateway
```

输出记录：

```text
water_gateway start
sim sample: ph=7.12 temp=25.34 turbidity=3.20 conductivity=820
```

### 6. 当日结论

2026-06-22 的环境验证任务完成。开发板 Linux 启动、串口终端、SSH、root 权限和第一版最小 demo 均验证通过。

## 2026-06-23 应用层最小 Demo 验收

### 1. 当日目标

在第一天环境验证的基础上，完成应用层最小 Demo 框架：

- 编写 `Makefile`
- 完成配置读取模块
- 完成日志模块
- 完成模拟水质数据模块
- 在 `main.c` 中串联配置、日志和模拟采样流程

### 2. 新增和修改文件

```text
app/Makefile
app/include/config.h
app/src/config.c
app/include/logger.h
app/src/logger.c
app/include/sample.h
app/src/sample.c
app/src/main.c
config/gateway.conf
```

### 3. 配置文件验证

配置文件路径：

```text
config/gateway.conf
```

配置内容：

```ini
device_id=water_gateway_001
sample_period_ms=1000
log_level=info
serial_device=/dev/ttyUSB0
baudrate=9600
```

程序已能读取以下配置项：

- `device_id`
- `sample_period_ms`
- `log_level`
- `serial_device`
- `baudrate`

### 4. 编译验证

PowerShell 当前环境未安装 `make`，因此本地使用 `gcc` 直接完成语法和链接验证。

验证命令：

```sh
gcc -std=gnu99 -Iinclude src/main.c src/config.c src/logger.c src/sample.c -o water_gateway_test.exe
```

验证结果：

```text
compile success
```

说明：

- `Makefile` 已补充 `-std=gnu99`
- 可避免老版本 GCC 遇到 `for (int i = 0; ...)` 时报 C89/C90 兼容性错误
- 在 Linux 主机或开发板上可继续使用 `make clean && make`

### 5. 运行验证

运行命令：

```sh
./water_gateway_test.exe
```

关键输出：

```text
[2026-06-23 19:44:45] [INFO] water gateway start
[2026-06-23 19:44:45] [INFO] config loaded
[2026-06-23 19:44:45] [INFO] device_id=water_gateway_001
[2026-06-23 19:44:45] [INFO] sample_period_ms=1000
[2026-06-23 19:44:45] [INFO] serial_device=/dev/ttyUSB0
[2026-06-23 19:44:45] [INFO] baudrate=9600
[2026-06-23 19:44:45] [INFO] timestamp_ms=1782215085414 ph=7.01 temperature=25.01 turbidity=3.01 conductivity=801.00 sensor_status=0x0 alarm_status=0x0 seq=1
[2026-06-23 19:44:46] [INFO] timestamp_ms=1782215086416 ph=7.02 temperature=25.02 turbidity=3.02 conductivity=802.00 sensor_status=0x0 alarm_status=0x0 seq=2
[2026-06-23 19:44:47] [INFO] timestamp_ms=1782215087420 ph=7.03 temperature=25.03 turbidity=3.03 conductivity=803.00 sensor_status=0x0 alarm_status=0x0 seq=3
[2026-06-23 19:44:48] [INFO] timestamp_ms=1782215088423 ph=7.04 temperature=25.04 turbidity=3.04 conductivity=804.00 sensor_status=0x0 alarm_status=0x0 seq=4
[2026-06-23 19:44:49] [INFO] timestamp_ms=1782215089424 ph=7.05 temperature=25.05 turbidity=3.05 conductivity=805.00 sensor_status=0x0 alarm_status=0x0 seq=5
```

### 6. 问题与修复

#### 问题 1：本地 GCC 默认标准较旧

现象：

```text
error: 'for' loop initial declarations are only allowed in C99 or C11 mode
```

修复方式：

在 `Makefile` 中加入：

```makefile
CFLAGS += -std=gnu99
```

#### 问题 2：Windows 本地测试时 `usleep` 行为不稳定

修复方式：

在 `main.c` 中封装 `sleep_ms()`：

- Windows 使用 `Sleep()`
- Linux 使用 `usleep()`

#### 问题 3：模拟采样时间戳只有秒级精度

修复方式：

在 `sample.c` 中将时间戳改为毫秒级：

- Windows 使用 `GetSystemTimeAsFileTime()`
- Linux 使用 `gettimeofday()`

### 7. 当日验收结论

2026-06-23 的第二天任务完成。

已完成内容：

- `Makefile` 可维护当前应用层 C 文件编译
- `config` 模块可读取配置文件
- `logger` 模块可输出带时间戳和等级的日志
- `sample` 模块可生成模拟水质数据
- `main` 程序可按配置周期输出 5 组模拟采样结果

当前最小 Demo 已形成闭环：

```text
gateway.conf -> config_load -> logger_init -> sample_generate_mock -> log_info
```

### 8. 下一步计划

下一阶段进入串口模块开发：

- 编写 `serial_port.h`
- 编写 `serial_port.c`
- 支持打开串口设备
- 支持设置波特率、8N1、读超时
- 封装 `serial_read` 和 `serial_write`
- 为后续 Modbus RTU 主站模块做准备

## 2026-06-24 开发板运行、RTC 和串口模块初验

### 1. 当日目标

在 2026-06-23 应用层最小 Demo 的基础上，完成以下验证：

- 将新版 `water_gateway` 交叉编译并上传到 i.MX6ULL 开发板
- 使用 `-c gateway.conf` 参数解决配置文件路径问题
- 修复开发板重启后系统时间错误的问题
- 完成 `serial_port.h` 和 `serial_port.c`
- 在开发板上验证串口设备 `/dev/ttymxc2` 可被程序打开

### 2. 配置文件路径问题验证

开发板运行命令：

```sh
./water_gateway -c gateway.conf
```

关键输出：

```text
[2026-06-24 20:01:04] [INFO] water gateway start
[2026-06-24 20:01:04] [INFO] config_path=gateway.conf
[2026-06-24 20:01:04] [INFO] config loaded
[2026-06-24 20:01:04] [INFO] device_id=water_gateway_001
[2026-06-24 20:01:04] [INFO] sample_period_ms=1000
[2026-06-24 20:01:04] [INFO] serial_device=/dev/ttymxc2
[2026-06-24 20:01:04] [INFO] baudrate=9600
```

结论：

- `-c gateway.conf` 参数生效
- 程序已能在开发板当前目录读取配置文件
- 之前出现的 `open config file failed: No such file or directory` 问题已解决

### 3. 开发板 RTC 时间修复

问题现象：

```text
使用 date -s 修改系统时间后，重启开发板时间又恢复错误。
```

原因分析：

```text
date -s 只修改 Linux 当前系统时间，没有同步到 RTC 硬件时钟。
```

处理命令：

```sh
date -s "2026-06-24 20:00:00"
hwclock -w
reboot
date
```

验证结果：

```text
开发板重启后系统时间保持正常。
```

结论：

- 当前开发板存在 RTC 设备
- 系统时间已成功写入 RTC
- 后续日志时间可正常用于调试记录

### 4. 串口设备确认

执行命令：

```sh
ls /dev/
```

观察到的串口相关设备：

```text
/dev/ttymxc0
/dev/ttymxc2
```

说明：

- `/dev/ttymxc0` 大概率为调试控制台串口，不建议作为应用程序测试串口
- `/dev/ttymxc2` 可作为当前阶段串口模块测试设备
- 当前未观察到 `/dev/ttyUSB0`，说明 USB-RS485 暂未接入或未识别

### 5. 新增串口模块

新增文件：

```text
app/include/serial_port.h
app/src/serial_port.c
```

当前接口：

```c
int serial_open(const char *device, int baudrate);
int serial_close(int fd);
int serial_write(int fd, const unsigned char *buf, int len);
int serial_read(int fd, unsigned char *buf, int len, int timeout_ms);
```

已实现功能：

- 打开串口设备
- 支持 `9600`、`19200`、`38400`、`57600`、`115200`
- 配置 8N1
- 关闭校验位
- 关闭硬件流控
- 设置 raw mode
- 使用 `select()` 支持超时读取
- 封装循环写入

### 6. 交叉编译问题记录

#### 问题 1：glibc 版本不匹配

现象：

```text
./water_gateway: /lib/libc.so.6: version `GLIBC_2.28' not found
./water_gateway: /lib/libc.so.6: version `GLIBC_2.34' not found
```

原因：

```text
交叉编译工具链使用的 glibc 版本高于开发板 rootfs 中的 glibc 版本。
```

处理方式：

```sh
make clean
make CROSS_COMPILE=arm-linux-gnueabihf- LDFLAGS=-static
```

#### 问题 2：Exec format error

现象：

```text
-sh: ./water_gateway: cannot execute binary file: Exec format error
```

原因：

```text
上传的可执行文件不是开发板可执行的 ARM 32-bit ELF，可能是 PC 本地 gcc 编译产物或旧文件。
```

处理方式：

```sh
make clean
make CROSS_COMPILE=arm-linux-gnueabihf- LDFLAGS=-static
file water_gateway
```

正确文件类型应包含：

```text
ELF 32-bit LSB executable, ARM
statically linked
```

### 7. 串口打开验证

配置文件中串口相关配置：

```ini
serial_device=/dev/ttymxc2
baudrate=9600
```

开发板运行命令：

```sh
./water_gateway -c gateway.conf
```

关键输出：

```text
[2026-06-24 20:01:04] [INFO] serial_device=/dev/ttymxc2
[2026-06-24 20:01:04] [INFO] baudrate=9600
[2026-06-24 20:01:09] [INFO] serial open success: /dev/ttymxc2
```

结论：

- `serial_port` 模块在 i.MX6ULL 上编译和运行通过
- 程序可成功打开 `/dev/ttymxc2`
- 串口基础 open/config/close 流程验证通过
- 项目已从纯模拟数据推进到真实 Linux 串口设备接口层

### 8. 当日验收结论

2026-06-24 的任务完成。

已完成内容：

- 最小 Demo 在 i.MX6ULL 上运行正常
- 配置路径问题已解决
- RTC 时间保持问题已解决
- `serial_port` 模块第一版完成
- `/dev/ttymxc2` 串口打开验证通过

当前阶段闭环：

```text
gateway.conf -> config_load -> logger_init -> sample_generate_mock -> serial_open(/dev/ttymxc2)
```

### 9. 下一步计划

下一阶段进入 Modbus RTU 主站基础模块：

- 编写 `modbus_rtu.h`
- 编写 `modbus_rtu.c`
- 实现 CRC16
- 构造 0x03 读保持寄存器请求帧
- 解析响应帧
- 通过串口模块发送和接收 Modbus RTU 数据帧

## 2026-06-25 Modbus RTU 模块开发

### 1. 当日目标

完成 Modbus RTU 主站基础模块：

- 编写 `modbus_rtu.h`
- 编写 `modbus_rtu.c`
- 实现 Modbus CRC16 校验
- 构造 0x03 读保持寄存器请求帧
- 解析 Modbus RTU 响应帧（含异常码分类）
- 在主程序中添加 Modbus 功能测试

### 2. 新增和修改文件

```text
app/include/modbus_rtu.h   (新增)
app/src/modbus_rtu.c       (新增)
app/src/main.c             (修改：添加 Modbus 测试代码)
```

### 3. 模块接口

`modbus_rtu.h` 定义：

```c
unsigned short modbus_crc16(const unsigned char *buf, int len);
int modbus_build_read_request(unsigned char slave_addr,
                              unsigned short start_addr,
                              unsigned short quantity,
                              unsigned char *buf, int buf_size);
int modbus_parse_response(const unsigned char *buf, int len,
                          unsigned short *values, int max_values);
void modbus_hex_dump(const unsigned char *buf, int len,
                     char *out, int out_size);
```

已实现功能：

- CRC16（多项式 0xA001，初值 0xFFFF）
- 请求帧构造（从机地址 + 0x03 + 起始地址 + 寄存器数量 + CRC）
- 响应帧解析（异常响应检测、CRC 校验、大端解包）
- 十六进制 dump（调试用）

### 4. Modbus 传感器回复测试案例

2026-06-25 新增 `modbus_run_sensor_tests()`，覆盖 8 个测试案例：

| # | 测试场景 | 关键数据 | 预期结果 |
|---|---------|---------|---------|
| 1 | 正常数据 | pH=712, temp=2534, turb=320, cond=820, status=0, alarm=0, seq=1280 | 7 寄存器均解析正确 |
| 2 | 传感器故障 | pH故障=0, temp饱和=32767, status=0x0003(bit0+bit1) | status 反射 pH+temp 故障 |
| 3 | 阈值告警 | pH=450(4.50低), turb=5000(50.00高), alarm=0x0005(bit0+bit2) | alarm 反射 pH低+turb高 |
| 4 | 边界最小值 | 全部寄存器 = 0x0000 | 7 个 0 值正常解析 |
| 5 | 边界最大值 | 全部寄存器 = 0xFFFF | 7 个 0xFFFF，signed 溢出可观测 |
| 6 | 异常响应 | func=0x83, code=0x02 | 识别为异常响应，返回 -1 |
| 7 | CRC 错误 | 正常帧 CRC 被覆写为 0x0000 | CRC 校验失败，返回 -1 |
| 8 | 短帧 | 仅 4 字节 (01 03 0E 02) | 帧太短被拒绝，返回 -1 |

### 5. 寄存器到水质数据映射

按计划书 register map 定义：

| 寄存器地址 | 值(raw) | 缩放 | 解析结果 |
|---|---|---|---|
| 0x0000 | 712 | 0.01 | pH=7.12 |
| 0x0001 | 2534 | 0.01 | temp=25.34 C |
| 0x0002 | 320 | 0.01 | turbidity=3.20 NTU |
| 0x0003 | 820 | 1 | conductivity=820 us/cm |
| 0x0004 | 0 | - | sensor_status=0x0000 |
| 0x0005 | 0 | - | alarm_status=0x0000 |
| 0x0006 | 1 | - | sequence=1 |

### 6. Makefile 说明

Makefile 使用 `$(wildcard $(SRC_DIR)/*.c)` 自动收集源文件，新增 `modbus_rtu.c` 无需修改 Makefile 即可自动加入编译。

交叉编译命令：

```sh
make clean
make CROSS_COMPILE=arm-linux-gnueabihf- LDFLAGS=-static
```

### 7. 预期运行输出

```text
[2026-06-25 xx:xx:xx] [INFO] === modbus rtu sensor tests ===

========================================
  Modbus RTU Sensor Response Tests
========================================

[Test 1] Normal sensor data
  frame: 01 03 0E 02 C8 09 E6 01 40 03 34 00 00 00 00 05 00 XX XX
  result: 8 passed, 0 failed

[Test 2] Sensor fault (pH+temp fault, status bit0+bit1)
  frame: 01 03 0E 00 00 7F FF 01 40 03 34 00 03 00 00 08 00 XX XX
  interpretation: pH fault(raw=0), temp saturated(raw=32767->327.67 C)
  sensor_status=0x0003: bit0=pH fault, bit1=temp fault
  result: 4 passed, 0 failed

[Test 3] Threshold alarm (pH low + turbidity high, alarm bit0+bit2)
  frame: 01 03 0E 01 C2 09 C4 13 88 05 DC 00 00 00 05 0C 00 XX XX
  interpretation: pH=4.50 (low), turb=50.00 NTU (high)
  alarm_status=0x0005: bit0=pH low, bit2=turb high
  result: 4 passed, 0 failed

[Test 4] Boundary: all registers zero
  frame: 01 03 0E 00 00 00 00 00 00 00 00 00 00 00 00 00 00 XX XX
  result: 8 passed, 0 failed

[Test 5] Boundary: all registers 0xFFFF
  frame: 01 03 0E FF FF FF FF FF FF FF FF FF FF FF FF FF FF XX XX
  interpretation: pH=655.35 (overflow), temp=-0.01 (signed overflow)
  result: 8 passed, 0 failed

[Test 6] Exception response (illegal data address 0x02)
  frame: 01 83 02 XX XX
  correctly detected: exception code=2
  result: 1 passed, 0 failed

[Test 7] CRC error (corrupted checksum)
  frame: 01 03 0E ... 00 00 (last 2 bytes corrupted)
  correctly detected: CRC mismatch
  result: 1 passed, 0 failed

[Test 8] Short frame (only 4 bytes)
  frame: 01 03 0E 02
  correctly detected: frame too short
  result: 1 passed, 0 failed

========================================
  Total: 35 passed, 0 failed
========================================
```

### 8. 当日验收结论

2026-06-25 的 Modbus RTU 基础模块和传感器测试案例开发完成。

已完成内容：

- `modbus_rtu.h` 和 `modbus_rtu.c` 模块完成
- CRC16 校验实现
- 0x03 读保持寄存器请求帧构造
- 响应帧解析（含异常响应、CRC 校验、长度校验）
- 十六进制 dump 调试工具 (`modbus_hex_dump`)
- 8 传感器回复测试案例 (`modbus_run_sensor_tests`)，覆盖正常/故障/告警/边界/异常/CRC错误/短帧场景
- `modbus_read_registers()` — 串口+Modbus 一体化收发函数（构造请求→串口发送→超时接收→解析响应）
- `modbus_test_serial_read()` — 真实串口 Modbus 读取测试（有从机则显示真实数据，无从机则 fallback 到模拟测试）
- `main.c` 逻辑更新：串口打开成功→走真实 Modbus 通讯；串口失败→走纯模拟测试

### 9. 下一步计划

- 在 Linux 主机上 `make` 编译验证
- 交叉编译上传到 i.MX6ULL 开发板运行验证
- 若开发板有 CH32/STM32 采集板接入 RS485，可直接用 `modbus_test_serial_read` 读取真实数据
- 将 Modbus 响应解析结果替换模拟数据，接入 sample 数据结构
- 准备进入阶段 3（多线程架构和 SQLite 缓存）
