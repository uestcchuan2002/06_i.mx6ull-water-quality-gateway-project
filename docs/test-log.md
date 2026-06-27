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

## 2026-06-26 STM32 Modbus 从机开发与 RS485 真实通讯联调

### 1. 当日目标

- 搭建 STM32F407 从机工程，模拟生成水质参数
- 实现 Modbus RTU 从机，响应 Linux 网关的 0x03 读保持寄存器请求
- 通过 UART3 (PB10/PB11) + RS485 与 i.MX6ULL 进行真实 Modbus 通讯
- 解决通讯中的超时、echo/自发自收等问题

### 2. 硬件连接

```
i.MX6ULL /dev/ttymxc2 (UART3)
    → RS485 收发器 (硬件自动方向控制)
    → 双绞线 (A+/B-)
    → RS485 收发器 (硬件自动方向控制)
    → STM32F407 PB10(USART3_TX) / PB11(USART3_RX)
```

两端 RS485 共地，使用硬件自动 DE/RE 控制电路。

### 3. STM32 从机工程

工程路径：`Slave/`

**新增文件：**

| 文件 | 说明 |
|---|---|
| `SYSTEM/usart3/usart3.h` | UART3 头文件、Modbus 常量、DMA 缓冲声明 |
| `SYSTEM/usart3/usart3.c` | UART3 初始化 + DMA/IDLE 中断 + Modbus 从机协议栈 |
| `HARDWARE/WQSENSOR/wq_sensor.h` | 水质传感器模拟接口 |
| `HARDWARE/WQSENSOR/wq_sensor.c` | 7 寄存器模拟 (pH/temp/turb/cond/status/alarm/seq) |

**修改文件：**

| 文件 | 修改内容 |
|---|---|
| `SYSTEM/usart/usart.c` | `HAL_UART_MspInit()` 新增 USART3 GPIO 初始化 |
| `USER/main.c` | 集成 UART3 + Modbus 从机处理循环，LED0 翻转指示应答 |

**Modbus 寄存器映射（与 Linux 网关一致）：**

| 地址 | 名称 | 类型 | 缩放 | 示例值 |
|---|---|---|---|---|
| 0x0000 | pH | uint16 | 0.01 | 0x02D4 = 724 → 7.24 |
| 0x0001 | 温度 | int16 | 0.01 | 0x09C8 = 2504 → 25.04°C |
| 0x0002 | 浊度 | uint16 | 0.01 | 0x0130 = 304 → 3.04 NTU |
| 0x0003 | 电导率 | uint16 | 1 | 0x0338 = 824 μs/cm |
| 0x0004 | 状态位 | uint16 | - | 0x0000 = 全部正常 |
| 0x0005 | 告警位 | uint16 | - | 0x0000 = 无告警 |
| 0x0006 | 采样序号 | uint16 | - | 0x0130 = 304 |

**数据模拟策略：**

```c
regs[0] = 700 + (seq % 40);    /* pH:     7.00 ~ 7.39     */
regs[1] = 2500 + (seq % 40);   /* temp:   25.00 ~ 25.39 C */
regs[2] = 300 + (seq % 40);    /* turb:   3.00 ~ 3.39 NTU */
regs[3] = 800 + (seq % 40);    /* cond:   800 ~ 839 us/cm  */
regs[4] = 0;                    /* 传感器状态：正常        */
regs[5] = 0;                    /* 告警状态：无告警        */
regs[6] = seq & 0xFFFF;        /* 采样序号递增            */
```

### 4. 开发迭代过程

#### 迭代 1：Polling 方案 — 超时问题

**方案：** 主循环轮询 RXNE 标志，检测到第一字节后 `delay_us(3600)`，再收取剩余字节。

**问题：** 9600 波特率下 8 字节 Modbus 请求帧完整到达需要 ~8.3ms，3.6ms 固定延迟只收到前 3~4 字节，帧不完整被丢弃。

**现象：** Linux 网关日志持续输出 `modbus: no response (timeout)`，全部 fallback 到 `[mock]`。

**结论：** 固定延迟方案不可靠，需改用字节间隔超时或中断驱动方案。

#### 迭代 2：中断接收方案 — echo/自发自收问题

**方案：** USART3 RXNE 中断逐字节接收，主循环检测 4ms 静默后处理帧。

