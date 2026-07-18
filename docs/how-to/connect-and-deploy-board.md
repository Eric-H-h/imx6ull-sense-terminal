# 连接和部署开发板

## 默认链路

项目默认通过 USB RNDIS 访问开发板：

```text
debian@192.168.7.2
```

必须连接开发板 USB OTG 接口，并使用支持数据传输的 USB 线。Windows 侧设备需要绑定 `USB RNDIS Adapter` 或等价 Remote NDIS 驱动。

## 验证连接

在 WSL 中执行：

```sh
ping -c 4 192.168.7.2
ssh debian@192.168.7.2 'uname -a; ip addr show usb0'
```

预期结果：

- ping 无持续丢包。
- `usb0` 包含 `LOWER_UP`。
- 板端地址为 `192.168.7.2/30`。

## 部署当前程序

```sh
cd /home/eric/projects/imx6ull-sense-terminal
scp app/daemon/imx6ull-sense debian@192.168.7.2:/tmp/
scp config/config.json debian@192.168.7.2:/tmp/
```

板端运行：

```sh
ssh debian@192.168.7.2 '/tmp/imx6ull-sense -c /tmp/config.json'
```

## 连接失败时

按以下顺序排查：

1. USB OTG 接口。
2. 数据线是否支持数据传输。
3. Windows RNDIS 驱动绑定。
4. 板端 `usb0` carrier 和 `LOWER_UP`。
5. IP、ping、SSH、SCP。
6. 最后才检查应用二进制和参数。

完整复盘见 [M0-NET-WIFI-001](../bug_reports/M0-NET-WIFI-001_usb_rndis_board_link.md)。
