# 架构决策记录

ADR 保存影响硬件路线、接口、模块边界、性能、可靠性或文档治理的长期决定。

## 当前决策

| ADR | 状态 | 决策 |
| --- | --- | --- |
| [0001](0001-use-uvc-first.md) | Accepted | MVP 使用 UVC-first，OV5640 为增强项 |
| [0002](0002-use-mjpeg-over-http.md) | Accepted | 浏览器预览使用 MJPEG over HTTP |
| [0003](0003-build-minimal-daemon.md) | Accepted | 自研最小 daemon，不直接交付成熟 streamer |
| [0004](0004-camera-failure-degraded-mode.md) | Accepted | 摄像头失败进入 degraded 并重试 |
| [0005](0005-use-docs-as-code-structure.md) | Accepted | 文档按用途建立唯一事实源 |
| [0006](0006-defer-camera-source-seam.md) | Accepted | 推迟正式 CameraSource seam，保留通用帧模型 |

## 状态

- `Proposed`：正在评估。
- `Accepted`：当前有效。
- `Superseded`：已被新 ADR 取代。
- `Rejected`：评估后不采用。

## 规则

- 一个 ADR 只记录一个决定。
- 文件按 `NNNN-short-title.md` 命名。
- 旧决定不删除；变更时创建新 ADR 并双向链接。
- 路线原因写 ADR，当前执行顺序写 [`plans/current.md`](../../plans/current.md)。
- 新 ADR 使用 [`_template.md`](_template.md)。
