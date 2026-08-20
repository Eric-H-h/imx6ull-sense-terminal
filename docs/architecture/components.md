# Daemon 组件

本文描述当前源码已经存在的模块。尚未实现的模块明确标为“计划”。

## 当前组件

| 组件 | 源码 | 当前职责 |
| --- | --- | --- |
| `main` | `app/daemon/main.c` | 参数解析、信号处理、状态初始化、启动采集线程和 HTTP 主循环 |
| `config` | `app/daemon/config.c` | 加载项目所需的有限 JSON 配置 |
| `capture_v4l2` | `app/daemon/capture_v4l2.c` | 查找 UVC Capture 节点、协商 MJPEG、MMAP 采集和失败重试 |
| `state` | `app/daemon/state.c` | 保存最新 JPEG、运行状态、互斥锁、条件变量和客户端计数 |
| `http_server` | `app/daemon/http_server.c` | 提供页面、MJPEG stream、JSON status 和连接线程 |
| `jpeg_decoder` | `app/daemon/jpeg_decoder.c` | 使用 libjpeg 将内存 JPEG 缩放解码为 8-bit grayscale |
| `motion_detector` | `app/daemon/motion_detector.c` | 保存上一灰度帧并计算变化像素比例 |
| `motion_event_gate` | `app/daemon/motion_event_gate.c` | 维护 IDLE/COOLDOWN 和事件计数 |
| `event_log` | `app/daemon/event_log.c` | 将运动事件追加为单行 JSONL |
| `motion_pipeline` | `app/daemon/motion_pipeline.c` | 编排解码、帧差、cooldown 和事件日志 |
| `motion_worker` | `app/daemon/motion_worker.c` | 按配置低频抽样最新 JPEG、处理恢复并发布状态 |
| `status_json` | `app/daemon/status_json.c` | 将状态快照安全序列化为 JSON |
| 公共接口 | `app/daemon/sense.h` | 配置、共享状态、快照和模块函数声明 |

## 当前依赖方向

```text
main
  -> config
  -> state
  -> capture_v4l2
  -> motion_worker -> motion_pipeline
                   -> jpeg_decoder
                   -> motion_detector
                   -> motion_event_gate
                   -> event_log
  -> http_server -> status_json

capture_v4l2 -> state
motion_worker -> state
http_server  -> state
```

`state` 是当前采集端和 HTTP 端之间的共享边界。HTTP 客户端只读取最新完整 JPEG，不持有 V4L2 buffer。

## CameraSource 演进 seam

当前 `capture_v4l2` 是 UVC/MJPEG 专用实现，不把它描述为所有 V4L2 摄像头的最终 interface。

当第二个真实 adapter 出现时，再提取 CameraSource module。该 module 应隐藏设备发现、格式协商、buffer 生命周期和具体 driver 差异，并向调用方提供包含以下元数据的帧：

```text
pixel_format
width
height
stride / bytesused
sequence
timestamp
```

M2 不实现插件 ABI，也不猜测 OV5640 的节点、driver 或格式。详细决定见 [ADR-0006](decisions/0006-defer-camera-source-seam.md)。

## M3 已实现组件

M3 复用 M2 的 latest JPEG，以 3 FPS 低频抽样并使用 libjpeg `1/4` scaling 解码到 160x120 grayscale。耗时操作在 AppState 锁外完成，不改变 UVC MJPEG pass-through 主链路。选择依据和板端性能数据见 [M3 evidence](../verification/evidence/M3_motion_event.md)。

## M4 已实现组件

- `imx6ull-sense.service`：`User=debian`，`Restart=on-failure`，`RestartPreventExitStatus=78`。
- 安装脚本将二进制、配置和 unit 放到正式路径，默认不 enable/start。
- 摄像头缺失、事件日志写失败进入 degraded；崩溃由 systemd 拉起；非法配置以 78 退出。
- 操作见 [服务生命周期](../operations/runbooks/service-lifecycle.md)，证据见 [M4 evidence](../verification/evidence/M4_fault_injection.md)。
