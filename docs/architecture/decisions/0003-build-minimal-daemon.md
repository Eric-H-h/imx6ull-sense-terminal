# ADR-0003：自研最小 daemon，不直接交付成熟 streamer

## Status

Accepted

## Context

uStreamer 和 mjpg-streamer 已能解决类似的视频推流问题，但项目目标包括理解和展示 V4L2、共享帧、多线程、socket、状态接口及故障恢复。

## Considered Options

- 直接部署 uStreamer 或 mjpg-streamer。
- fork 成熟项目并做少量修改。
- 参考成熟项目的取舍，自研最小闭环。

## Decision

最终演示使用本仓库实现的最小 daemon。成熟项目仅用于接口、并发、性能和异常处理参考，不复制其实现代码。

## Consequences

### Positive

- 项目实现边界可解释、可调试，设计取舍可被追问。
- 只保留当前硬件和 MVP 所需能力。

### Negative

- 需要自行处理 socket、线程退出、资源释放和异常路径。
- 功能和成熟度不会等同于生产级 streamer。

### Neutral / Follow-up

M5 可以运行开源 streamer 作为 benchmark，但它不是产品主线。
