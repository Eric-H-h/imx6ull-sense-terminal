# ADR-0004：摄像头失败进入 degraded 状态

## Status

Accepted

## Context

摄像头可能未插入、节点编号变化、格式协商失败或运行中断开。HTTP 状态服务仍然有诊断价值，简单退出会隐藏实际原因。

## Considered Options

- 摄像头失败立即退出进程。
- 忽略失败并继续返回旧画面。
- 保持 HTTP 服务，报告 degraded，并周期性重试摄像头。

## Decision

可恢复的摄像头故障进入 degraded 状态，更新 `last_error`，保留 `/status`，并重新扫描合适的 UVC Video Capture 节点。

## Consequences

### Positive

- 用户可以区分“服务不可达”和“摄像头不可用”。
- 摄像头重新接入后可以自动恢复采集。

### Negative

- 进程生命周期和退出同步更复杂。
- 必须验证重试不会形成高频循环或资源泄漏。

## Verification

M2 验证基本 degraded 与重连行为；M4 再执行正式 camera-missing 故障注入。
