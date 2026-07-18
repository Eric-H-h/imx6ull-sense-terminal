# 识别 UVC 节点并采集一帧

## 前置条件

- USB UVC 摄像头已连接开发板。
- 板端已安装 `v4l2-ctl`。
- 已通过 USB RNDIS 登录板端。

## 建立设备映射

```sh
v4l2-ctl --list-devices
ls -l /dev/video*
```

不要假设 `/dev/video0` 是摄像头。对候选节点先检查：

```sh
v4l2-ctl -d /dev/videoX -D
```

图像节点必须同时满足：

- driver 为 `uvcvideo`。
- Device Caps 包含 `Video Capture`。
- Device Caps 包含 `Streaming`。

Metadata Capture、Video Output、PXP 和 CSI 节点不能当作 UVC 图像输入。

## 枚举格式

```sh
v4l2-ctl -d /dev/videoX --list-formats-ext
```

## 采集 MJPEG 首帧

将 `VIDEO_DEV` 替换为本次识别出的 UVC Video Capture 节点：

```sh
VIDEO_DEV=/dev/videoX
v4l2-ctl -d "$VIDEO_DEV" \
  --set-fmt-video=width=640,height=480,pixelformat=MJPG \
  --set-parm=30 \
  --stream-mmap --stream-count=1 \
  --stream-to=/tmp/uvc_first_frame.jpg
ls -l /tmp/uvc_first_frame.jpg
```

采集后将图片复制到主机并实际打开验证，不要只凭非零文件大小判断成功。

## PXP 风险

当前 BSP 中查询 PXP Video Output 节点曾触发内核 Oops。不要对未识别用途的节点批量执行 `--all`。详情见 [M1-DRV-PXP-001](../bug_reports/M1-DRV-PXP-001_pxp_query_kernel_oops.md)。

真实 M1 设备映射和首帧证据见 [M1 UVC capture](../verification/evidence/M1_uvc_capture.md)。
