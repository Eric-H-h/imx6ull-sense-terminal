# i.MX6ULL Sense Terminal

嵌入式 Linux 智能感知与设备运维终端。

本项目面向野火 EBF6ULL S1 Pro / i.MX6ULL eMMC 开发板，目标是做出一个小而闭环、可上板运行、可演示、可写进简历的嵌入式 Linux 项目。

## MVP

最小闭环：

```text
V4L2 camera/UVC capture
        -> JPEG/MJPEG stream
        -> browser preview
        -> motion event
        -> local event log
        -> systemd watchdog/restart
        -> fault-injection report
```

## Why Not Use an Existing Streamer Directly?

本项目参考 uStreamer、mjpg-streamer、Motion、v4l-utils 和 libjpeg-turbo 的工程取舍，但最终实现自己的最小闭环版本。

原因：

- 直接使用成熟项目，简历里“自己实现了什么”会变弱。
- 成熟项目功能太多，不适合第一版落地。
- 自己实现核心链路能更好地训练 V4L2、socket、多线程、daemon、systemd 和故障注入能力。

## Repository Layout

```text
app/daemon/       C daemon skeleton and later MVP modules
config/           Runtime config examples
docs/             Bring-up, stream, event, reliability and interview notes
docs/plans/       Saved project plans
docs/reference/   Open-source reference notes and design decisions
scripts/          Build/deploy/helper scripts
systemd/          Service unit templates
```

## Current Status

This repository is currently initialized with the open-source-informed route and documentation scaffold. Hardware bring-up and target-board verification are the next steps.

