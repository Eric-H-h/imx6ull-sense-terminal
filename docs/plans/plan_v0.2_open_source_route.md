# Plan v0.2 Open-Source-Informed Route

## Summary

本项目不闭门造车。路线改为：

> 参考高质量开源项目的架构、接口和工程取舍；自己实现一个小而闭环的学生版。

当前版本只调整路线，不扩大 MVP 范围。

## Open-Source References

1. **uStreamer / PiKVM**
   - 参考轻量 V4L2 MJPEG HTTP streamer 思路。
   - 借鉴 capture / encode / HTTP / status / degraded / systemd 集成。
   - 不直接复制代码。

2. **mjpg-streamer**
   - 参考 input/output 解耦。
   - 不实现复杂插件系统，只保留采集层和输出层分离。

3. **Motion**
   - 参考 motion event、阈值、冷却时间和长期运行 daemon 思路。
   - 不实现录像、区域检测和复杂配置系统。

4. **v4l-utils**
   - 作为 V4L2 bring-up 和验证标准。
   - 使用 `v4l2-ctl` 枚举设备、格式和抓帧。

5. **libjpeg-turbo**
   - 作为 JPEG 性能优化方向。
   - 第一版优先使用系统 libjpeg。

## Phases

### Phase 0 开源项目拆解

输出：

```text
docs/reference/ustreamer_notes.md
docs/reference/mjpg_streamer_notes.md
docs/reference/motion_notes.md
docs/reference/design_decisions.md
```

回答：

- 成熟项目如何组织 capture / encode / HTTP？
- input/output 分离为什么适合嵌入式？
- motion event 如何定义？
- 哪些设计适合 i.MX6ULL，哪些必须砍掉？

### Phase 1 自己实现最小视频链路

模块：

```text
capture_v4l2.c
jpeg_encoder.c
http_server.c
frame_buffer.c
main.c
```

成功标准：

- 浏览器能访问 `/stream`。
- `/status` 显示 fps、frame_count、client_count、motion_state。
- 单客户端稳定运行 30 分钟。

### Phase 2 小型事件检测系统

模块：

```text
motion_detector.c
event_log.c
status.c
```

成功标准：

- 晃手触发 motion。
- `/status` 显示 `motion_state=true`。
- 本地 JSONL 日志记录 motion event。

### Phase 3 服务化与故障注入

模块：

```text
systemd/imx6ull-sense.service
config/config.json
docs/05_fault_injection.md
docs/test_report.md
```

必做故障：

- kill 进程后自动恢复。
- 摄像头不存在时进入 degraded 状态。
- 配置损坏时 fail-safe。
- 重启后服务自动启动。

## Key Decisions

- 不直接使用 uStreamer/mjpg-streamer 作为最终主体。
- 不复制 GPL 项目代码。
- 可以运行开源项目作为 benchmark。
- 自己实现核心：V4L2 采集、MJPEG HTTP、motion event、systemd 可靠性。
- 简历重点写自己实现和验证的闭环。