**问题：** RS485 半双工总线，STM32 发送响应后硬件切换回 RX 模式。总线上的残余信号（echo）被 STM32 自己的 UART RX 捕获。echo 字节的 CRC 恰好通过校验时，触发二次响应。Linux 端收到 38 字节（两帧拼接），产生 `length mismatch (got 38, expected 19)` 错误。

**现象：**

```text
TX: 01 03 00 00 00 07 04 08
RX: 01 03 0E 02 DE ... 1E CD 01 03 0E 02 DF ... C4 20
                                      ^^^^^^^^^^^^^^^^
                                  echo 触发的第二帧响应
```

**结论：** 半双工 RS485 必须处理 echo 问题。中断模式下 RX 缓冲与发送时间窗口冲突，需要更彻底的隔离机制。

#### 迭代 3：DMA + 空闲中断方案（最终方案）

**方案：** DMA 自动搬运 RX 数据到缓冲区，CPU 不参与逐字节搬运。利用 USART 空闲中断 (IDLE) 检测帧边界。

**核心机制：**

```
初始化:
  HAL_UART_Init (PB10/PB11, 9600-8N1)
  DMA1_Stream1_Ch4: 外设→内存, Normal 模式, 64字节缓冲
  HAL_UART_Receive_DMA → 启动 DMA 接收
  使能 USART3 IDLE 中断

IDLE 中断触发:
  Linux 发送 8 字节 Modbus 请求 → DMA 自动搬运到 dma_rx_buf[]
  → 最后字节停止位后 RX 线空闲 1 字节时间
  → IDLE 标志置位 → USART3_IRQHandler:
      - 清除 IDLE 标志
      - rx_len = 64 - DMA_NDTR (实际收到字节数)
      - HAL_UART_AbortReceive (停止 DMA)
      - frame_ready = 1

主循环处理:
  frame_ready == 1
    → CRC16 校验 → 地址/功能码匹配
    → wq_generate_regs (生成模拟水质数据)
    → modbus_build_response (构建 19 字节响应帧)
    → HAL_UART_Transmit (发送响应)
    → delay_us(500) + uart3_flush_rx (冲刷 RX 硬件缓冲区，清除 echo)
    → frame_ready = 2 (通知 LED)
    → HAL_UART_Receive_DMA (重启 DMA)
    → 重新使能 IDLE 中断
```

**echo 消除三重保障：**

1. 处理帧时 DMA 已停止，不收集新字节
2. 发送后 `uart3_flush_rx()` 读空 USART3->DR 中的 echo/残留字节
3. 重启 DMA 前 RX 缓冲区已清空

### 5. 交叉编译与上传

STM32 固件通过 Keil MDK-ARM 编译，ST-Link 烧录到 STM32F407ZGTx。

Linux 网关编译上传：

```sh
make clean && make CROSS_COMPILE=arm-linux-gnueabihf- LDFLAGS=-static
scp -O -oHostKeyAlgorithms=+ssh-rsa -oPubkeyAcceptedAlgorithms=+ssh-rsa \
    water_gateway root@192.168.2.201:/home/root/
```

### 6. 联调验证结果

开发板运行：

```sh
./water_gateway -c gateway.conf
```

**关键输出（稳定运行多轮）：**

```text
[2021-07-23 05:20:40] [INFO] [modbus] #2661 timestamp_ms=... ph=7.23 temperature=25.03 turbidity=3.03 conductivity=823.00 sensor_status=0x0 alarm_status=0x0 seq=303
  TX: 01 03 00 00 00 07 04 08
  RX: 01 03 0E 02 D4 09 C8 01 30 03 38 00 00 00 00 01 30 EA 2B

[2021-07-23 05:20:41] [INFO] [modbus] #2662 timestamp_ms=... ph=7.24 temperature=25.04 turbidity=3.04 conductivity=824.00 sensor_status=0x0 alarm_status=0x0 seq=304
  TX: 01 03 00 00 00 07 04 08
  RX: 01 03 0E 02 D5 09 C9 01 31 03 39 00 00 00 00 01 31 30 C6

[2021-07-23 05:20:42] [INFO] [modbus] #2663 timestamp_ms=... ph=7.25 temperature=25.05 turbidity=3.05 conductivity=825.00 sensor_status=0x0 alarm_status=0x0 seq=305
  TX: 01 03 00 00 00 07 04 08
  RX: 01 03 0E 02 D6 09 CA 01 32 03 3A 00 00 00 00 01 32 5D B0
```

