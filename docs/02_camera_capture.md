# 02 摄像头采集

## 目标

从 **USB UVC 摄像头** 捕获至少一帧真实图像。USB UVC 是 MVP 默认路线；CSI/OV5640 作为后续可选增强项。

## 设备发现

```sh
v4l2-ctl --list-devices
v4l2-ctl -d /dev/videoX --list-formats-ext
```

## 第一帧采集

```sh
VIDEO_DEV=/dev/videoX
v4l2-ctl -d "$VIDEO_DEV" --stream-mmap --stream-count=1 --stream-to=/tmp/m1_frame_$(date -u +%Y%m%dT%H%M%SZ).yuv
ls -l /tmp/m1_frame_*.yuv
wc -c /tmp/m1_frame_*.yuv
```

## 结果记录

M1 执行后需要把以下验收输出追加到本文件：

```text
ls -l /dev/videoX
v4l2-ctl --list-devices
v4l2-ctl --list-formats-ext -d /dev/videoX
v4l2-ctl --stream-mmap --stream-count=1 --stream-to=/tmp/m1_first_frame.yuv
```

- `v4l2` 设备路径：`/dev/videoX`
- 捕获命令退出码：`0`
- 产物文件大小 > 0：`wc -c` 输出 > 0

## 备注

- 如果没有 `/dev/video*`：优先怀疑摄像头没有被枚举，检查 USB/UVC 线缆、USB 口和驱动。
- 如果格式列表为空：换 USB 口、换线，或换兼容性更好的普通 UVC 摄像头。
- 涉及 USB、电源、pinout 或板级问题时，先查 `docs/reference/local_board_documents.md` 中的本地资料索引。

