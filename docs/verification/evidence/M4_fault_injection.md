# M4 验收证据：systemd 与故障注入

## 状态

Completed。systemd 正式安装、正常生命周期、`kill -9`、非法配置、事件日志写失败和 reboot 自启均已验证。阶段总结见 [M4 systemd 与故障注入](../../stage_summaries/M4_systemd_fault_injection.md)；PR #5 已合入 `develop`。

## 预检查期间已解决的板端阻塞

M4 预检查期间发现，板上额外启用的 `autowifi.service` 会自动拉起项目并不依赖的 AP6212 `wlan0`，进入不稳定的 DHD/SDIO 打开路径，导致串口和 SSH 用户态交互间歇失去响应。详细诊断见 [`M4-DRV-WIFI-001`](../../bug_reports/M4-DRV-WIFI-001_ap6212_dhd_sdio_hang.md)。

采用的项目级修复是保留 ConnMan 和厂商 unit 文件，只删除 `autowifi.service` 的持久 enable 链接；部署和验收继续使用 USB RNDIS。

正常重启且不带临时 U-Boot mask 后的关键证据：

```text
systemctl is-enabled autowifi.service -> disabled
systemctl is-active autowifi.service  -> inactive
systemctl is-active connman.service   -> active
systemctl is-active networking.service -> active
systemctl is-active ssh.service       -> active
wlan0                                 -> DOWN
启动日志中的 DHD 自动上电/扫描          -> 无
Windows USB RNDIS Adapter             -> Up, 192.168.7.1/30
WSL route                             -> dev eth5, src 192.168.7.1
board usb0                            -> UP, carrier=1, 192.168.7.2/30
USB RNDIS ping                        -> 4/4, 0% packet loss
SSH banner                            -> OpenSSH_7.9p1 Debian-10+deb10u2
SSH command                           -> success
```

第一次复核时 OTG 数据线未插入，`192.168.7.2` 被 Windows `xray_tun` 接管，产生了无真实 SSH banner 的 ping/TCP 假阳性；上述 RNDIS 证据均取自重新插线并确认专用路由后的有效复核。该 Wi-Fi 问题在项目范围内已解除阻塞；AP6212/DHD 的具体底层卡点保留为独立 BSP 深挖项，不纳入 M4 验收。

## D2-B 正式安装与正常生命周期验证

板端环境为 systemd 241。安装脚本将 ARM 程序、配置和 unit 安装到正式路径，但不会擅自启用或启动服务：

```text
/usr/local/bin/imx6ull-sense                 0755 root:root
/etc/imx6ull-sense/config.json               0644 root:root
/etc/systemd/system/imx6ull-sense.service    0644 root:root
/var/lib/imx6ull-sense                       0750 debian:debian
systemd-analyze verify                       exit 0
安装后状态                                   disabled / inactive
```

执行 `systemctl enable --now imx6ull-sense.service` 后：

```text
enabled / active
MainPID=1345
NRestarts=0
运行用户/组=debian:debian
journal: listening on 0.0.0.0:8080
```

启动时未连接摄像头，服务保持运行并通过 `/status` 报告 `health=degraded`、`camera_state=unavailable`。插入 UVC 摄像头后，原进程 PID 仍为 1345、`NRestarts=0`，日志出现 `capture active: /dev/video2 MJPG 640x480`，证明恢复由 daemon 自身完成，而不是依赖 systemd 重启。

恢复后的接口和数据验证：

```text
/status health/camera_state                  ok / active
采集 FPS                                     29.9
motion sample FPS                            3.00
GET /                                        HTTP 200, text/html
GET /stream                                  HTTP 200, boundary=frame
2 秒流数据                                   1,300,493 bytes, 61 boundaries
首帧 JPEG                                    SOI ff d8, EOI ff d9, valid
events.jsonl                                 1 line, 155 bytes, debian:debian
JSONL 严格逐行解析                           1/1 passed
```

向主进程发送 SIGTERM 后，systemd 记录 `Result=success`、`ExecMainStatus=0`，服务进入 inactive，`NRestarts=0`。这验证了正常退出不会触发 `Restart=on-failure`。随后手动启动成功，新 PID 为 1401；摄像头恢复 active，HTTP 状态约 29.8 FPS，原有 JSONL 行仍可解析。

