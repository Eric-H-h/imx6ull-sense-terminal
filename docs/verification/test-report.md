# MVP 综合测试报告

## 状态

In Progress。M5 在 M0-M4 验收完成后汇总最终结论；当前不填写推测数值。

## 硬件与环境

| 项目 | 当前事实 |
| --- | --- |
| 开发板 | EBF6ULL S1 Pro eMMC / i.MX6ULL |
| 摄像头路线 | USB UVC first |
| 主机 | Ubuntu on WSL2 |
| 板端连接 | USB RNDIS `192.168.7.2` |
| 当前阶段 | M2 验收中 |

## 功能结果

| 能力 | 结果 | 证据 |
| --- | --- | --- |
| M0 环境和板端基线 | Pass | [M0 summary](../stage_summaries/M0_environment_and_board_baseline.md) |
| M1 UVC 枚举和首帧 | Pass | [M1 summary](../stage_summaries/M1_usb_uvc_camera_capture.md) |
| M2 MJPEG 浏览器流 | Pending | [M2 evidence](evidence/M2_mjpeg_stream.md) |
| M3 motion event | Pending | [M3 evidence](evidence/M3_motion_event.md) |
| M4 systemd/fault injection | Pending | [M4 evidence](evidence/M4_fault_injection.md) |

## 性能

| 指标 | 最终值 | 环境与说明 |
| --- | ---: | --- |
| 分辨率 | Pending | |
| FPS | Pending | |
| CPU | Pending | |
| RSS | Pending | |
| 单客户端持续时间 | Pending | |
| motion 延迟 | Pending | |

## 故障注入

Pending。M4 完成后汇总 kill、reboot、camera missing 和 bad config 的结论并链接原始 evidence。

## 已知限制

- 当前 MVP 只面向可信局域网，不提供认证或 TLS。
- OV5640、录像、H.264/RTSP 和 AI 检测不属于 MVP 必选范围。