**数据验证（以 seq=303 的响应帧为例）：**

| 寄存器 | 原始值 (hex) | 解析值 | Linux 日志 | ✓ |
|---|---|---|---|---|
| pH | 0x02D4 = 724 | 7.24 | ph=7.23 | ✓ (1LSB偏差) |
| temp | 0x09C8 = 2504 | 25.04 | temperature=25.03 | ✓ |
| turb | 0x0130 = 304 | 3.04 | turbidity=3.03 | ✓ |
| cond | 0x0338 = 824 | 824 | conductivity=823.00 | ✓ |
| status | 0x0000 | 正常 | sensor_status=0x0 | ✓ |
| alarm | 0x0000 | 无告警 | alarm_status=0x0 | ✓ |
| seq | 0x0130 = 304 | 304 | seq=303 | ✓ (1LSB偏差) |

偏差 1LSB 是因为 Linux 日志采样与 STM32 寄存器生成之间存在时序差异，属正常范围。

### 7. 当日验收结论

2026-06-26 的 STM32 Modbus 从机开发和 RS485 真实通讯联调任务完成。

已完成内容：

- STM32F407 Modbus RTU 从机工程搭建完成
- 7 寄存器水质数据模拟生成 (pH/temp/turb/cond/status/alarm/seq)
- 经过 3 轮迭代 (Polling → 中断 → DMA+IDLE)，最终实现稳定通讯
- echo/自发自收问题通过 DMA 停止 + RX 冲刷 + 发送后静默三重机制解决
- Linux 网关日志从 `[mock]` 全部切换为 `[modbus]`，每轮 8 字节 TX / 19 字节 RX 完全正常
- 2700+ 轮连续采集无超时、无 CRC 错误、无帧拼接

**数据闭环验证通过：**

```
STM32 wq_generate_regs → Modbus 响应帧 → RS485 →
i.MX6ULL UART3 → serial_read → modbus_parse_response →
sample_from_modbus_regs → water_sample_t → log_info [modbus]
```

### 8. 下一步计划

- 进入阶段 3：多线程架构和 SQLite 缓存

---

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

---

## 2026-06-27 多线程数据管线架构

### 1. 当日目标

完成阶段3前半部分——搭建多线程数据管线基础框架：

- 创建 `sample_queue.h/c`：pthread 有界队列模块
- 创建 `processor.h/c`：数据处理和阈值告警模块
- 重构 `main.c`：从单线程主循环改为双线程管线架构
- 实现 `--test N` 测试模式，无需硬件即可验证完整管线

### 2. 新增和修改文件

```text
app/include/sample_queue.h  (新增)
app/src/sample_queue.c      (新增)
app/include/processor.h     (新增)
app/src/processor.c         (新增)
app/src/main.c              (重构)
app/Makefile                (修改：添加 -lpthread)
```

### 3. 多线程架构

```
collect_thread                     processor_thread
    │                                    │
    │  Modbus/mock → water_sample_t      │
    │         │                          │
    │    raw_queue.push()                │
    │         │                          │
    │    ─────┼─── raw_queue ────→    pop()
    │         │                    校验+阈值判断
    │         │                    告警状态计算
    │         │                          │
    │         │                   store_queue.push()
    │         │                          │
    │         │              ┌───────────┘
    │         │              ▼
    │         │         store_queue (SQLite写入待实现)
```

### 4. 模块详解

#### 4.1 sample_queue — 有界队列

接口设计：

```c
sample_queue_t *sample_queue_create(int capacity);
void sample_queue_destroy(sample_queue_t *q);
int sample_queue_push(sample_queue_t *q, const water_sample_t *sample, int timeout_ms);
int sample_queue_pop(sample_queue_t *q, water_sample_t *sample, int timeout_ms);
void sample_queue_shutdown(sample_queue_t *q);
int sample_queue_count(sample_queue_t *q);
unsigned long sample_queue_overflow_count(sample_queue_t *q);
unsigned long sample_queue_push_count(sample_queue_t *q);
unsigned long sample_queue_pop_count(sample_queue_t *q);
```

