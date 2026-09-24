# WZ-724 MAVLink链路

数据链路为：

`Pixhawk串口/USB -> 香橙派socat透明桥 -> UDP 15001 -> 笔记本UDP 5555 -> wz-724 MAVLink解析器`

这里的UDP只是MAVLink传输层，姿态、深度、速度、PWM反馈和控制命令仍然都是MAVLink消息。

当前香橙派已经有 `/etc/init.d/S100_mavlink-bridge` 在运行，并已实测通过
`/dev/ttyS0 @ 57600` 收到 Pixhawk（system id 1）的真实 MAVLink 心跳。
不要同时启动本目录中的 `S94wz-mavlink`，否则两个服务会争用串口和 UDP 15001。
下面的部署步骤仅用于原服务丢失、损坏或更换系统后的恢复。

部署到香橙派：

```sh
cp S94wz-mavlink /etc/init.d/S94wz-mavlink
cp wz-mavlink.conf /etc/wz-mavlink.conf
chmod 755 /etc/init.d/S94wz-mavlink
/etc/init.d/S94wz-mavlink start
```

验证：

```sh
/etc/init.d/S94wz-mavlink status
tail -f /var/log/wz-mavlink.log
```

本机当前配置是 `/dev/ttyS0 @ 57600`。如果以后改为 USB 或其他 GPIO UART，
再把 `/etc/wz-mavlink.conf` 中的 `MAVLINK_SERIAL_DEVICE` 和
`MAVLINK_BAUD` 改成对应 SERIAL 端口参数。
