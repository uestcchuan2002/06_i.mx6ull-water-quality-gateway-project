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
- 2026-06-27：完成阶段3前半部分——多线程数据管线架构 (pthread) 和告警处理模块
- 2026-06-27：完成阶段3后半部分——SQLite 存储模块和存储线程 (3线程完整管线)
- 2026-06-28：完成阶段5——sysvinit init.d 脚本、部署安装、开发板全功能验证 (start/stop/restart/status/SIGTERM/自启/崩溃恢复)

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
- 多线程数据管线（阶段3）：
  - collect 线程：周期 Modbus 采集或 mock 数据生成，推入 raw_queue
  - process 线程：从 raw_queue 拉取样本，数据校验、阈值判断、告警状态计算，推入 store_queue
  - sample_queue：pthread mutex + condition variable 有界队列，支持超时等待和优雅退出
  - processor：可配置阈值 (pH/temp/turb/cond)，超限自动触发告警
- `--test N` 测试模式：无需硬件即可验证完整线程管线
- SQLite 存储模块（阶段3）：
  - `samples` 表：12 字段 (id/device_id/timestamp_ms/ph/temperature/turbidity/conductivity/sensor_status/alarm_status/sequence/uploaded/upload_retry/created_at)
  - 绑定参数化插入，WAL 模式 + NORMAL 同步
  - `uploaded` 索引和 `timestamp_ms` 索引
  - 上传状态标记 (`mark_uploaded`)、重试计数 (`inc_retry`)
  - 缓存裁剪 (`trim_cache`)：优先删除已上传旧数据，超出上限强制清理
  - store 线程：从 store_queue 拉取，批量写入 + 定期 trim
  - 优雅退出：shutdown 时排空队列后关闭数据库
  - 查询接口：`get_unuploaded_count` / `get_total_count`
- TCP 上传模块（阶段4）：
  - upload 线程：4 线程完整管线 (collect → process → store → upload)
  - JSON 序列化：按协议格式打包水质数据
  - TCP 连接管理：自动重连、优雅断线恢复
  - 补传机制：启机时/定时查询 uploaded=0 记录批量上传
  - shutdown 排空：退出前清空所有未上传记录
  - 重试管理：失败记录 upload_retry 递增，超阈值跳过
  - 服务端断开不崩溃：SIGPIPE 信号忽略 + MSG_NOSIGNAL
  - --test 模式支持完整 4 线程上传测试
- sysvinit 服务化（阶段5）：
  - init.d 脚本：start/stop/restart/status 四个 action
  - PID 文件管理 (/var/run/water_gateway.pid)
  - 日志输出：stdout + 文件双写 (log_file 配置项)
  - 心跳日志：每 60s 输出采集/存储/上传统计
  - 验证通过：SIGTERM 优雅退出 (80条零丢失)、开机自启、崩溃恢复、完整数据链路

## 目录结构

```text
03_water_quality_gateway_project/
  app/
    Makefile
    include/
      config.h
      logger.h
      sample.h
      sample_queue.h
      processor.h
      serial_port.h
      modbus_rtu.h
      sqlite_store.h
      uploader.h
    src/
      main.c
      config.c
      logger.c
      sample.c
      sample_queue.c
      processor.c
      serial_port.c
      modbus_rtu.c
      sqlite_store.c
      uploader.c
  config/
    gateway.conf
  docs/
    test-log.md
  driver/
  system/
    water-gateway.sh              (init.d 脚本)
    install.sh                    (一键安装脚本)
  scripts/
    run_receiver.py              (TCP 上传测试接收端)

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

### 编译依赖

编译前需安装以下开发库：

```sh
# Debian/Ubuntu
sudo apt-get install -y libsqlite3-dev

# CentOS/RHEL/Fedora
sudo yum install -y sqlite-devel
```

交叉编译 ARM 版本同理，需确保交叉编译工具链中包含 sqlite3 库（头文件位于工具链 sysroot 的 `/usr/include/sqlite3.h`）。

### Linux 网关

```sh
cd app
make clean && make CROSS_COMPILE=arm-linux-gnueabihf- LDFLAGS=-static
scp -O -oHostKeyAlgorithms=+ssh-rsa -oPubkeyAcceptedAlgorithms=+ssh-rsa \
    water_gateway root@192.168.2.201:/home/root/