## D3-A `kill -9` 崩溃恢复

崩溃前服务仍是 D2-B 手动拉起的进程：

```text
MainPID=1401
NRestarts=0
User/Group=debian:debian
/status health/camera_state                  ok / active
采集 FPS                                     30.0
uptime                                       11894.966
event_count（进程内）                         5
events.jsonl                                 6 lines, 947 bytes, debian:debian
JSONL 严格逐行解析                           6/6 passed
```

对 PID 1401 发送一次 `kill -9` 后，未执行 `systemctl start`。systemd 按 `Restart=on-failure` 和 `RestartSec=3` 自动拉起新进程：

```text
enabled / active
MainPID=1553
NRestarts=1
ActiveEnterTimestamp                         Fri 2026-07-31 18:02:15 CST
journal: listening on 0.0.0.0:8080
journal: capture active: /dev/video2 MJPG 640x480
```

恢复后的接口和数据验证（`curl --noproxy 192.168.7.2`）：

```text
/status health/camera_state                  ok / active
采集 FPS                                     29.9
motion sample FPS                            3.00
uptime                                       70.034
event_count（进程内）                         0
GET /                                        HTTP 200, text/html
GET /stream                                  HTTP 200, boundary=frame
2 秒流数据                                   1,416,438 bytes, 61 boundaries
首帧 JPEG                                    SOI ff d8, EOI ff d9, valid
events.jsonl                                 6 lines, 947 bytes, 未截断
JSONL 严格逐行解析                           6/6 passed
mtime                                        仍为 kill 前的 Jul 31 16:14
```

`/status` 的 `event_count` 从 5 变为 0，这是新进程内存计数器重置，不是日志被清空。JSONL 行内容与 kill 前一致。本次只杀一次主进程，没有出现 30 秒内连续重启触发 `StartLimitBurst=3` 的风暴。

主机侧没有从 kill 瞬间开始的 HTTP 掉线曲线：默认 `urllib` 走了 SOCKS 代理，不能作为恢复时延证据。恢复时延以 systemd 的 `RestartSec=3`、新 PID 出现、以及随后绕过代理的 `/status` 与码流为准。

## D3-B 非法配置 fail-safe

注入前服务仍是 D3-A 恢复后的进程：

```text
MainPID=1553
NRestarts=1
ActiveState=active
ExecMainStatus=0
config.json                                  http_port=8080
/status health/camera_state                  ok / active
```

将正式配置备份到 `/tmp/imx6ull-sense-config.json.d3b.bak` 后，把 `http_port` 写成 `0`（其余字段保持不变），再执行 `systemctl restart`。daemon 在启动阶段拒绝该配置并以 78 退出；unit 的 `RestartPreventExitStatus=78` 阻止自动重启。

```text
t=0s  / t=12s
ActiveState/SubState                         failed / failed
MainPID                                      0
Result                                       exit-code
ExecMainCode                                 1
ExecMainStatus                               78
NRestarts                                    0（12 秒内未增加）
journal                                      config error: http_port must be between 1 and 65535
systemd                                      Main process exited, code=exited, status=78/CONFIG
systemd                                      Failed with result 'exit-code'
```

`NRestarts` 从注入前的 1 变为 0，是 `systemctl restart` 开启新一轮后计数器归零，不是重启风暴。若 `RestartPreventExitStatus=78` 未生效，12 秒内会按 `RestartSec=3` 连续拉起并推高 `NRestarts`；实际保持 `failed`、`MainPID=0`。

恢复备份配置、`systemctl reset-failed` 并 `systemctl start` 后：

```text
config.json                                  http_port=8080，sha256 与注入前一致
MainPID=1686
ActiveState=active
NRestarts=0
ExecMainStatus=0
/status health/camera_state                  ok / active
采集 FPS                                     29.8
GET /                                        HTTP 200, text/html
GET /stream                                  HTTP 200, boundary=frame
2 秒流数据                                   1,398,238 bytes, 60 boundaries
首帧 JPEG                                    SOI ff d8, EOI ff d9, valid
events.jsonl                                 13 lines, 未因本次 fail-safe 被截断
```

