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
| 公共接口 | `app/daemon/sense.h` | 配置、共享状态、快照和模块函数声明 |

## 当前依赖方向

```text
main
  -> config
  -> state
  -> capture_v4l2
  -> http_server

capture_v4l2 -> state
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

## M3 计划组件

- `motion_detector`：从可用于检测的图像数据计算变化分数。
- `event_log`：追加写入 JSONL 运动事件。

当前 UVC 主链路直接传输压缩 MJPEG。M3 开始前必须先决定解码 MJPEG、切换 YUYV，或采用其他低成本检测输入；不能把计划中的 Y 分量实现描述成现状。

## M4 计划组件

- systemd unit 的正式安装和启用。
- 启动、停止、重启和诊断 runbook。
- kill、reboot、camera missing 和 bad config 故障注入。