ssh -oHostKeyAlgorithms=+ssh-rsa -oPubkeyAcceptedAlgorithms=+ssh-rsa \
    root@192.168.2.201 "./water_gateway -c gateway.conf"
```

### 线程管道测试（无需硬件）

```sh
cd app
make clean && make
./water_gateway -c ../config/gateway.conf --test 10
```

测试模式使用 mock 数据在 4 个线程中运行完整管线，输出队列和上传统计信息。适用于本地 PC 或开发板快速验证。

### 上传测试（需两个终端）

```sh
# 终端 1：启动 TCP 接收端
cd scripts
python3 run_receiver.py --port 18800

# 终端 2：启动网关（接收端在 Ubuntu 则改 config 中 upload_server_host 为 Ubuntu IP）
cd app
make clean && make
./water_gateway -c ../config/gateway.conf
```

接收端会实时打印每条收到的 JSON 记录，网关停机后接收端自动等待重连。

### 多线程架构

```
collect_thread       processor_thread        store_thread         upload_thread
     │                      │                      │                     │
     │ Modbus/mock →       │                      │                     │
     │ water_sample_t      │                      │                     │
     │      │               │                      │                     │
     │ raw_queue.push()    │                      │                     │
     │      │               │                      │                     │
     │  ────┼── raw_queue → pop()                  │                     │
     │      │           校验+阈值判断               │                     │
     │      │           告警状态计算                │                     │
     │      │               │                      │                     │
     │      │       store_queue.push()             │                     │
     │      │               │                      │                     │
     │      │       ───────┼── store_queue → pop() │                     │
     │      │               │              SQLite INSERT                │
     │      │               │              缓存裁剪(每100条)            │
     │      │               │                      │                     │
     │      │               │                      └── water_gateway.db  │
     │      │               │                              ▲              │
     │      │               │                              │              │
     │      │               │                    upload_thread poll      │
     │      │               │                  SELECT WHERE uploaded=0   │
     │      │               │                          │                 │
     │      │               │                    JSON 序列化              │
     │      │               │                      TCP send              │
     │      │               │                          │                 │
     │      │               │                  ┌───────┘                 │
     │      │               │                  ▼                         │
     │      │               │           run_receiver.py (TCP server)     │
```

### STM32 从机

在 Keil MDK-ARM 中打开 `Slave/USER/Slave.uvprojx`，编译后通过 ST-Link 烧录到 STM32F407ZGTx。

**工程文件（Keil 中需手动添加）：**
- `SYSTEM/usart3/usart3.c`
- `HARDWARE/WQSENSOR/wq_sensor.c`

Include paths 需添加：`SYSTEM/usart3`、`HARDWARE/WQSENSOR`

### 开发板部署

开发板使用 sysvinit（非 systemd），通过 init.d 脚本管理服务：

```sh
# 1. 交叉编译 + 上传
cd app
make clean && make CROSS_COMPILE=arm-linux-gnueabihf- LDFLAGS=-static
scp -O -oHostKeyAlgorithms=+ssh-rsa -oPubkeyAcceptedAlgorithms=+ssh-rsa \
    water_gateway root@192.168.2.201:/home/root/
scp -O -oHostKeyAlgorithms=+ssh-rsa -oPubkeyAcceptedAlgorithms=+ssh-rsa \
    ../config/gateway.conf root@192.168.2.201:/home/root/
scp -O -oHostKeyAlgorithms=+ssh-rsa -oPubkeyAcceptedAlgorithms=+ssh-rsa \
    ../system/water-gateway.sh root@192.168.2.201:/home/root/
scp -O -oHostKeyAlgorithms=+ssh-rsa -oPubkeyAcceptedAlgorithms=+ssh-rsa \
    ../system/install.sh root@192.168.2.201:/home/root/