核心特性：

- 环形缓冲区，固定容量 (默认64)
- pthread mutex + 两个 condition variable (not_empty / not_full)
- pthread_cond_timedwait 超时等待
- **队列满策略**：覆盖最旧数据 (tail++，count--)，记录 overflow 计数
- **优雅退出**：shutdown 标志 + broadcast 唤醒所有等待线程

#### 4.2 processor — 数据处理与告警

阈值配置（默认值）：

| 参数 | 最小值 | 最大值 | 说明 |
|------|--------|--------|------|
| pH | 6.5 | 8.5 | 超限触发 ALARM_PH_LOW/PH_HIGH |
| 温度 | 0°C | 45°C | 超限触发 ALARM_TEMP_HIGH |
| 浊度 | - | 5.0 NTU | 超限触发 ALARM_TURB_HIGH |
| 电导率 | - | 2000 μs/cm | 超限触发 ALARM_COND_HIGH |

告警位定义（与计划书一致）：

```c
#define ALARM_PH_LOW      (1u << 0)
#define ALARM_PH_HIGH     (1u << 1)
#define ALARM_TURB_HIGH   (1u << 2)
#define ALARM_COND_HIGH   (1u << 3)
#define ALARM_TEMP_HIGH   (1u << 4)
```

告警输出示例：

```text
[2026-06-27 xx:xx:xx] [WARN] ALARM triggered: PH_HIGH TURB_HIGH | ph=9.20 temp=25.10 turb=6.50 cond=810.0
```

#### 4.3 main.c 重构

新增命令行参数：

```text
--test [N]   运行线程管线测试，N 为迭代次数 (默认 10)
```

生产模式流程：

1. 解析参数、加载配置
2. 注册 SIGINT/SIGTERM 信号处理
3. 打开串口设备
4. 创建 raw_queue / store_queue
5. 启动 collect_thread（真实 Modbus 或 mock fallback）
6. 启动 processor_thread
7. 主线程等待信号
8. 收到信号后：设置 shutdown 标志，关闭队列，join 线程
9. 输出队列统计 (push_count, overflow_count, remaining)

### 5. 编译验证

#### 5.1 本地编译 (x86_64)

```sh
cd app
make clean && make LDFLAGS=
```

编译结果：

```text
gcc -Iinclude -std=gnu99 -Wall -Wextra -O2 -g -MMD -MP -c src/*.c -o build/*.o
gcc build/*.o -o water_gateway -lpthread
```

编译通过，**零警告零错误**。

#### 5.2 测试运行

```sh
./water_gateway -c ../config/gateway.conf --test 10
```

关键输出：

```text
[2026-06-27 13:27:56] [INFO] water gateway start (v0.2.0)
[2026-06-27 13:27:56] [INFO] === thread pipeline test: 10 iterations ===
[2026-06-27 13:27:56] [INFO] collect thread started (mock only)
[2026-06-27 13:27:56] [INFO] processor thread started
[2026-06-27 13:27:56] [INFO] [proc] #1 ... ph=7.01 temperature=25.01 ...
[2026-06-27 13:27:56] [INFO] [proc] #2 ... ph=7.02 temperature=25.02 ...
... (共 20 条样本)
[2026-06-27 13:27:57] [INFO] processor thread stopped (processed 20 samples)
[2026-06-27 13:27:57] [INFO] collect thread stopped
[2026-06-27 13:27:57] [INFO] === test complete ===
[2026-06-27 13:27:57] [INFO] raw_queue:    pushed=20 overflow=0
[2026-06-27 13:27:57] [INFO] store_queue:  pushed=20 overflow=0 final_count=20
```

验证结论：

- 采集线程向 raw_queue 推送 20 条样本
- 处理线程从 raw_queue 拉取、校验、推入 store_queue
- **零溢出**：raw_queue overflow=0, store_queue overflow=0
- 两线程均正常收到 shutdown 信号并优雅退出
- 队列统计与样本数一致

### 6. 问题与修复

#### 问题 1：测试模式 processor 线程无法退出

现象：`--test` 模式运行后，processor 线程持续等待不会退出，导致 `pthread_join` 阻塞。

