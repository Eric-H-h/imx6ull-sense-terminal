# ADR-0006：推迟正式 CameraSource seam，保留通用帧模型

## Status

Accepted

## Context

当前 M2 只有一个真实输入实现：USB UVC 摄像头输出 MJPEG。未来可能接入 UVC YUYV 或 OV5640 DVP/CSI，它们仍通过 V4L2 进入用户态，但设备发现、driver、格式协商和帧处理路径可能不同。

如果现在直接把 `uvcvideo + MJPEG` 定义成系统唯一能力，后续扩展会把设备差异传播到 HTTP 和 motion 调用方。反过来，如果只有一个真实 adapter 时就引入插件 ABI 和大接口，也会得到一个没有实际 leverage 的浅 module。

## Considered Options

- 永久把 `uvcvideo + MJPEG` 写成唯一输入能力。
- 在 M2 立即实现复杂 CameraSource 插件 ABI。
- M2 保持 UVC/MJPEG 专用实现，同时记录未来 seam 的最小 interface 和触发条件。

## Decision

M2 继续以 UVC MJPEG 为唯一开发和验收主线，不等待 OV5640，也不为未到货硬件提前重构代码。

当前只定义未来 interface 必须隐藏的知识：

- 设备发现和格式协商策略。
- V4L2 buffer 生命周期和错误映射。
- 具体 driver、设备节点和像素格式差异。

统一帧描述至少携带：

- `pixel_format`
- `width`
- `height`
- `stride` 或有效的 `bytesused`
- `sequence`
- `timestamp`

当出现第二个真实 adapter，例如 YUYV 输入或 OV5640，才提取正式 CameraSource seam。interface 应保持小而深，不引入动态插件 ABI。

计划数据路径：

```text
UVC MJPEG
  -> JPEG pass-through
  -> latest JPEG
  -> HTTP stream

UVC/OV5640 YUYV
  -> Y data for motion
  -> JPEG encoder
  -> latest JPEG
  -> HTTP stream
```

如果 UVC 主线继续采用 MJPEG，M3 对 motion 先实测低频 JPEG 解码和灰度提取，再决定是否切换 YUYV。M2 不提前硬编码该结论。

OV5640 的设备节点、driver 名和可用格式必须在真实硬件到货后枚举确认，不能在规划中猜死。

## Consequences

### Positive

- 当前 UVC MVP 不受未来硬件阻塞。
- 未来设备差异集中在 CameraSource module，而不是泄漏到 HTTP 和 motion。
- seam 由第二个真实 adapter 触发，避免提前抽象。

### Negative

- 当前 `capture_v4l2` 仍是 UVC/MJPEG 专用实现。
- 后续接入 YUYV 或 OV5640 时需要提取帧描述、协商和转换路径。

### Neutral / Follow-up

M3 若需要 YUYV，或 OV5640 增强项正式开始时，重新评审并实现 seam。届时以 interface 为测试面，分别验证 UVC MJPEG 与第二个 adapter。

## Verification

- M2 验收只要求 UVC MJPEG。
- M3 evidence 记录 motion 输入方案和性能依据。
- OV5640 接入必须保存真实 `v4l2-ctl`、设备树和原理图证据。
