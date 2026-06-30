<!--
 * @Author: uestcchuan2002 1992735052@qq.com
 * @Date: 2026-06-30 15:02:35
 * @LastEditors: uestcchuan2002 1992735052@qq.com
 * @LastEditTime: 2026-06-30 15:12:22
 * @FilePath: /03_water_quality_gateway_project/driver/water_alarm/README.md
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
-->
# water_alarm 使用说明

## 1.环境搭建与驱动安装

参考调试日志(test-log.md)中的阶段6 GPIO字符设备驱动小结

最终实现在开发板上实现 `/dev/newchrled` 设备节点

## 2.使用案例

### 2.1 编译应用程序

```bush
arm-linux-gnueabihf-gcc ledApp.c -o ledApp
```

### 2.2 将应用程序放置在开发板中

```bush
scp -O -o HostKeyAlgorithms=+ssh-rsa ./ledApp root@192.168.2.201: /home/root/ledApp
```

### 2.3 启动应用程序

```bush
./ledApp /dev/newchrled 0 #关闭led
./ledApp /dev/newchrled 1 #开启led
```

