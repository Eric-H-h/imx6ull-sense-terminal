# M0 阶段总结：环境与板端基线

## 状态

- 里程碑：M0 — 环境与板端基线
- 完成日期：2026-07-03
- 结果：已完成
- M0 后默认板端连接：USB RNDIS `192.168.7.2`
- 下一阶段：M1 — USB UVC 摄像头 bring-up 与一帧采集

## 阶段范围

M0 的目标是在编写 V4L2/MJPEG 代码前，先证明主机环境、交叉编译路径、板端登录路径、文件传输路径和板端运行路径是可用的。

M0 不包含摄像头采集、MJPEG 推流、运动检测、systemd 强化或故障注入。

## 验收结果

| 检查项 | 结果 | 证据 |
| --- | --- | --- |
| WSL 构建工具可用 | 通过 | `make`、`gcc`、`git` 已记录在 `docs/01_bringup.md` |
| 主机侧 scaffold 可编译运行 | 通过 | `app/daemon/imx6ull-sense` scaffold 输出已记录 |
| ARM 交叉编译可用 | 通过 | `file app/daemon/imx6ull-sense` 显示 ARM EABI5 可执行文件 |
| 主机可访问板端 | 通过 | USB RNDIS ping `192.168.7.2` 为 0% 丢包 |
| 文件传输路径可用 | 通过 | `scp` 二进制和配置到 `/tmp`，退出码为 0 |
| 板端可运行 scaffold | 通过 | 板端输出 scaffold 日志并返回 `exit:0` |
| 真实日志已归档 | 通过 | `docs/01_bringup.md` 已包含 USB RNDIS 和板端运行日志 |

## 关键证据

ARM 二进制检查：

```text
app/daemon/imx6ull-sense: ELF 32-bit LSB executable, ARM, EABI5 ... interpreter /lib/ld-linux-armhf.so.3
```

USB RNDIS 连通性：

```text
$ ping -c 4 192.168.7.2
4 packets transmitted, 4 received, 0% packet loss
```

板端 USB 网卡状态：

```text
usb0: <BROADCAST,MULTICAST,UP,LOWER_UP>
inet 192.168.7.2/30
```

板端运行输出：

```text
imx6ull-sense daemon scaffold
config: /tmp/imx6ull-sense-config.json
next: implement capture_v4l2 -> jpeg_encoder -> http_server -> motion_detector
exit:0
```

## 本阶段 bug

| Bug ID | 摘要 | 状态 | 报告 |
| --- | --- | --- | --- |
| M0-NET-WIFI-001 | Wi-Fi/SSH/SCP 链路不稳定，USB RNDIS 初期受数据线和 Windows 驱动绑定影响 | M0 已绕过并解决 | [bug 复盘](../bug_reports/M0-NET-WIFI-001_usb_rndis_board_link.md) |

## 关键决策

- M0 后默认板端连接改为 USB RNDIS `192.168.7.2`。
- Wi-Fi `192.168.18.210` 不再作为默认部署路径，因为它出现丢包、SSH 超时和 `No route to host`。
- 只有在板端 ARM 程序返回 `exit:0` 后，M0 才算关闭。
- M1 应从新的里程碑分支 `codex/m1-uvc-capture` 开始。

## 已更新文档

- `docs/01_bringup.md`：补充 USB RNDIS、SCP 和板端运行真实日志。
- `docs/plans/plan_v0.3_mvp_execution.md`：记录 UVC-first 路线和 Git 工作流规则。
- `docs/stage_summaries/M0_environment_and_board_baseline.md`：本阶段总结。
- `docs/bug_reports/M0-NET-WIFI-001_usb_rndis_board_link.md`：M0 网络 bug 复盘。
- `docs/bug_reports/README.md`：跨阶段 bug 索引。

## Git

分支：

```text
develop
```

M0 后已有提交：

```text
8408572 docs: add git workflow rules to execution plan
d999cb6 chore: initialize imx6ull sense terminal project
```

本地忽略产物：

```text
app/daemon/imx6ull-sense
app/daemon/main.o
secrets.local.md
```

## 下一步方向

在 `codex/m1-uvc-capture` 上启动 M1。

M1 首先要证明 USB UVC 摄像头能在板端枚举为 `/dev/videoX`，并记录：

```sh
ls /dev/video*
v4l2-ctl --list-devices
v4l2-ctl --list-formats-ext -d /dev/video0
```

完成枚举后，抓取一帧真实图像，并把命令、输出和生成文件记录到 `docs/02_camera_capture.md`。
