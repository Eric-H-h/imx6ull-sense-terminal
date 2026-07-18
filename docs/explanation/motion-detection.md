# Motion Detection 原理

## 状态

Planned for M3。当前 `/status` 中的 motion 字段是占位状态，不能作为已实现证据。

## 基本模型

MVP 计划采用低成本帧差，而不是目标检测模型：

```text
当前亮度数据 - 参考亮度数据
  -> 变化像素或归一化 score
  -> threshold
  -> cooldown
  -> JSONL motion event
```

`threshold` 控制灵敏度，`cooldown` 防止同一次运动连续写入大量事件。

## 当前待决问题

M2 主链路直接使用压缩 MJPEG。M3 必须通过实测在以下方案中选择：

- 对部分帧解码后计算亮度差。
- 让 UVC 摄像头切换或并行提供 YUYV。
- 使用更低成本但可解释的图像统计量。

选择必须以 i.MX6ULL CPU、内存、FPS 和事件准确性实测为依据，不能直接把早期 YUYV 设想写成当前实现。
