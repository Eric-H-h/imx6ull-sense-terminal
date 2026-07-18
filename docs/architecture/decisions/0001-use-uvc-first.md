# ADR-0001：MVP 使用 UVC-first 路线

## Status

Accepted

## Context

项目需要先完成摄像头到浏览器、运动事件和 systemd 的完整闭环。OV5640 DVP/CSI 可能引入排线、pinout、供电、时钟、设备树和 BSP 驱动风险。

## Considered Options

- USB UVC 摄像头。
- OV5640 DVP/并口 CSI 模组。

## Decision

MVP 首先使用 Linux 免驱 USB UVC 摄像头。OV5640 仅在 UVC MVP 闭环完成且剩余时间充足时作为增强项。

## Consequences

### Positive

- 先验证 V4L2、MJPEG、HTTP、motion 和 systemd 等用户态主线。
- 降低板级摄像头 bring-up 阻塞整个项目的概率。

### Negative

- UVC 摄像头隐藏了传感器 ISP 和一部分板级驱动工作。
- 后续 motion 输入格式可能需要额外取舍。

### Neutral / Follow-up

OV5640 若连续两个晚上仍无法建立基本采集链路，则停止增强项，不影响 MVP 发布。

## Verification

M1 已完成 UVC 枚举、格式确认和 MJPG 首帧采集，见 [M1 证据](../../verification/evidence/M1_uvc_capture.md)。
