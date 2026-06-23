# Water Quality Linux Gateway

## 项目目标

基于 i.MX6ULL 实现工业水质监测 Linux 网关，支持串口/RS485 数据采集、SQLite 缓存、TCP/MQTT 上传和本地告警。

## 当前进度

- 2026-06-22：完成开发板 Linux 启动验证
- 2026-06-22：完成串口终端通信验证
- 2026-06-22：完成 SSH 通信验证
- 2026-06-22：确认当前用户为 root
- 2026-06-22：确认 Kernel 版本为 `4.1.15-g3dc0a4b`
- 2026-06-22：完成第一版最小 demo 运行
- 2026-06-23：完成应用层最小 Demo 框架
- 2026-06-23：完成 `Makefile`，支持自动编译 `src/*.c`
- 2026-06-23：完成 `config.c/config.h`，支持读取 `config/gateway.conf`
- 2026-06-23：完成 `logger.c/logger.h`，支持带时间戳的日志输出
- 2026-06-23：完成 `sample.c/sample.h`，支持模拟水质数据生成与格式化输出
- 2026-06-23：完成主程序周期采样测试，可按 `sample_period_ms` 输出模拟水质数据

## 当前最小 Demo 功能

当前版本已经支持：

- 读取设备配置：`device_id`、`sample_period_ms`、`log_level`、`serial_device`、`baudrate`
- 初始化日志模块，并按照配置中的日志等级输出运行状态
- 生成模拟水质数据：pH、温度、浊度、电导率、传感器状态位、告警状态位、采样序号
- 按配置周期输出模拟采样结果

## 目录结构

```text
03_water_quality_gateway_project/
  app/
    Makefile
    include/
      config.h
      logger.h
      sample.h
    src/
      main.c
      config.c
      logger.c
      sample.c
  config/
    gateway.conf
  docs/
    test-log.md
  driver/
  scripts/
```

## 编译和运行

进入应用目录：

```sh
cd app
make clean
make
./water_gateway
```

如果当前环境没有 `make`，可以临时使用 `gcc` 直接验证：

```sh
gcc -std=gnu99 -Iinclude src/main.c src/config.c src/logger.c src/sample.c -o water_gateway_test
./water_gateway_test
```

## 配置文件

配置文件路径：

```text
config/gateway.conf
```

当前配置示例：

```ini
device_id=water_gateway_001
sample_period_ms=1000
log_level=info
serial_device=/dev/ttyUSB0
baudrate=9600
```

## 下一步计划

下一阶段开始实现串口模块：

- 编写 `serial_port.h`
- 编写 `serial_port.c`
- 封装串口打开、波特率设置、8N1 配置、读写接口和超时处理
- 为后续 Modbus RTU 主站模块做准备
