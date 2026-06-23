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
