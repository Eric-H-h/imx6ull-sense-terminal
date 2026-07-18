# 摄像头采集链路原理

## UVC 与 V4L2

USB UVC 摄像头由 Linux `uvcvideo` 驱动识别，并通过 V4L2 暴露为 `/dev/videoX`。设备编号不是稳定身份，同一摄像头还可能同时暴露 Video Capture 和 Metadata Capture 节点，因此应用必须根据 driver 和 Device Caps 选择图像节点。

V4L2 streaming 的基本过程是：

```text
open
  -> VIDIOC_QUERYCAP
  -> VIDIOC_S_FMT
  -> VIDIOC_REQBUFS
  -> VIDIOC_QUERYBUF + mmap
  -> VIDIOC_QBUF
  -> VIDIOC_STREAMON
  -> select/poll
  -> VIDIOC_DQBUF
  -> 使用帧
  -> VIDIOC_QBUF
```

## 当前 MJPEG 路线

当前 M2 请求摄像头直接输出 MJPEG。采集线程从 MMAP buffer 取得 JPEG 字节，检查基本帧头后复制到 AppState。HTTP 端直接发送这些 JPEG，不执行软件编码。

优点是降低 CPU 占用和实现复杂度。限制是压缩数据不能像 YUYV 那样直接读取亮度分量，M3 motion 开始前必须明确检测输入方案。

## 帧模型与后续格式

未来支持第二种输入格式时，帧不能只用“JPEG 字节”隐式表示。最小帧描述需要携带像素格式、宽高、stride 或 bytesused、sequence 和 timestamp。

计划路径：

```text
UVC MJPEG -> JPEG pass-through -> latest JPEG -> HTTP
UVC/OV5640 YUYV -> Y data for motion -> JPEG encoder -> latest JPEG -> HTTP
```

当前只有一个 UVC MJPEG adapter，因此 M2 不提取正式 CameraSource seam。M3 若需要 YUYV，或 OV5640 增强项开始时，再按 [ADR-0006](../architecture/decisions/0006-defer-camera-source-seam.md) 实现。

## OV5640 增强路线

OV5640 可能通过 i.MX6ULL CSI 输出 YUV 数据，涉及设备树、MCLK、供电、RESET/PWDN 和排线 pinout。它仍可复用上层状态、HTTP 和事件模块，但不属于当前 UVC MVP 的验收条件。具体设备节点、driver 名和格式必须在真实模组到货后枚举，规划阶段不猜测。

路线决定见 [ADR-0001](../architecture/decisions/0001-use-uvc-first.md)，板级事实必须优先查阅 [本地硬件资料索引](../reference/hardware/local-board-documents.md)。