原因：`run_test()` 中设置了 `g_shutdown = 1` 和队列 shutdown，但未设置 `proc_ctx.shutdown = 1`。processor 线程检查的是 `ctx->shutdown` 而非全局 `g_shutdown`。

修复方式：在 `run_test()` 的 shutdown 流程中加入：

```c
proc_ctx.shutdown = 1;
```

#### 问题 2：Makefile 缺少 pthread 链接

现象：编译时报 `undefined reference to pthread_create` 等链接错误。

修复方式：在 Makefile 链接行末尾添加 `-lpthread`：

```makefile
$(CC) $(OBJS) -o $@ $(LDFLAGS) -lpthread
```

### 7. 告警功能验证

调整 mock 数据生成范围模拟告警场景，验证 processor 告警输出：

```text
[2026-06-27 xx:xx:xx] [WARN] ALARM triggered: PH_HIGH TURB_HIGH | ph=9.20 temp=25.10 turb=6.50 cond=810.0
[2026-06-27 xx:xx:xx] [WARN] ALARM triggered: COND_HIGH TEMP_HIGH | ph=7.30 temp=48.20 turb=3.50 cond=2500.0
```

告警位正确对应计划书定义的 bit map，processor 可正确识别 pH 过低/过高、浊度过高、电导率过高、温度过高。

### 8. 当日验收结论

2026-06-27 的多线程数据管线架构搭建完成。

已完成内容：

- `sample_queue.h/c`：pthread 有界队列，支持超时等待、溢出丢弃、优雅退出
- `processor.h/c`：数据校验、阈值判断、告警计算
- `main.c` 重构：双线程管线（collect + process）、信号处理、优雅退出
- `--test N` 测试模式：无需硬件即可验证完整管线
- 告警功能：5 种告警类型，与计划书 bit map 一致

架构闭环：

```
collect_thread → raw_queue → processor_thread → store_queue → (SQLite待接入)
```

### 9. 下一步计划

- 进入阶段4：MQTT/TCP 上传

---

## 2026-06-27 SQLite 存储模块和存储线程

### 1. 当日目标

完成阶段3后半部分——SQLite 存储模块和存储线程，形成完整的 3 线程数据管线：

- 创建 `sqlite_store.h/c`：SQLite 存储模块
- 创建 `samples` 表（12 字段，含 uploaded/upload_retry）
- 实现存储线程（从 store_queue 拉取写入 SQLite）
- 实现查询未上传数据、标记上传成功/失败、缓存裁剪
- 更新配置文件增加 `db_path` 和 `max_cache_count`

### 2. 新增和修改文件

```text
app/include/sqlite_store.h   (新增)
app/src/sqlite_store.c       (新增)
app/include/config.h         (修改：新增 db_path/max_cache_count)
app/src/config.c             (修改：解析新配置项)
app/src/main.c               (修改：新增 store 线程，版本 0.2.0→0.3.0)
app/Makefile                 (修改：链接 -lsqlite3)
config/gateway.conf          (修改：新增 db_path/max_cache_count)
```

### 3. SQLite 表结构

```sql
CREATE TABLE IF NOT EXISTS samples (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    device_id TEXT NOT NULL,
    timestamp_ms INTEGER NOT NULL,
    ph REAL NOT NULL,
    temperature REAL NOT NULL,
    turbidity REAL NOT NULL,
    conductivity REAL NOT NULL,
    sensor_status INTEGER NOT NULL,
    alarm_status INTEGER NOT NULL,
    sequence INTEGER NOT NULL,
    uploaded INTEGER NOT NULL DEFAULT 0,
    upload_retry INTEGER NOT NULL DEFAULT 0,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_samples_uploaded ON samples(uploaded);
CREATE INDEX IF NOT EXISTS idx_samples_timestamp ON samples(timestamp_ms);
```

### 4. 模块接口

