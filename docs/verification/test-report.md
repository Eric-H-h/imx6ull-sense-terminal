# MVP 综合测试报告

## 状态

Completed。本报告只汇总 M0-M4 已记录的板端事实，不填写推测数值。M5 不新增 daemon 能力，也不补做未发生过的测试。

| 项目 | 值 |
| --- | --- |
| 报告日期 | 2026-08-20 |
| 功能闭环合入 | `develop` @ `bee388a`（PR #5） |

## 硬件与环境

| 项目 | 当前事实 | 来源 |
| --- | --- | --- |
| 开发板 | EBF6ULL S1 Pro eMMC / i.MX6ULL | [M0](../stage_summaries/M0_environment_and_board_baseline.md) |
| 内核 / init | Linux 4.19.35-imx6，systemd 241 | [M4](evidence/M4_fault_injection.md) |
| 摄像头路线 | USB UVC first；动态选择 `uvcvideo` Capture 节点 | [ADR-0001](../architecture/decisions/0001-use-uvc-first.md) |
| 视频格式 | MJPG 640x480@30 | [M1](evidence/M1_uvc_capture.md)、[M2](evidence/M2_mjpeg_stream.md) |
| 主机 | Ubuntu on WSL2 | [M0](evidence/M0_bringup.md) |
| 板端连接 | USB RNDIS `debian@192.168.7.2` | [M0-NET-WIFI-001](../bug_reports/M0-NET-WIFI-001_usb_rndis_board_link.md) |
| 正式安装 | `/usr/local/bin/imx6ull-sense`，`debian:debian` | [M4](evidence/M4_fault_injection.md) |

节点号不是稳定身份。M3 验收使用 `/dev/video1`；M4 安装验收为 `/dev/video2`，reboot 后变为 `/dev/video1`。

## 功能结果

| 能力 | 结果 | 证据 |
| --- | --- | --- |
| M0 环境和板端基线 | Pass | [M0 summary](../stage_summaries/M0_environment_and_board_baseline.md) |
| M1 UVC 枚举和首帧 | Pass | [M1 summary](../stage_summaries/M1_usb_uvc_camera_capture.md) |
| M2 MJPEG 浏览器流 | Pass | [M2 summary](../stage_summaries/M2_mjpeg_browser_stream.md) |
| M3 motion event | Pass | [M3 summary](../stage_summaries/M3_motion_event_logging.md) |
| M4 systemd/fault injection | Pass | [M4 summary](../stage_summaries/M4_systemd_fault_injection.md) |
| M5 测试报告与演示材料 | Pass | [M5 summary](../stage_summaries/M5_resume_demo_packaging.md) |

接口：`GET /`、`GET /stream`、`GET /status`。当前服务无认证、无 TLS。

## 性能

全栈数字取自 M3 单客户端 30 分钟稳定性（UVC MJPEG + 浏览器 stream + 3 FPS motion）。M2 是尚未接入 motion 的推流基线，单独列出。

| 指标 | 最终值 | 环境与说明 |
| --- | --- | --- |
| 分辨率 | 640x480 | UVC MJPEG 协商结果 |
| 推流 FPS | 30.0 | M3 30 分钟全程 30.0；M2 并行验证 29.9-30.0 |
| motion 抽样 | 3.0 FPS | 160x120 grayscale，libjpeg `1/4` |
| CPU | 16.3%-16.6% | M3 全栈 30 分钟；进程累计平均 |
| RSS | 2968 KB，delta 0 | 同一 PID，7 个采样点 |
| 单客户端持续时间 | 30 分钟（M3 全栈）；约 41 分 30 秒（M2 仅推流） | M2 RSS 2996→3000 kB |
| JPEG 解码 | 平均 18.554 ms / 最大 20.910 ms | 独立 3 FPS benchmark，30 秒 |
| motion 检测延迟 | 未单独探针 | 受 3 FPS 约束，最坏约一个采样周期（~333 ms）加解码（≤21 ms） |

M2 仅推流、无客户端快照时 CPU 为 1.9%→6.7%。独立 3 FPS 解码 benchmark 约 5.1%-5.4% CPU。5 FPS 解码约 8.0%-8.8% CPU，保留为可调上限，不是默认值。

## 故障注入

| 场景 | 结果 | 关键事实 |
| --- | --- | --- |
| 启动无摄像头 | Pass | 服务保持运行，`health=degraded`，`camera_state=unavailable` |
| 插回摄像头 | Pass | 同一 PID 恢复采集，`NRestarts=0` |
| SIGTERM | Pass | `Result=success`，不误触发重启 |
| `kill -9` | Pass | D3-A：PID 1401→1553，`NRestarts=1` |
| 非法配置 | Pass | D3-B：`http_port=0`，退出码 78，12 秒无重启风暴 |
| 事件日志写失败 | Pass | D3-C：`/dev/full`，推流继续，`event_log_state=unavailable` |
| 写路径恢复 | Pass | 同一 PID；下一次成功追加后 `event_log_state=ok` |
| reboot 自启 | Pass | D3-D：`who` 为空即已运行；前 15 行 JSONL sha256 不变并继续追加 |

完整命令见 [M4 evidence](evidence/M4_fault_injection.md)。`event_count` 是进程内计数，重启后归零；JSONL 在磁盘上继续追加。这是已确认语义。

## Motion 行为

| 场景 | 结果 | 来源 |
| --- | --- | --- |
| 静止误报 | 5 分钟 `static_delta=0` | [M3](evidence/M3_motion_event.md) |
| 明显挥手 | 10/10 | 同上 |
| 持续运动 | 7 个 cooldown 受限事件 | 默认 1500 ms |
| 摄像头恢复 | 第一帧只重建 baseline | 恢复后约 3.66 s 才出现下一事件 |
| JSONL | 13/13 行可独立解析 | M3 验收路径 `/tmp/...`；正式路径 `/var/lib/imx6ull-sense/events.jsonl` |

默认参数：像素差 25，变化比例 5%，cooldown 1500 ms。阈值针对当前摄像头和室内场景，换设备后需要重新校准。

## 已知限制

- 当前 MVP 只面向可信局域网，不提供认证或 TLS。
- 只验证了单客户端稳定性，没有多客户端压力测试。
- JSONL 追加不 `fsync`；断电时最后一行持久性不保证。
- `event_log_state` 在写失败后保持 unavailable，直到下一次成功追加。
- 板端 RTC 不可靠，验收以 monotonic、PID 和文件 sha256 为准。
- `/dev/videoX` 编号会随枚举变化；必须按 driver 和 Device Caps 选择节点。
- 不要启用 `autowifi.service`。AP6212 DHD/SDIO 见 [M4-DRV-WIFI-001](../bug_reports/M4-DRV-WIFI-001_ap6212_dhd_sdio_hang.md)。
- OV5640、录像、H.264/RTSP 和 AI 检测不属于 MVP。
- `jpeg_quality` 预留给软件 JPEG 编码；当前 MJPEG pass-through 不使用。

## Fallback

- UVC 格式或 BSP 不兼容时，换另一款 Linux 免驱 UVC 摄像头。
- MJPEG 性能不足时按顺序降低分辨率、FPS 和 JPEG 质量。
- 板端网络失败时按 OTG 线、Windows RNDIS、`usb0` carrier、IP/SSH 的顺序排查；curl 使用 `--noproxy 192.168.7.2`。
