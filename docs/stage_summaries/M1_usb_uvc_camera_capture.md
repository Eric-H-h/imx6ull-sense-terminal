# M1 阶段总结：USB UVC 摄像头采集

## 状态

- 里程碑：M1 — USB UVC 摄像头枚举与一帧采集
- 完成日期：2026-07-12
- 结果：已完成
- 工作分支：`codex/m1-uvc-capture`
- 下一阶段：M2 — MJPEG 浏览器预览

## 阶段范围

M1 的目标是在编写项目自己的 V4L2/MJPEG 代码前，证明 USB UVC 摄像头能在 i.MX6ULL 板端完成设备枚举、能力识别、格式枚举和真实一帧采集。

M1 不包含应用内 V4L2 采集模块、HTTP 服务、运动检测、录像、CSI/OV5640 适配或 systemd 服务。

## 验收结果

| 检查项 | 结果 | 证据 |
| --- | --- | --- |
| USB 摄像头由 UVC 驱动识别 | 通过 | `Driver name: uvcvideo`，总线为 `usb-ci_hdrc.1-1.3` |
| 区分图像与元数据节点 | 通过 | 图像节点的 `Device Caps` 为 Video Capture；元数据节点为 Metadata Capture |
| 支持格式可枚举 | 通过 | `VIDIOC_ENUM_FMT` 列出 MJPG 和 YUYV |
| 显式选择轻量采集格式 | 通过 | `MJPG 640x480@30 fps` |
| 真实一帧采集 | 通过 | `/tmp/m1_first_frame.jpg` 已生成并由操作者验证 |
| M1 日志与 bug 记录 | 通过 | [M1 evidence](../verification/evidence/M1_uvc_capture.md) 和 M1 PXP bug 报告已更新 |

## 关键证据

重启后的设备映射：

```text
i.MX6S_CSI (platform:21c4000.csi): /dev/video3
pxp (pxp_v4l2): /dev/video0
Web Camera (usb-ci_hdrc.1-1.3): /dev/video1, /dev/video2
```

节点能力：

```text
/dev/video1: Driver=uvcvideo, Device Caps=Video Capture, Streaming
/dev/video2: Driver=uvcvideo, Device Caps=Metadata Capture, Streaming
```

格式能力摘要：

```text
MJPG: 640x480@30 fps（并支持多种更高分辨率）
YUYV: 640x480@30 fps；高分辨率下帧率下降
```

首帧命令：

```sh
VIDEO_DEV=/dev/video1
v4l2-ctl -d "$VIDEO_DEV" \
  --set-fmt-video=width=640,height=480,pixelformat=MJPG \
  --set-parm=30 \
  --stream-mmap --stream-count=1 \
  --stream-to=/tmp/m1_first_frame.jpg
```

操作者确认首帧文件有效。具体文件大小、哈希和完整 `dmesg` 数值未保留在当前会话，因此阶段总结不填写推测数据。

## 本阶段 bug

| Bug ID | 摘要 | 状态 | 报告 |
| --- | --- | --- | --- |
| M1-DRV-PXP-001 | 查询 PXP Video Output 节点时触发内核 Oops | Mitigated；UVC 绕过已验证，PXP 根因待确认 | [bug 复盘](../bug_reports/M1-DRV-PXP-001_pxp_query_kernel_oops.md) |

## 实际运行事故

无。M1 是摄像头 bring-up 和首帧验收阶段，PXP Oops 作为开发调试 Bug 记录，不归类为已部署服务事故。

## 关键决策

- MVP 继续采用 UVC-first，不在 M1 切换到 CSI/OV5640。
- `/dev/videoX` 编号会在重启后变化，后续必须同时依据 `uvcvideo` 驱动名和 `Device Caps: Video Capture` 选择图像节点。
- `Capabilities` 是设备综合能力；存在 Device Capabilities 标志时，节点用途以 `Device Caps` 为准。
- M1 首帧选择 MJPG 640x480@30 fps，便于直接验证 JPEG 且降低 USB 带宽和板端内存压力。
- UVC Metadata Capture 节点可以采集时序等元数据，但不作为图像帧来源。
- PXP 不属于当前 UVC 主线；除非后续功能必须依赖 PXP，否则不在 M1 深挖其 BSP 驱动。

## 风险与缓解

- PXP 查询路径的根因仍未关闭：继续绕开 `/dev/video0`，需要 PXP 时单独建立最小复现。
- 节点编号不稳定：每次启动动态枚举，不在配置中永久写死本次编号。
- 本次首帧的具体字节数和哈希未归档：后续板端验收命令应同时保存输出日志。

## 已更新文档

- `docs/verification/evidence/M1_uvc_capture.md`：记录节点映射、格式能力、首帧命令和验收结果。
- `docs/bug_reports/M1-DRV-PXP-001_pxp_query_kernel_oops.md`：记录 PXP Oops、绕过方式和验证。
- `docs/bug_reports/README.md`：登记 M1 PXP bug。
- `docs/stage_summaries/M1_usb_uvc_camera_capture.md`：本阶段总结。

## Git

当前分支：

```text
codex/m1-uvc-capture
```

当前基线提交：

```text
ad438f3 docs: document RNDIS bring-up and git workflow
```

本总结随 M1 文档提交；提交哈希以 Git 历史为准。提交前已按明确文件清单核对范围。

## 下一步

先整理并提交 M1 相关文档，再按项目 Git 规则准备合并回 `develop`。完成 M1 集成后，才能从 `develop` 创建 M2 工作分支并开始 MJPEG 浏览器预览；当前仍未编写项目自己的 V4L2/MJPEG 实现。