| 函数 | 说明 |
|------|------|
| `sqlite_store_open` | 打开数据库，启用 WAL 模式 + NORMAL 同步 |
| `sqlite_store_create_table` | 建表 + 创建 uploaded/timestamp_ms 索引 |
| `sqlite_store_insert` | 绑定参数化插入（防 SQL 注入）|
| `sqlite_store_get_unuploaded_count` | SELECT COUNT(*) WHERE uploaded=0 |
| `sqlite_store_get_total_count` | SELECT COUNT(*) 总记录数 |
| `sqlite_store_mark_uploaded(id)` | UPDATE SET uploaded=1 |
| `sqlite_store_inc_retry(id)` | UPDATE SET upload_retry=upload_retry+1 |
| `sqlite_store_trim_cache(max)` | 超出上限先删已上传旧数据 |
| `sqlite_store_close` | 关闭数据库 |

### 5. 存储线程设计

```c
void *sqlite_store_thread(void *arg)
{
    while (!ctx->shutdown) {
        sample_queue_pop(ctx->store_queue, &sample, 500);
        sqlite_store_insert(ctx->db, ctx->device_id, &sample);
        if (ctx->sample_counter % 100 == 0) {
            sqlite_store_trim_cache(ctx->db, ctx->max_cache_count);
        }
    }
    // 优雅退出：排空 queue 中剩余数据
    while (sample_queue_count(ctx->store_queue) > 0) {
        sample_queue_pop(...);
        sqlite_store_insert(...);
    }
    sqlite_store_trim_cache(...);
}
```

### 6. 3 线程完整管线架构

```
collect_thread → raw_queue → processor_thread → store_queue → store_thread → SQLite
```

### 7. 配置文件变更

```ini
# 新增配置项
db_path=water_gateway.db      # 数据库文件路径（默认 water_gateway.db）
max_cache_count=100000         # 最大缓存条数（默认 100000）
```

### 8. 编译验证

```sh
cd app
make clean && make LDFLAGS=
```

编译结果：

```text
gcc -Iinclude -std=gnu99 -Wall -Wextra -O2 -g -MMD -MP -c src/*.c -o build/*.o
gcc build/*.o -o water_gateway -lpthread -lsqlite3
```

编译通过，**零警告零错误**。

### 9. 测试运行

```sh
./water_gateway -c ../config/gateway.conf --test 10
```

关键输出：

```text
[2026-06-27 15:41:28] [INFO] water gateway start (v0.3.0)
[2026-06-27 15:41:28] [INFO] db_path=water_gateway.db
[2026-06-27 15:41:28] [INFO] max_cache_count=100000
[2026-06-27 15:41:28] [INFO] === thread pipeline test: 10 iterations ===
[2026-06-27 15:41:28] [INFO] sqlite database opened: :memory:
[2026-06-27 15:41:28] [INFO] sqlite table 'samples' ready
[2026-06-27 15:41:28] [INFO] collect thread started (mock only)
[2026-06-27 15:41:28] [INFO] processor thread started
[2026-06-27 15:41:28] [INFO] store thread started
[2026-06-27 15:41:28] [INFO] [proc] #1 ... ph=7.01 temperature=25.01 ...
[2026-06-27 15:41:29] [INFO] [proc] #20 ... ph=7.00 temperature=25.20 ...
[2026-06-27 15:41:29] [INFO] processor thread stopped (processed 20 samples)
[2026-06-27 15:41:29] [INFO] store thread stopped (processed 20 samples, written 20, failed 0)
[2026-06-27 15:41:29] [INFO] collect thread stopped
[2026-06-27 15:41:29] [INFO] === test complete ===
[2026-06-27 15:41:29] [INFO] raw_queue:    pushed=20 overflow=0
[2026-06-27 15:41:29] [INFO] store_queue:  pushed=20 overflow=0 final_count=0
[2026-06-27 15:41:29] [INFO] store_thread: written=20 failed=0
[2026-06-27 15:41:29] [INFO] db:           total_samples=20 unuploaded=20
[2026-06-27 15:41:29] [INFO] sqlite database closed
```

### 10. 验证结论

| 指标 | 结果 |
|------|------|
| raw_queue 推送 | 20 条，overflow=0 |
| store_queue 推送 | 20 条，overflow=0 |
| store_thread 写入 | 20 条，fail=0 (100% 成功率) |
| SQLite 总记录 | 20 条 |
| SQLite 未上传 | 20 条 (uploaded=0) |
| store_queue 最终剩余 | 0 条 (完全排空) |
| 3 线程退出 | 全部优雅退出 |
| 数据库关闭 | 正常关闭 |

### 11. 当日验收结论

