# ADR-0002：浏览器预览使用 MJPEG over HTTP

## Status

Accepted

## Context

项目需要在 i.MX6ULL 上提供无需安装播放器的局域网浏览器预览。Cortex-A7 上的软件 H.264 编码会增加实现和性能风险。

## Considered Options

- MJPEG over HTTP。
- H.264/RTSP。
- 只保存本地图片，不提供实时预览。

## Decision

MVP 使用 `multipart/x-mixed-replace` 传输连续 JPEG 帧，并提供 `/`、`/stream` 和 `/status`。

## Consequences

### Positive

- 浏览器可直接显示。
- 协议简单，容易抓包、验证和解释。
- UVC 摄像头输出 MJPEG 时可以直接传输。

### Negative

- 带宽高于 H.264。
- 不适合公网或大规模客户端。

### Neutral / Follow-up

MVP 只要求局域网少量客户端；实际 FPS、CPU、RSS 和稳定性由 M2 验收决定。

## Verification

验收结果写入 [M2 证据](../../verification/evidence/M2_mjpeg_stream.md)。