## D3-C 事件日志写入失败

不重启服务，把运行目录中的 `events.jsonl` 换成指向 `/dev/full` 的符号链接，迫使下一次 motion 追加得到 `ENOSPC`。注入前：

```text
MainPID=1686
NRestarts=0
/status health/event_log_state               ok / ok
events.jsonl                                 普通文件, 13 lines, 2047 bytes
```

摄像头前挥手触发写入后，同一进程进入 degraded，HTTP 推流继续：

```text
MainPID=1686
NRestarts=0
ActiveState                                  active
/status health                               degraded
camera_state                                 active
event_log_state                              unavailable
采集 FPS                                     30.0
last_error                                   event log write failed: No space left on device
journal                                      motion pipeline: event log error: No space left on device
GET /                                        HTTP 200, text/html
GET /stream                                  HTTP 200, boundary=frame
2 秒流数据                                   1,424,656 bytes, 61 boundaries
首帧 JPEG                                    SOI ff d8, EOI ff d9, valid
```

随后删除符号链接并恢复备份文件，PID 仍为 1686。`event_log_state` 不会因路径恢复而自动变回 `ok`；下一次成功追加后：

```text
MainPID=1686
NRestarts=0
/status health/event_log_state               ok / ok
last_error                                   null
journal                                      motion event: sequence 41117 ... event_count 4
events.jsonl                                 普通文件, 15 lines, 2363 bytes
```

JSONL 从 13 行增至 15 行，说明恢复后的文件可继续追加，注入期间失败的写入没有截断原日志。

## D3-D reboot 自启与 JSONL 持久化

reboot 前：

```text
enabled / active
MainPID=1686
NRestarts=0
autowifi.service                             disabled / inactive
events.jsonl                                 15 lines, sha256 4bfd236b...
who                                          （有 SSH 会话）
```

执行 `sudo reboot`。板端再次可达后，无人登录（`who` 为空），服务已由 systemd 拉起：

```text
enabled / active
MainPID=432
NRestarts=0
ActiveEnterTimestampMonotonic                23002229
journal -b                                   Started i.MX6ULL Sense Terminal
journal                                      listening on 0.0.0.0:8080
journal                                      capture active: /dev/video1 MJPG 640x480
autowifi.service                             disabled / inactive
```

JSONL 前 15 行 sha256 仍为 `4bfd236b...`，与 reboot 前整文件一致。新进程追加了第 16 行（`sequence=12`），文件变为 16 lines / 2518 bytes。

恢复后的接口：

```text
/status health/camera_state                  ok / active
device                                       /dev/video1
采集 FPS                                     29.8
event_log_state                              ok
GET /                                        HTTP 200, text/html
GET /stream                                  HTTP 200, boundary=frame
2 秒流数据                                   1,386,006 bytes, 60 boundaries
首帧 JPEG                                    SOI ff d8, EOI ff d9, valid
```

UVC 节点从 reboot 前的 `/dev/video2` 变为 `/dev/video1`；auto selector 选到了正确的 Capture 节点。`who` 为空证明这次启动不依赖登录后的手动 `systemctl start`。

## 计划验收

| 场景 | 预期 | 结果 |
| --- | --- | --- |
| `kill -9` | systemd 按策略恢复服务 | Pass（D3-A） |
| reboot | 服务自动启动 | Pass（D3-D） |
| camera missing | 服务可诊断地 degraded，且不重启 | Pass（D2-B） |
| camera restored | 同一进程恢复采集 | Pass（D2-B） |
| SIGTERM | 正常退出 0，不触发失败重启 | Pass（D2-B） |
| bad config | 明确日志并 fail safe | Pass（D3-B） |
| event log write failure | 推流继续，degraded，恢复后可写 | Pass（D3-C） |
| `journalctl` | 可追踪启动、degraded、恢复和退出 | Pass（D2-B） |

M4 实现后补充 service 安装命令、unit 状态、日志、重启时间和关闭条件。正式运行中产生实际影响的事故进入 [`operations/postmortems/`](../../operations/postmortems/)，开发调试问题仍进入 [`bug_reports/`](../../bug_reports/)。