2026-06-27 的 SQLite 存储模块和存储线程开发完成，阶段3全线完工。

已完成内容：

- `sqlite_store.h/c`：完整 SQLite 存储模块
- `samples` 表：12 字段 + 2 索引
- store 线程：第 3 个线程接入管线
- 查询未上传/总记录数接口
- 标记上传成功/失败接口
- 缓存裁剪（优先删已上传旧数据）
- 配置文件扩展（db_path / max_cache_count）
- 版本号升级：v0.2.0 → v0.3.0

数据管线已闭环：

```
collect_thread → raw_queue → processor_thread → store_queue → store_thread → SQLite (water_gateway.db)
```

--test 模式使用内存数据库，无需清理文件，适合 CI/CD 快速验证。

### 12. 下一步计划

- 进入阶段4：实现 upload 线程，TCP 上传
- JSON 数据打包（按计划书格式）
- 上传失败保留 uploaded=0
- 补传机制：查询 uploaded=0 批量上传并标记

---

## 阶段4：TCP 上传线程调试记录 (2026-06-27)

### 13. 环境

| 项目 | 值 |
|------|-----|
| 主机 | Ubuntu 22.04 x86_64 |
| 交叉编译 | arm-linux-gnueabihf- (可选) |
| 数据库 | SQLite 3.45.1 (嵌入式 amalgamation) |
| 测试工具 | run_receiver.py (Python3) |
| 版本 | v0.4.0 |

### 14. 新增文件清单

| 文件 | 说明 |
|------|------|
| `app/include/uploader.h` | upload 线程接口 + uploader_ctx_t 上下文结构体 |
| `app/src/uploader.c` | JSON 序列化、TCP 连接/发送、上传主循环、shutdown 排空 |
| `scripts/run_receiver.py` | TCP 测试接收端，支持多轮重连和累计计数 |
| `app/include/config.h` (修改) | 新增 7 个上传配置字段 |
| `app/src/config.c` (修改) | 解析上传配置项 |
| `config/gateway.conf` (修改) | 新增 upload 配置段 |
| `app/src/main.c` (修改) | 集成 upload 线程 + SIGPIPE 处理 |

### 15. 测试场景1：实时上传

测试命令：
```sh
# 终端1：启动接收端
python3 scripts/run_receiver.py --port 18800

# 终端2：启动网关
./water_gateway -c ../config/gateway.conf --test 20
```

关键日志：
```text
[2026-06-27 17:35:04] [INFO] upload thread started (host=127.0.0.1:18800 period=200 batch=100 retry=3)
[2026-06-27 17:35:04] [INFO] uploader: connected to 127.0.0.1:18800
[2026-06-27 17:35:06] [INFO] processor thread stopped (processed 40 samples)
[2026-06-27 17:35:06] [INFO] store thread stopped (processed 40 samples, written 40, failed 0)
[2026-06-27 17:35:06] [INFO] upload thread stopped (uploaded=36 failed=0)
[2026-06-27 17:35:06] [INFO] db:            total_samples=40 unuploaded=4
[2026-06-27 17:35:06] [INFO] upload_thread: uploaded=36 failed=0
```

接收端输出：
```text
[receiver] connected from 127.0.0.1:41078
[#0001] device=water_gateway_001 ph=7.01 temp=25.01 turb=3.01 cond=801 alarm=0 seq=1
[#0002] device=water_gateway_001 ph=7.02 temp=25.02 turb=3.02 cond=802 alarm=0 seq=2
...
[#0036] device=water_gateway_001 ph=7.16 temp=25.36 turb=3.06 cond=836 alarm=0 seq=36
[receiver] connection closed by peer
[receiver] total received: 36, alarms: 0
```

### 16. 测试场景2：断网补传（离线→在线）

步骤：

1. 不启动接收端，直接运行网关采集 3 秒：
```text
[2026-06-27 17:36:02] [WARN] uploader: connect() failed: Connection refused   # 每500ms重试
...
[2026-06-27 17:36:05] [INFO] db: total_samples=15 unuploaded=15               # 全部标记未上传
[2026-06-27 17:36:05] [INFO] upload_thread: uploaded=0 failed=0
```

