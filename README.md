# i.MX6ULL Sense Terminal

嵌入式 Linux 智能感知与设备运维终端。

本项目面向野火 EBF6ULL S1 Pro / i.MX6ULL eMMC 开发板，目标是做出一个小而闭环、可上板运行、可演示、可写进简历的嵌入式 Linux 项目。

## MVP

最小闭环：

```text
V4L2 摄像头 / UVC 采集
        -> JPEG/MJPEG 推流
        -> 浏览器预览
        -> motion event
        -> 本地事件日志
        -> systemd watchdog/restart
        -> 故障注入报告
```

## 为什么不直接使用成熟 streamer？

本项目参考 uStreamer、mjpg-streamer、Motion、v4l-utils 和 libjpeg-turbo 的工程取舍，但最终实现自己的最小闭环版本。

原因：

- 直接使用成熟项目，简历里“自己实现了什么”会变弱。
- 成熟项目功能太多，不适合第一版落地。
- 自己实现核心链路能更好地训练 V4L2、socket、多线程、daemon、systemd 和故障注入能力。

## 仓库结构

```text
app/daemon/       C daemon 骨架，后续放 MVP 模块
config/           运行时配置示例
docs/             bring-up、推流、事件、可靠性和面试笔记
docs/stage_summaries/
                  每个完成阶段一份总结
docs/bug_reports/ 每个 meaningful bug 一份复盘，并维护跨阶段汇总
docs/plans/       已保存项目计划
docs/reference/   开源参考、本地资料索引和设计决策
scripts/          构建、部署、辅助脚本
systemd/          service unit 模板
```

## 当前状态

M0 已完成。ARM scaffold 已经交叉编译、复制到 i.MX6ULL 板端，并通过 USB RNDIS `192.168.7.2` 成功运行。

当前执行路线：

```text
M1 USB UVC 摄像头 bring-up
  -> M2 MJPEG 浏览器推流
  -> M3 motion event 日志
  -> M4 systemd 与故障注入
  -> M5 最终文档和演示包装
```

当前文档规则：

- 每完成一个阶段，在 `docs/stage_summaries/` 写阶段总结。
- 每处理一个 meaningful bug，在 `docs/bug_reports/` 写独立复盘。
- 如果某阶段出现 bug，必须同时在阶段总结和 `docs/bug_reports/README.md` 中链接。
- 涉及板级硬件、pinout、启动、CSI、USB 或设备树的问题，先查 `docs/reference/local_board_documents.md`，不要凭经验猜测。

