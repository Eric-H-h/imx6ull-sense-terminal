# 三分钟 Demo 脚本

本脚本面向已经安装 `imx6ull-sense.service` 的 EBF6ULL S1 Pro。地址默认 `http://192.168.7.2:8080/`。所有结论必须能当场复现，不要口述未经验证的数字。

操作前：USB OTG 已连接，Windows RNDIS 为 `192.168.7.1/30`，`autowifi.service` 保持 disabled。观众挥手由操作者完成。

## 时间轴

| 时间 | 画面 | 要说的话 | 操作 |
| --- | --- | --- | --- |
| 0:00-0:25 | 架构一页 | 这是跑在 i.MX6ULL 上的最小感知服务：UVC 采集、浏览器 MJPEG、可解释 motion、JSONL、systemd。不是套成熟 streamer。 | 打开 [架构总览](../architecture/overview.md) 或 README 图。 |
| 0:25-0:50 | 浏览器画面 | 板端正式路径监听 8080。摄像头是动态选的 UVC Capture 节点，MJPEG 640x480@30。 | 打开 `http://192.168.7.2:8080/`。刷新一次，证明重连不掉服务。 |
| 0:50-1:20 | `/status` | `/status` 是真实状态，不是占位。health、camera、motion score、event_count 都能对上画面。 | `curl --noproxy 192.168.7.2 http://192.168.7.2:8080/status` |
| 1:20-1:55 | 挥手出事件 | motion 按 3 FPS 抽样、160x120 灰度、5% 变化比例、1500 ms cooldown。挥手产生事件，静止不应刷日志。 | 对摄像头挥手一次；再 `curl` `/status`，看 `event_count` 增加。 |
| 1:55-2:20 | JSONL | 事件落在 `/var/lib/imx6ull-sense/events.jsonl`。`event_count` 是进程内计数，重启归零；文件继续追加。 | `ssh -n debian@192.168.7.2 'tail -n 1 /var/lib/imx6ull-sense/events.jsonl'` |
| 2:20-2:50 | 故障策略 | 摄像头没了服务还在，报 degraded。进程被杀掉 systemd 会拉起。配错了退出码 78，不会重启风暴。 | 口头对照 [测试报告故障表](../verification/test-report.md)；不要在正式演示里现场 `kill -9`，除非预留恢复时间。 |
| 2:50-3:00 | 边界 | 可信局域网，无认证。Wi-Fi 不做部署路径。OV5640 和 H.264 不在 MVP。 | 收束到 README 已知限制。 |

## 备用 60 秒版本

若只有一分钟：浏览器画面（15 s）→ 挥手 + `/status`（20 s）→ JSONL 一行（15 s）→ systemd 与 degraded 一句话（10 s）。

## 演示前检查

```sh
ping -c 2 192.168.7.2
ssh -n debian@192.168.7.2 'systemctl is-active imx6ull-sense.service; systemctl is-enabled autowifi.service'
curl --noproxy 192.168.7.2 http://192.168.7.2:8080/status
```

预期：服务 `active`，`autowifi` 为 `disabled`，`/status` 在有摄像头时 `health=ok`、`camera_state=active`。

## 不要做的事

- 不要启用 `autowifi.service`。
- 不要把 `/dev/videoX` 编号说成固定设备。
- 不要在没有 `--noproxy` 时用会被 SOCKS 接管的 curl。
- 不要提交演示视频或原始抓帧到仓库。