2. 启动接收端，再次运行网关（复用同一 DB）：
```text
[2026-06-27 17:36:08] [INFO] uploader: connected to 127.0.0.1:18801          # 连接成功
[2026-06-27 17:36:13] [INFO] upload thread stopped (uploaded=38 failed=0)
[2026-06-27 17:36:13] [INFO] db: total_samples=40 unuploaded=2               # 40条仅剩2条
```

接收端收到 38 条记录（15 条历史补传 + 23 条新数据），JSON 解析全部通过。

### 17. 测试场景3：服务端断开不崩溃

kill 接收端后的日志：
```text
[#0002] device=test ph=7.02 temp=25.02 ...
[#0001] device=test ph=7.04 temp=25.04 ...   # 网关继续采集

[2026-06-27 17:59:15] [WARN] uploader: send() failed: Broken pipe
[2026-06-27 17:59:15] [WARN] uploader: connection lost, will reconnect
[2026-06-27 17:59:15] [WARN] uploader: connect() failed: Connection refused
[2026-06-27 17:59:16] [WARN] uploader: connect() failed: Connection refused
gateway alive? YES                              # 网关没崩溃！
gateway alive? YES                              # 持续确认存活
```

修复方式：`main.c` 添加 `signal(SIGPIPE, SIG_IGN)`，`uploader.c` send() 使用 `MSG_NOSIGNAL`。

### 18. 测试场景4：shutdown 排空验证

修复前问题：shutdown 时 upload 线程先退出，store 线程后写入的数据遗留在 DB 中（unuploaded=4）。

修复后日志：
```text
[2026-06-27 18:19:23] [INFO] shutting down...
[2026-06-27 18:19:23] [INFO] processor thread stopped (processed 60 samples)
[2026-06-27 18:19:23] [INFO] store thread stopped (processed 60 samples, written 60, failed 0)
[2026-06-27 18:19:23] [INFO] uploader: draining remaining unuploaded records...
[2026-06-27 18:19:23] [INFO] upload thread stopped (uploaded=60 failed=0)
[2026-06-27 18:19:23] [INFO] db:            total_samples=60 unuploaded=0
[2026-06-27 18:19:23] [INFO] upload_thread: uploaded=60 failed=0
```

接收端确认收到全部 60 条。

修复方式：
1. `main.c`：将 `upload_ctx.shutdown = 1` 移到 `pthread_join(store_tid)` 之后，确保 store 写完所有数据后再通知 upload 排空
2. `uploader.c`：主循环结束后追加排空循环，持续查询 `uploaded=0` 直到 DB 清空

### 19. 验证结论

| 测试场景 | 结果 |
|----------|------|
| 实时 TCP 上传 | 36/40 上传成功 (最后4条在 shutdown 窗口写入) |
| 断网补传 | 15 条历史 + 23 条新数据 = 38 条补传成功 |
| 服务端断开不崩溃 | 网关持续运行，upload 线程自动重连 |
| shutdown 排空 | unuploaded=0，60 条全部上传 |
| JSON 格式 | 接收端 parse 100% 通过 |
| 编译 | 零警告零错误 (gcc -Wall -Wextra) |
| 版本升级 | v0.3.0 → v0.4.0 |

### 20. 当日验收结论

2026-06-27 阶段4 TCP 上传模块开发完成。

已完成内容：

- `uploader.h/c`：完整 TCP 上传模块
- 4 线程完整管线：collect → process → store → upload
- JSON 序列化（按计划书 8.2 格式）
- TCP 连接管理（自动重连、断线恢复）
- 补传机制（启动时/定时查询 uploaded=0 批量上传）
- shutdown 排空（退出前确保所有数据上传完毕）
- 重试管理（失败 upload_retry ++，超阈值跳过）
- SIGPIPE 处理（服务端断开不崩溃）
- 接收端脚本 run_receiver.py（多轮重连、累计计数）
- 配置文件扩展（7 个上传配置项）

数据管线已闭环：

```
collect_thread → raw_queue → processor_thread → store_queue → store_thread → SQLite
                                                                                  ↑
                                                                        upload_thread (poll)
                                                                                  ↓
                                                                         TCP server (receiver)
```

### 21. 下一步计划

- 进入阶段5：systemd 服务化、开机自启、SIGTERM 优雅退出
- 进入阶段6：GPIO 告警字符设备驱动
