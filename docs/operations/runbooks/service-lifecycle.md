# 服务生命周期

面向已安装的 `imx6ull-sense.service`。默认板端地址 `debian@192.168.7.2`，HTTP `http://192.168.7.2:8080`。验收命令见 [M4 evidence](../../verification/evidence/M4_fault_injection.md)。

## 前置条件

- USB OTG 数据线已连接，Windows RNDIS 为 `192.168.7.1/30`。
- `autowifi.service` 保持 disabled。不要用板载 Wi-Fi 作为部署路径。
- 交叉编译产物：`app/daemon/build/arm/imx6ull-sense`。
- curl 访问板端时使用 `--noproxy 192.168.7.2`。
- 不需要远端 stdin 的 SSH 使用 `ssh -n`。

## 安装（不启动）

在仓库根目录打包并拷到板上后，以 root 执行安装脚本。脚本会安装二进制、配置和 unit，并 `daemon-reload`，**不会** enable 或 start。

```sh
sudo ./install-service.sh
```

预期：

```text
/usr/local/bin/imx6ull-sense                 0755 root:root
/etc/imx6ull-sense/config.json               0644 root:root
/etc/systemd/system/imx6ull-sense.service    0644 root:root
/var/lib/imx6ull-sense                       0750 debian:debian
```

已存在的 `/etc/imx6ull-sense/config.json` 默认保留。需要覆盖时设置 `OVERWRITE_CONFIG=1`。

## 启用并启动

```sh
sudo systemctl enable --now imx6ull-sense.service
systemctl is-enabled imx6ull-sense.service
systemctl is-active imx6ull-sense.service
curl --noproxy 192.168.7.2 http://192.168.7.2:8080/status
```

预期：`enabled` / `active`，`/status` 在有摄像头时 `health=ok`，无摄像头时 `health=degraded` 且服务仍在运行。

## 查看状态和日志

```sh
systemctl status imx6ull-sense.service --no-pager -l
journalctl -u imx6ull-sense.service -n 50 --no-pager
ls -l /var/lib/imx6ull-sense/events.jsonl
```

## 摄像头拔插

服务应保持同一 MainPID。拔出后 `/status` 为 `camera_state=unavailable`；插回后同进程恢复采集，`NRestarts` 不增加。不要把 `/dev/videoX` 编号当成稳定身份。

## 正常停止与崩溃恢复

- 正常停止：`sudo systemctl stop imx6ull-sense.service`。SIGTERM 应得到退出码 0，不应自动重启。
- 崩溃恢复：主进程被 `kill -9` 后，systemd 按 `Restart=on-failure` 在约 3 秒后拉起新 PID。
- 非法配置：`http_port` 等校验失败时进程以 78 退出，服务保持 failed，不重启风暴。恢复备份配置后 `systemctl reset-failed` 再 `start`。

## 失败分支

| 现象 | 先查 |
| --- | --- |
| ping 通但 SSH/HTTP 无 banner | OTG 线、RNDIS 网卡、是否被其他 TUN 抢走 `192.168.7.2` |
| 串口或 SSH 卡死 | `autowifi` 是否被重新 enable；见 [M4-DRV-WIFI-001](../../bug_reports/M4-DRV-WIFI-001_ap6212_dhd_sdio_hang.md) |
| 服务 failed，退出码 78 | 配置 JSON；不要当成崩溃反复 start |
| `health=degraded` 且 `event_log_state=unavailable` | `/var/lib/imx6ull-sense/events.jsonl` 是否可写；恢复路径后需一次成功 motion 写入才会回到 `ok` |

## 回滚

```sh
sudo systemctl disable --now imx6ull-sense.service
```

二进制和配置文件仍留在正式路径，直到手动删除。事件日志默认保留。
