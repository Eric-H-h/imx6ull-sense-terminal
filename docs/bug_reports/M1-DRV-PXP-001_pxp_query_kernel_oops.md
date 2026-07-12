# Bug M1-DRV-PXP-001：查询 PXP 节点触发内核 Oops

## 状态

- 归属阶段：M1 — USB UVC 摄像头采集
- 类型：内核 / V4L2 / PXP 驱动
- 状态：Mitigated，根因待确认；UVC 绕过路径已验证
- 发现日期：2026-07-12
- 当前绕过方式：M1 只操作 UVC Video Capture 节点，不查询或采集 `/dev/video0`

## Environment

- 开发板：野火 EBF6ULL S1 Pro / i.MX6ULL eMMC
- 板端内核：Linux 4.19.35-imx6，ARMv7
- 工具：`v4l2-ctl`
- PXP 节点：`/dev/video0`
- UVC 图像节点：动态编号；重启后本次为 `/dev/video1`
- UVC 元数据节点：动态编号；重启后本次为 `/dev/video2`

## Symptom

对 `/dev/video0` 执行 `v4l2-ctl --all` 时，PXP 驱动先返回 Video Output 能力，随后内核发生 Oops，`v4l2-ctl` 以 Segmentation fault 结束。

USB 摄像头已由 `uvcvideo` 驱动枚举到 `/dev/video2` 和 `/dev/video3`，因此该异常不表示 UVC 摄像头枚举失败。

## Reproduction

```sh
v4l2-ctl -d /dev/video0 --all
```

## Evidence

```text
Driver name      : pxp
Bus info         : pxp_v4l2
Capabilities     : Video Output, Video Output Overlay, Streaming

kernel: Internal error: Oops: 5 [#1] PREEMPT SMP ARM
kernel: Process v4l2-ctl (pid: 998, stack limit = ...)
kernel: Code: e1a0000c e584e004 e19330b2 e5843008 (e5912008)
Segmentation fault
```

## Root cause

尚未确认。

当前证据把故障范围限定在 PXP V4L2 节点的查询路径。日志中的 `feedcafe` 可能是内核内存调试或毒化标记，但不足以单独证明具体的无效指针类型。

需要结合完整 `dmesg` 调用栈、`v4l2-ctl` 版本、BSP 内核 PXP 驱动源码和可重复性进一步定位。当前不把工具兼容问题或驱动实现缺陷提前认定为根因。

## Fix/workaround

M1 使用能力为 Video Capture、驱动为 `uvcvideo` 的节点。重启后本次枚举为 `/dev/video1`。不将 `/dev/video0` 当作摄像头，也不再对它执行 `--all`。发生 Oops 后先重启开发板，再继续 UVC 采集验证。

设备编号可能在重启或重新插拔后变化，后续应按驱动名和设备能力识别节点，不能永久写死任何 `/dev/videoX` 编号。

## Verification

绕过路径已由设备能力确认：

```text
/dev/video1
Driver name      : uvcvideo
Device Caps      : Video Capture, Streaming
Pixel Format     : 'MJPG'

/dev/video2
Driver name      : uvcvideo
Device Caps      : Metadata Capture, Streaming
```

UVC 绕过路径已完成 `MJPG 640x480@30 fps` 的真实一帧采集。PXP 查询异常本身未修复且根因未确认，因此状态保持 Mitigated，不标记为 Resolved。

## Impact on plan

- 不阻塞 UVC-first 主线，M1 继续使用 UVC Video Capture 节点。
- M1 阶段总结必须引用本报告。
- PXP 深入定位不进入 M1 主线，除非后续功能必须依赖 PXP。

## 本地资料检查

- 已按规则检查 `docs/reference/local_board_documents.md` 的资料优先级。
- 当前证据来自板端 V4L2 枚举和内核日志；尚未定位到本地教程或原理图中的 PXP V4L2 专门章节/页码。
- 后续若进入根因修复，应优先查阅当前 4.19.35-imx6 BSP 的 PXP V4L2 驱动源码。

## Follow-up

1. 保存 Oops 前后的完整 `dmesg`，包含函数调用栈和 loaded modules。
2. 当前板端 v4l2-ctl 不支持 --version；如需版本信息，使用 dpkg-query -W v4l-utils。
3. 如项目后续需要 PXP，再建立可重复的最小查询用例并对照 BSP 驱动源码定位。
