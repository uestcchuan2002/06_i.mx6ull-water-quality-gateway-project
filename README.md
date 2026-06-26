# Water Quality Linux Gateway

## 项目目标

基于 i.MX6ULL 实现工业水质监测 Linux 网关，支持 RS485/Modbus RTU 数据采集、SQLite 缓存、TCP/MQTT 上传和本地告警。

## 当前进度

- 2026-06-22：完成开发板 Linux 启动、串口终端、SSH、root 权限验证
- 2026-06-23：完成应用层最小 Demo 框架 (Makefile/config/logger/sample)
- 2026-06-24：完成 i.MX6ULL 开发板运行验证、RTC 时间修复、串口模块
- 2026-06-25：完成 Modbus RTU 主站模块 (CRC16/0x03请求/响应解析/8个测试案例)
- 2026-06-26：完成 STM32F407 Modbus 从机开发 + RS485 真实通讯联调
- 2026-06-26：经过 3 轮迭代 (Polling→中断→DMA+IDLE) 实现稳定通讯，2700+ 轮无异常

## 当前 Demo 功能

当前版本已经支持：

- 读取设备配置：`device_id`、`sample_period_ms`、`log_level`、`serial_device`、`baudrate`、`modbus_slave_addr`
- 初始化日志模块，按配置等级输出运行状态
- Linux 串口模块：open/config/read/write/close (`/dev/ttymxc2`)
- Modbus RTU 主站 (i.MX6ULL)：
  - CRC16 校验（多项式 0xA001）
  - 0x03 读保持寄存器请求帧构造
  - 响应帧解析（异常响应检测、CRC 校验、大端解包）
  - 十六进制 dump 调试输出
  - 串口+Modbus 一体化收发 (`modbus_read_registers`)
- Modbus RTU 从机 (STM32F407)：
  - 7 寄存器水质数据模拟 (pH/temp/turb/cond/status/alarm/seq)
  - DMA1_Stream1 + USART3 IDLE 中断帧边界检测
  - 三重 echo 消除机制
- 主循环周期 Modbus 采集：串口可用时自动走 Modbus 采集，失败/超时时自动 fallback 模拟数据
- 日志标识数据来源：`[modbus]` 真实采集 / `[mock]` 模拟 fallback

## 目录结构

```text
03_water_quality_gateway_project/
  app/
    Makefile
    include/
      config.h
      logger.h
      sample.h
      serial_port.h
      modbus_rtu.h
    src/
      main.c
      config.c
      logger.c
      sample.c
      serial_port.c
      modbus_rtu.c
  config/
    gateway.conf
  docs/
    test-log.md
  driver/
  scripts/

Slave/                              (STM32F407 Modbus 从机工程)
  USER/
    main.c                          (Modbus 从机主循环)
  SYSTEM/
    usart/usart.c/h                 (USART1 调试串口 + USART3 MSP)
    usart3/usart3.c/h               (DMA+IDLE 中断 Modbus 从机协议栈)
    delay/delay.c/h                 (SysTick 延时)
    sys/sys.c/h                     (时钟、位带操作)
  HARDWARE/
    LED/led.c/h                     (LED 指示)
    WQSENSOR/wq_sensor.c/h          (水质数据模拟)
```

## 编译和运行

### Linux 网关

```sh
cd app
make clean && make CROSS_COMPILE=arm-linux-gnueabihf- LDFLAGS=-static
scp -O water_gateway root@192.168.2.201:/home/root/
ssh root@192.168.2.201 "./water_gateway -c gateway.conf"
```

### STM32 从机

在 Keil MDK-ARM 中打开 `Slave/USER/Slave.uvprojx`，编译后通过 ST-Link 烧录到 STM32F407ZGTx。

**工程文件（Keil 中需手动添加）：**
- `SYSTEM/usart3/usart3.c`
- `HARDWARE/WQSENSOR/wq_sensor.c`

Include paths 需添加：`SYSTEM/usart3`、`HARDWARE/WQSENSOR`

## 配置文件

```ini
device_id=water_gateway_001
sample_period_ms=1000
log_level=info
serial_device=/dev/ttymxc2
baudrate=9600
modbus_slave_addr=1
```

## 硬件连接

```
i.MX6ULL /dev/ttymxc2 (UART3)          STM32F407 USART3
    TX ────────────────────────────────────> RX (PB11)
    RX <──────────────────────────────────── TX (PB10)
         │                                          │
         └─── RS485 收发器 ─── 双绞线 ─── RS485 收发器 ───┘
              (硬件自动 DE/RE)                  (硬件自动 DE/RE)
```

## RS485 Modbus 通讯验证结果

稳定运行，每轮 TX 8 字节 / RX 19 字节，CRC 校验通过：

```text
[2021-07-23 05:20:40] [INFO] [modbus] #2661 ph=7.23 temperature=25.03 ...
  TX: 01 03 00 00 00 07 04 08
  RX: 01 03 0E 02 D4 09 C8 01 30 03 38 00 00 00 00 01 30 EA 2B
[2021-07-23 05:20:41] [INFO] [modbus] #2662 ph=7.24 temperature=25.04 ...
  TX: 01 03 00 00 00 07 04 08
  RX: 01 03 0E 02 D5 09 C9 01 31 03 39 00 00 00 00 01 31 30 C6
```

## 当前代码状态 (2026-06-26)

```
已实现：
  ✅ 配置文件读取 (config.c)
  ✅ 日志模块 (logger.c)
  ✅ 水质数据结构 (sample.c)
  ✅ Linux 串口封装 (serial_port.c)
  ✅ Modbus RTU 主站 (modbus_rtu.c)
  ✅ STM32F407 Modbus 从机 (DMA+IDLE 中断)
  ✅ RS485 真实通讯联调 (i.MX6ULL ↔ STM32)
  ✅ Modbus 采集 + mock 自动 fallback 主循环
  ✅ 交叉编译 + 静态链接 + 开发板运行

待实现：
  ❌ 多线程架构（阶段 3）
  ❌ SQLite 缓存
  ❌ MQTT/TCP 上传
  ❌ systemd 服务化
  ❌ GPIO 告警驱动
```

## 下一步计划

- 进入阶段 3：多线程架构 (pthread) 和 SQLite 断网缓存
- 将采集、处理、存储、上传拆分为独立线程
- 实现断网数据本地存储和网络恢复后补传
