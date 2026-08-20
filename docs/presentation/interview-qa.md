# Interview Q&A

答案只陈述仓库里已经验收的事实。数字必须能指回 [测试报告](../verification/test-report.md) 或对应 evidence。

## Q1: 为什么不直接用 uStreamer 或 mjpg-streamer？

目标不是交付生产级 streamer，而是自己走通并验证嵌入式 Linux 主路径：V4L2 采集、HTTP 推流、事件检测和 systemd 可靠性。开源项目用作架构参考，实现保持小到能讲清楚、能在板上 debug。见 [ADR-0003](../architecture/decisions/0003-build-minimal-daemon.md)。

## Q2: 为什么用 MJPEG 而不是 H.264/RTSP？

i.MX6ULL 没有硬件 H.264 编码器，软件 H.264 对主线过重。MJPEG-over-HTTP 协议简单、浏览器可直接打开，适合局域网演示。代价是带宽更高。见 [ADR-0002](../architecture/decisions/0002-use-mjpeg-over-http.md)。

## Q3: 这个项目最有工程含量的点是什么？

不只是出画面。它是板上验证过的设备服务：真实 `/status`、JSONL 事件、systemd 崩溃恢复、配置错误 fail-safe，以及可复查的故障注入证据。

## Q4: 从开源项目借了什么？

uStreamer 提供轻量 MJPEG daemon 形态，mjpg-streamer 提供 input/output 分离思路，Motion 提供事件状态和 cooldown，v4l-utils 提供摄像头核实路径。产品主线仍是本仓库的最小 daemon。

## Q5: 自己实现了哪些部分？

UVC 节点发现、V4L2 MMAP 采集、共享最新 JPEG、HTTP `/` `/stream` `/status`、libjpeg 灰度抽样、帧差 motion、JSONL、systemd unit、安装脚本，以及板端验收和失败模式文档。

## Q6: 为什么先做 USB UVC，而不是板上 OV5640？

OV5640 依赖设备树、时钟和 CSI/DVP，两个晚上不通就应切免驱 UVC，避免硬件 bring-up 吞掉 MVP。UVC 先把采集、推流、事件和服务闭环做完；OV5640 是增强项。见 [ADR-0001](../architecture/decisions/0001-use-uvc-first.md)。

## Q7: 为什么 motion 是 3 FPS、160x120，而不是每帧全分辨率检测？

主链路是 MJPEG pass-through。全分辨率逐帧解码会和推流抢 CPU。板上 benchmark：3 FPS 独立解码约 5% CPU、平均 18.6 ms；与推流并行时画面仍保持 30 FPS。5 FPS 约 8% CPU，只作为可调上限。见 [M3 evidence](../verification/evidence/M3_motion_event.md)。

## Q8: 摄像头拔掉为什么不靠 systemd 重启进程？

摄像头是可恢复的运行时条件，不是进程崩溃。daemon 进入 degraded，HTTP 继续提供 `/status`，采集线程重试。进程被 `kill -9` 才由 `Restart=on-failure` 拉起。配置错误用退出码 78 和 `RestartPreventExitStatus` 避免重启风暴。见 [ADR-0004](../architecture/decisions/0004-camera-failure-degraded-mode.md)。

## Q9: `/status` 的 `event_count` 重启后为什么变 0？日志是不是丢了？

`event_count` 是本次进程内存计数。JSONL 在 `/var/lib/imx6ull-sense/events.jsonl` 继续追加。M4 reboot 验收中，重启后前 15 行 sha256 不变，随后继续写新行。这是明确语义，不是缺陷。

## Q10: 板端为什么不用 Wi-Fi 部署？

默认路径是 USB RNDIS `192.168.7.2`。额外启用 `autowifi.service` 会拉起 AP6212 DHD/SDIO，用户态 SSH/串口可能失去响应。项目级处理是保持该服务 disabled，不把底层 BSP 根因纳入 MVP。见 [M4-DRV-WIFI-001](../bug_reports/M4-DRV-WIFI-001_ap6212_dhd_sdio_hang.md)。

## 追问（可选用）

- 为什么查询 PXP 节点会 Oops，设备发现如何预筛？见 [M1-DRV-PXP-001](../bug_reports/M1-DRV-PXP-001_pxp_query_kernel_oops.md)。
- 为什么现在还不抽象 CameraSource？第二个真实 adapter 出现后再抽 seam。见 [ADR-0006](../architecture/decisions/0006-defer-camera-source-seam.md)。
- WSL 访问板端 HTTP 为什么会超时？代理/TUN 可能接管 `192.168.7.2`，curl 必须 `--noproxy`。见 [M2-NET-PROXY-001](../bug_reports/M2-NET-PROXY-001_wsl_proxy_bypasses_rndis.md)。
