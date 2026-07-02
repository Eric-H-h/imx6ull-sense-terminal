# Plan v0.1 Baseline

## Summary

目标是在 2026-07-31 前完成一个小而闭环、能上板运行、能录演示视频、能写进简历的嵌入式 Linux 项目。

仓库位置：

```text
/home/eric/projects/imx6ull-sense-terminal
```

主线：

```text
M0 环境闭环
-> M1 摄像头/UVC 采集
-> M2 MJPEG 浏览器预览
-> M3 运动事件与日志
-> M4 systemd 守护与故障注入
-> M5 README / 测试报告 / 演示视频 / 简历包装
```

## Flexible Delivery Levels

| 等级 | 完成内容 | 用途 |
|---|---|---|
| 保底版 | UVC/OV5640 采集 + MJPEG 浏览器预览 + 守护机制 + README | 最低可写简历、可演示 |
| 标准版 | 保底版 + 运动检测 + 事件日志 + 4 个故障注入测试 | 推荐最终目标 |
| 增强版 | 标准版 + OV5640 设备树理解 + HDMI/字符驱动/OTA/TTFF 任一加分项 | 时间充足时做 |

## Milestones

### M0 环境与板子基线

- 初始化 Git 仓库。
- 建立基础目录。
- 配置交叉编译工具链。
- 编译并上板运行 `hello_imx6ull`。
- 记录串口、IP、系统版本、`dmesg` 简查。

### M1 摄像头采集闭环

- 优先 OV5640。
- OV5640 两个晚上仍不出图则切 USB UVC。
- 使用 `v4l2-ctl` 枚举设备和格式。
- 自写最小 V4L2 mmap capture 程序。
- 保存一帧并转换为可查看图片。

### M2 MJPEG 浏览器预览

- 实现最小 HTTP server。
- `GET /` 返回 HTML。
- `GET /status` 返回 JSON。
- `GET /stream` 返回 MJPEG。
- 记录 fps、CPU、内存、分辨率。

### M3 运动事件与本地日志

- 提取 Y 分量。
- 实现帧差运动检测。
- 设置阈值和冷却时间。
- 事件写本地 JSONL 日志。

### M4 systemd 守护与故障注入

- 写 `imx6ull-sense.service`。
- 开机自启、异常恢复、`journalctl` 日志。
- 故障注入：kill 进程、重启、摄像头不存在、配置损坏。

### M5 收尾交付

- README。
- 测试报告。
- 1 分钟演示视频。
- 3 条简历 bullet。
- 10 个面试追问。

## Dynamic Rules

- OV5640 两晚不出图：切 UVC。
- HDMI 一晚不出图：降级为可选。
- 到 7 月 20 日仍没有 MJPEG：停止扩展，只保视频流和守护机制。
- 到 7 月 25 日仍没有 systemd：用 shell watchdog 临时替代。
- 最后一周禁止新增大功能。

