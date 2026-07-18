# M1 验收证据：USB UVC 摄像头采集

> 状态：Completed。本文保存真实节点、格式和首帧证据；当前阶段状态以 `stage_summaries/M1_usb_uvc_camera_capture.md` 为准。

## 目标

从 **USB UVC 摄像头** 捕获至少一帧真实图像。USB UVC 是 MVP 默认路线；CSI/OV5640 作为后续可选增强项。

## 设备发现

```sh
v4l2-ctl --list-devices
v4l2-ctl -d /dev/videoX --list-formats-ext
```
## M1 实际枚举结果

2026-07-12 的板端枚举结果：

| 节点 | 驱动 | Device Caps | M1 用途 |
| --- | --- | --- | --- |
| `/dev/video0` | `pxp` | Video Output | 不使用；查询 `--all` 会触发内核 Oops |
| `/dev/video1` | `mx6s-csi` | Video Capture | CSI 控制器当前未形成有效传感器链路，不使用 |
| `/dev/video2` | `uvcvideo` | Video Capture、Streaming | USB 摄像头图像帧候选节点 |
| `/dev/video3` | `uvcvideo` | Metadata Capture、Streaming | UVC 元数据节点，不作为图像帧来源 |

`/dev/video2 --all` 当前读到 `1920x1080`、`MJPG`、30 fps。这表示 V4L2 当前视频格式，不表示应用拿到 USB 总线上的原始 UVC 数据包。UVC 驱动已经完成 USB 传输解析，Video Capture 节点向用户态提供按当前像素格式组织的视频帧负载。

格式枚举命令当前只返回头部，没有列出格式项：

```text
$ v4l2-ctl -d /dev/video2 --list-formats-ext
ioctl: VIDIOC_ENUM_FMT
        Type: Video Capture
```

这与 `--all` 能通过 `VIDIOC_G_FMT` 读到当前 MJPG 格式不完全一致。当前先标记为待确认现象；需要检查命令退出码、基础格式枚举、工具版本和内核日志后，再判断是设备枚举限制、工具兼容问题还是驱动行为。

PXP 异常见 `docs/bug_reports/M1-DRV-PXP-001_pxp_query_kernel_oops.md`。

### 重启后的节点确认

开发板重启后，UVC 节点编号发生变化：

| 节点 | Device Caps | 用途 |
| --- | --- | --- |
| `/dev/video1` | Video Capture、Streaming | 当前 UVC 图像采集节点 |
| `/dev/video2` | Metadata Capture、Streaming | 当前 UVC 元数据节点 |
| `/dev/video3` | Video Capture、Streaming | i.MX6S CSI 节点，不属于 USB 摄像头 |

这证明 `/dev/videoX` 编号不是稳定接口。后续必须同时依据 `Driver name: uvcvideo` 和节点自身的 `Device Caps: Video Capture` 选择图像节点。

### UVC 支持格式

当前 `/dev/video1` 能够正常执行 `VIDIOC_ENUM_FMT`：

- `MJPG`：1920x1080、1280x720、640x480、352x288、424x240、640x360、800x480、800x600、1024x576、1600x896，均为 30 fps。
- `YUYV`：1920x1080@5 fps、1280x720@10 fps、640x480/352x288/424x240/640x360@30 fps、800x480/800x600/1024x576@15 fps、1600x896@7.5 fps。

M1 第一帧优先选择 `MJPG 640x480@30 fps`：文件可直接按 JPEG 特征验证，且 USB 带宽和板端内存压力低于同分辨率 YUYV。YUYV 路径留到第一帧闭环后再验证，不改变 M1 当前验收顺序。


## 第一帧采集

`M1` 使用重启后动态确认的 UVC Video Capture 节点：

```sh
VIDEO_DEV=/dev/video1
v4l2-ctl -d "$VIDEO_DEV" \
  --set-fmt-video=width=640,height=480,pixelformat=MJPG \
  --set-parm=30 \
  --stream-mmap \
  --stream-count=1 \
  --stream-to=/tmp/m1_first_frame.jpg
CAPTURE_RC=$?
echo "capture_exit:$CAPTURE_RC"
```

## 验收结果

| 检查项 | 结果 | 证据 |
| --- | --- | --- |
| 动态识别 UVC 图像节点 | 通过 | `Driver name: uvcvideo`，`Device Caps: Video Capture`；本次节点为 `/dev/video1` |
| 显式设置采集格式 | 通过 | `MJPG 640x480@30 fps` |
| 单帧采集 | 通过 | `v4l2-ctl --stream-mmap --stream-count=1` 已生成 `/tmp/m1_first_frame.jpg` |
| 产物非空 | 通过 | 操作者已在板端确认文件存在且包含第一帧数据 |
| JPEG 特征 | 通过 | 操作者已按 JPEG 头尾特征完成验证 |

操作者于 2026-07-12 确认第一帧采集成功。具体文件字节数、SHA-256 和完整 `dmesg` 输出没有保留在当前会话，因此本文不填写推测数值。

复核命令：

```sh
FRAME=/tmp/m1_first_frame.jpg
stat -c 'frame_size:%s bytes' "$FRAME"
od -An -tx1 -N2 "$FRAME"
tail -c 2 "$FRAME" | od -An -tx1
```

M1 验收结论：USB UVC 摄像头枚举、格式确认和真实一帧采集均已完成。

## 备注

- 如果没有 `/dev/video*`：优先怀疑摄像头没有被枚举，检查 USB/UVC 线缆、USB 口和驱动。
- 如果格式列表为空：换 USB 口、换线，或换兼容性更好的普通 UVC 摄像头。
- 涉及 USB、电源、pinout 或板级问题时，先查 [本地板级资料索引](../../reference/hardware/local-board-documents.md)。