# 2. 安装
ssh -oHostKeyAlgorithms=+ssh-rsa -oPubkeyAcceptedAlgorithms=+ssh-rsa \
    root@192.168.2.201
chmod +x install.sh && ./install.sh

# 3. 使用
/etc/init.d/water_gateway start
/etc/init.d/water_gateway status
/etc/init.d/water_gateway stop
/etc/init.d/water_gateway restart
```

安装后路径：
```text
/usr/bin/water_gateway          可执行文件
/etc/water_gateway.conf         配置文件
/var/lib/water_gateway/         数据库和运行数据
/var/run/water_gateway.pid      PID 文件
/var/log/water_gateway.log      日志文件
/etc/init.d/water_gateway       init.d 脚本
```

## SQLite 数据库

### 表结构

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
```

### 接口

| 函数 | 说明 |
|------|------|
| `sqlite_store_open` | 打开/创建数据库 (WAL + NORMAL同步) |
| `sqlite_store_create_table` | 建表 + 创建 uploaded/timestamp_ms 索引 |
| `sqlite_store_insert` | 绑定参数化插入一条采样记录 |
| `sqlite_store_get_unuploaded_count` | 查询未上传记录数 |
| `sqlite_store_get_total_count` | 查询总记录数 |
| `sqlite_store_mark_uploaded(id)` | 标记某条记录已上传 |
| `sqlite_store_inc_retry(id)` | 上传重试计数 +1 |
| `sqlite_store_trim_cache(max)` | 超上限时优先删除已上传旧数据 |

## 配置文件

```ini
device_id=water_gateway_001
sample_period_ms=1000
log_level=info
serial_device=/dev/ttymxc2
baudrate=9600
modbus_slave_addr=1
db_path=water_gateway.db
max_cache_count=100000

# upload config
upload_enabled=1
upload_protocol=tcp
upload_server_host=192.168.2.150
upload_server_port=18800
upload_period_ms=5000
upload_batch_max=20
upload_retry_max=3

# log config
log_file=stdout
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

## 当前代码状态 (2026-06-28)

```
已实现：
  ✅ 配置文件读取 (config.c)
  ✅ 日志模块 (logger.c)
  ✅ 水质数据结构 (sample.c)
  ✅ Linux 串口封装 (serial_port.c)
  ✅ Modbus RTU 主站 (modbus_rtu.c)
  ✅ STM32F407 Modbus 从机 (DMA+IDLE 中断)
  ✅ RS485 真实通讯联调 (i.MX6ULL ↔ STM32)
  ✅ pthread 有界队列 (sample_queue.c)
  ✅ 数据处理 + 阈值告警 (processor.c)
  ✅ 多线程数据管线 (collect + process 线程)
  ✅ --test N 管道测试模式
  ✅ 交叉编译 + 静态链接 + 开发板运行
  ✅ SQLite 存储模块 (sqlite_store.c) - 建表/插入/查询/标记上传/缓存裁剪
  ✅ 存储线程 - 3 线程完整管线 (collect → process → store)
  ✅ TCP 上传模块 (uploader.c) - JSON 序列化/TCP 发送/自动重连/补传/排空
  ✅ upload 线程 - 4 线程完整管线 (collect → process → store → upload)
  ✅ 接收端测试脚本 (run_receiver.py) - 支持多轮重连、累计计数
  ✅ SIGPIPE 信号处理 - 服务端断开网关不崩溃
  ✅ sysvinit init.d 脚本 - start/stop/restart/status (适配开发板 sysvinit)
  ✅ 一键安装部署脚本 (install.sh)
  ✅ 心跳日志 - 每 60s 输出采集/存储/上传统计
  ✅ 文件日志支持 - log_file 配置项可选写文件
  ✅ 开发板部署测试 - SIGTERM/自启/崩溃恢复/数据链路全部通过

待实现：
  ❌ GPIO 告警驱动（阶段6）
```

## 下一步计划

- 进入阶段6：GPIO 告警字符设备驱动
