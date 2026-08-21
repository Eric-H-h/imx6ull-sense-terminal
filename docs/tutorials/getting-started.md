# 从零理解并运行项目

本教程面向第一次克隆仓库的人。它给出阅读和执行顺序，不复制每份操作指南里的完整命令。

## 1. 项目做什么

在 EBF6ULL S1 Pro / i.MX6ULL 上完成：

```text
USB UVC 摄像头
  -> V4L2 采集
  -> MJPEG 浏览器预览
  -> motion event
  -> JSONL 日志
  -> systemd 与故障注入
```

先读仓库根目录 [README](../../README.md) 和 [架构总览](../architecture/overview.md)。综合结论见 [测试报告](../verification/test-report.md)。

## 2. 准备开发环境

在克隆后的仓库根目录操作。按照 [准备 WSL 开发环境](../how-to/setup-development-host.md) 安装编译工具，并确认：

```sh
make -C app/daemon verify
./scripts/build-arm.sh
```

## 3. 连接开发板

默认使用 USB RNDIS，常见地址：

```text
debian@192.168.7.2
```

以板端 `usb0` 实际地址为准。按照 [连接和部署开发板](../how-to/connect-and-deploy-board.md) 检查 OTG、RNDIS、SSH。不要启用 `autowifi.service`。

## 4. 安装并打开画面

正式安装和启停见 [服务生命周期](../operations/runbooks/service-lifecycle.md)。临时调试可用 [运行和检查 MJPEG 服务](../how-to/run-mjpeg-stream.md)。

浏览器：

```text
http://192.168.7.2:8080/
```

```sh
curl --noproxy 192.168.7.2 http://192.168.7.2:8080/status
```

现场按时间轴演示见 [怎样演示这个服务](../presentation/demo-script.md)。

## 5. 按阶段深入

- [M0](../stage_summaries/M0_environment_and_board_baseline.md)：主机、交叉编译、网络、部署和板端运行
- [M1](../stage_summaries/M1_usb_uvc_camera_capture.md)：UVC 节点、格式和首帧
- [M2](../stage_summaries/M2_mjpeg_browser_stream.md)：浏览器 MJPEG 和 `/status`
- [M3](../stage_summaries/M3_motion_event_logging.md)：motion event 与 JSONL
- [M4](../stage_summaries/M4_systemd_fault_injection.md)：systemd 与故障注入
- [M5](../stage_summaries/M5_resume_demo_packaging.md)：测试报告与发布说明

卡住时先看 [Bug 复盘索引](../bug_reports/README.md)，不要从错误假设重新排查。

## 6. 想改代码时

阅读 [如何参与](../../CONTRIBUTING.md) 和 [Git 工作流](../how-to/git-workflow.md)。按明确文件清单暂存，不要使用 `git add .`。
