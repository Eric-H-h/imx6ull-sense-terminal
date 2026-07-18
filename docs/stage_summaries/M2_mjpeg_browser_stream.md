# M2：MJPEG 浏览器推流

## 阶段状态

- 状态：Completed
- 完成日期：2026-07-18
- 分支：`codex/m2-mjpeg-stream`
- Commit：`b31be00`（功能代码）；本总结随文档提交

## 阶段目标

在 i.MX6ULL 开发板上把 USB UVC 摄像头的 MJPEG 数据持续采集到 daemon，通过 HTTP multipart 输出给 PC 浏览器，并提供可观察的 JSON 状态接口。

## 验收结果

| 验收项 | 结果 | 证据 |
| --- | --- | --- |
| 主机构建 | Passed | GCC 干净构建 exit 0，无告警 |
| ARM 构建与部署 | Passed | 操作者完成交叉编译、SHA-256 对比和板端启动 |
| UVC MJPEG 连续采集 | Passed | 板端 active，MJPG 640x480@30 |
| `/`、`/status`、`/stream` | Passed | 浏览器和 curl 验证 |
| multipart 与 JPEG 字节 | Passed | boundary、Content-Length、SOI/EOI 均正确 |
| 浏览器刷新与重连 | Passed | 关闭/刷新/重开后服务正常 |
| 客户端资源回收 | Passed | 打开时 1 client/3 threads，关闭后 0 client/2 threads |
| 稳定性 | Passed | 同一 PID 运行约 41 分 30 秒，RSS 仅增加 4 kB |
| 正常退出 | Passed | Ctrl+C 后进程消失且端口关闭 |

完整证据见 [M2 evidence](../verification/evidence/M2_mjpeg_stream.md)。

## 板端与运行事实

- 开发板/内核：EBF6ULL S1 Pro，Linux 4.19.35-imx6，ARMv7。
- 设备节点：动态识别 `uvcvideo + Video Capture + Streaming`；本次运行是 `/dev/video1`。
- 网络路径：USB RNDIS，板端 `192.168.7.2`。
- 关键配置：MJPG 640x480@30 fps，HTTP `0.0.0.0:8080`。
- 进程模型：主线程运行 HTTP accept loop，采集线程维护最新 JPEG，每个客户端独立线程。

## 关键变化

- 代码：新增配置加载、共享状态、V4L2 MMAP 采集、MJPEG HTTP 服务和信号退出流程。
- 安全绕过：在 V4L2 ioctl 前根据 sysfs driver 跳过非 `uvcvideo` 节点，避免触发已知 PXP 查询风险。
- 资源控制：HTTP socket 同时设置接收和发送超时，客户端断开后工作线程可回收。
- 配置：摄像头选择改为 `auto`，默认 MJPG 640x480@30。
- 构建：Makefile 支持主机和 `CROSS_COMPILE`，生成的 `*.d` 依赖文件加入忽略规则。
- 文档：更新 M2 evidence、阶段总结和学习型网络问题复盘。

## 学习型 Bug / Blocker

- [M2-NET-PROXY-001](../bug_reports/M2-NET-PROXY-001_wsl_proxy_bypasses_rndis.md)：WSL 代理导致板端私网 HTTP 请求超时。
- 复用 [M1-DRV-PXP-001](../bug_reports/M1-DRV-PXP-001_pxp_query_kernel_oops.md) 的经验，M2 在源码中加入 UVC driver 预筛。

## 实际运行事故

无。M2 尚处开发验收阶段，没有正式或长期运行事故。

## 未关闭风险

- 当前仅完成单客户端稳定性验证，未进行多客户端压力测试。
- HTTP 无认证，只适用于受信任局域网，不应暴露到公网。
- `/status` 中 motion/event 字段仍是 M3 前占位值。
- M4 仍需以完整日志重新执行 camera missing、reboot 和进程恢复等正式故障注入。
- 自动执行 shell 当前未包含操作者的 ARM 工具链 PATH；后续自动交叉编译前需显式配置工具链位置。

## 关键决策

- [ADR-0001：UVC first](../architecture/decisions/0001-use-uvc-first.md)
- [ADR-0002：MJPEG over HTTP](../architecture/decisions/0002-use-mjpeg-over-http.md)
- [ADR-0003：最小 daemon](../architecture/decisions/0003-build-minimal-daemon.md)
- [ADR-0006：延后 CameraSource seam](../architecture/decisions/0006-defer-camera-source-seam.md)

## 下一阶段进入条件

1. 文档迁移 PR 先合入 `develop`。
2. M2 代码和本阶段文档 PR 合入 `develop`。
3. 从最新 `develop` 创建 `codex/m3-motion-event`。
4. M3 编码前先根据 i.MX6ULL 性能选择 MJPEG 抽样解码或 YUYV 检测输入。
