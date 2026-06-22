<!--
 * @Author: uestcchuan2002 1992735052@qq.com
 * @Date: 2026-06-22 21:23:43
 * @LastEditors: uestcchuan2002 1992735052@qq.com
 * @LastEditTime: 2026-06-22 21:23:44
 * @FilePath: /03_water_quality_gateway_project/docs/test-log.md
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
-->
# 项目调试记录


## 2026-06-22 开发板启动与串口终端验证

### 1. 启动方式

*U-Boot启动后，终端会输出下列内容：*

- 开发板型号：I.MX6U ALPHA|MINI
- CPU：Freescale i.MX6ULL rev1.1
- DRAM：512 MiB
- U-Boot 版本：U-Boot 2016.03，编译时间 Aug 22 2024 11:37:22 +0800
- 启动方式：Normal Boot
- 控制台输入/输出：serial
- 启动介质：mmc1(part 0)，后续需确认是 SD 卡还是 eMMC
- Kernel 镜像：zImage
- Kernel 镜像大小：6785480 bytes
- 设备树文件：imx6ull-14x14-emmc-7-1024x600-c.dtb
- 网络状态：FEC1 未设置 MAC 地址，暂不影响串口启动验证
- 备注：U-Boot 提示 `bad CRC, using default environment`，说明当前使用默认环境变量，后续可再处理

### 2. Linux 系统版本记录

`uname` 用于查看系统内核、硬件平台信息，`-a = all`，打印全部信息，一次性输出完整系统标识

```ch
uname -a
Linux ATK-IMX6U 4.1.15-g3dc0a4b #1 SMP PREEMPT Thu Aug 18 09:27:40 CST 2022 armv7l armv7l armv7l GNU/Linux
```

- 主机名：ATK-IMX6U
- Linux Kernel 版本：4.1.15-g3dc0a4b
- 编译序号：#1 内核编译次数
- 内核特性：SMP PREEMPT  支持多核cpu 抢占式内核
- 编译时间：Thu Aug 18 09:27:40 CST 2022
- CPU 架构：armv7l
- 系统类型：GNU/Linux

### 3. 登录账户与权限

`whoami` 用于打印当前会话登录的用户名，嵌入式开发中常用来检查是否拥有 root 权限，执行驱动、设备文件操作前校验权限
`id` 用于打印当前用户身份信息

```sh
whoami
id

root
uid=0(root) gid=0(root) groups=0(root)
```

- 当前用户：root
- UID：0
- GID：0
- 登录方式：串口终端进入 root shell
- 说明：当前开发镜像通过串口终端可直接进入 root 环境，未观察到用户名/密码登录流程

>uid=0 代表当前登录用户是 root 超级管理员，拥有系统最高权限；gid=0 表示默认所属 root 用户组，groups 展示全部附属组；嵌入式操作驱动、设备文件、内核模块都需要 UID=0 的 root 权限，普通用户 UID 不为 0 会触发权限不足报错

#### SSH 连接

通过win终端来对Linux开发板进行控制

```sh
ssh -oHostKeyAlgorithms=+ssh-rsa -oPubkeyAcceptedAlgorithms=+ssh-rsa root@192.168.2.201
```

#### 串口终端

通过串口来对Linux开发板进行控制

波特率:115200

### 4. 今日结论

开发板已能通过串口终端正常输出 U-Boot 和 Linux 启动信息，并成功进入 Linux root shell。系统根文件系统挂载在 `/dev/mmcblk1p2`，类型为 ext3，串口控制台为 `ttymxc0,115200`。