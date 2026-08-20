# MVP 综合测试报告

## 状态

In Progress。M0-M4 板端验收已完成；M5 汇总最终结论，当前不填写推测数值。

## 硬件与环境

| 项目 | 当前事实 |
| --- | --- |
| 开发板 | EBF6ULL S1 Pro eMMC / i.MX6ULL |
| 摄像头路线 | USB UVC first |
| 主机 | Ubuntu on WSL2 |
| 板端连接 | USB RNDIS `192.168.7.2` |
| 当前阶段 | M4 验收完成，等待提交 |

## 功能结果

| 能力 | 结果 | 证据 |
| --- | --- | --- |
| M0 环境和板端基线 | Pass | [M0 summary](../stage_summaries/M0_environment_and_board_baseline.md) |
| M1 UVC 枚举和首帧 | Pass | [M1 summary](../stage_summaries/M1_usb_uvc_camera_capture.md) |
| M2 MJPEG 浏览器流 | Pass | [M2 summary](../stage_summaries/M2_mjpeg_browser_stream.md) |
| M3 motion event | Pass | [M3 summary](../stage_summaries/M3_motion_event_logging.md) |
| M4 systemd/fault injection | Pass | [M4 summary](../stage_summaries/M4_systemd_fault_injection.md) |

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

M4 已验收。结论见 [M4 evidence](evidence/M4_fault_injection.md)。M5 再汇总到本报告的最终表。

## 已知限制

- 当前 MVP 只面向可信局域网，不提供认证或 TLS。
- OV5640、录像、H.264/RTSP 和 AI 检测不属于 MVP 必选范围。
