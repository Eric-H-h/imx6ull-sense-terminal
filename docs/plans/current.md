# i.MX6ULL Sense Terminal 当前执行计划

## 状态

- 当前里程碑：M3，Motion Event 与 JSONL，验收完成并等待提交和合并。
- 当前分支：`codex/m3-motion-event`。
- 已完成：M0、M1、M2；M3 功能与板端验收已完成。
- 当前阶段状态：Completed；M3 PR 合入 `develop` 前不进入 M4 实现。
- MVP 路线：USB UVC first；OV5640 为可选增强项。
- 目标发布时间：2026 年 7 月底前完成 MVP。

本文件只维护执行顺序、验收条件、范围和协作约束。系统结构见 [architecture](../architecture/)，设计原因见 [ADR](../architecture/decisions/)，操作命令见 [how-to](../how-to/)。

## 项目目标

在 EBF6ULL S1 Pro / i.MX6ULL 上完成一个可上板、可浏览器演示、可产生事件、可长期运行并有故障证据的嵌入式 Linux 服务：

```text
UVC/V4L2
  -> MJPEG HTTP
  -> motion event
  -> JSONL log
  -> systemd
  -> fault injection
```

## 范围

MVP 必须完成：

- USB UVC 摄像头真实采集。
- `/`、`/stream`、`/status`。
- 简单且可调的 motion event。
- JSONL 事件日志。
- systemd 启动、重启和开机自启。
- kill、reboot、camera missing、bad config 测试。
- README、阶段总结、测试报告和演示材料。

MVP 不包含：

- 录像和视频文件管理。
- H.264/RTSP。
- 用户认证、公网服务或云平台。
- AI 目标检测。
- HDMI 主线。
- OV5640 必选适配。

## 里程碑总览

| 阶段 | 目标 | 状态 | 主要证据 |
| --- | --- | --- | --- |
| M0 | 主机、交叉编译、板端部署闭环 | Completed | [M0 summary](../stage_summaries/M0_environment_and_board_baseline.md) |
| M1 | UVC 枚举、格式和真实首帧 | Completed | [M1 summary](../stage_summaries/M1_usb_uvc_camera_capture.md) |
| M2 | 浏览器 MJPEG 和状态接口 | Completed | [M2 summary](../stage_summaries/M2_mjpeg_browser_stream.md) |
| M3 | motion event 与 JSONL | Completed | [M3 summary](../stage_summaries/M3_motion_event_logging.md) |
| M4 | systemd 与故障注入 | Planned | [M4 evidence](../verification/evidence/M4_fault_injection.md) |
| M5 | 测试报告、演示和发布 | Planned | [test report](../verification/test-report.md) |

## M2：已完成内容

### 目标

把 UVC MJPEG、共享最新帧和 HTTP 模块修正并验收到可演示状态。该目标已于 2026-07-18 完成。

### 当前实现

- 动态识别 `uvcvideo + Video Capture + Streaming` 节点。
- V4L2 MJPEG MMAP 采集。
- 共享最新 JPEG 和状态快照。
- `GET /`、`GET /stream`、`GET /status`。
- 摄像头缺失 degraded 和自动重试。

上述能力已完成代码审查、板端验证、浏览器验证和稳定性测试，结果见 M2 evidence 与阶段总结。

M2 的输入实现可以保持 UVC/MJPEG 专用，不要求为 OV5640 提前重构。架构只要求未来帧模型能够表达 pixel format、宽高、stride/bytesused、sequence 和 timestamp；第二个真实输入出现后再提取 CameraSource seam，见 ADR-0006。

### 执行顺序

1. 完成并发、socket、V4L2 buffer、配置边界和资源释放审查。
2. 修复阻塞验收的问题，并进行主机干净构建。
3. 进行 ARM 交叉编译，确认产物架构。
4. 通过 USB RNDIS 部署到开发板。
5. 验证 `/`、`/status` 和 `/stream` 的状态码、头部、boundary、长度和 JPEG 数据。
6. 使用 Windows 浏览器验证动态画面、刷新、关闭和重新连接。
7. 验证摄像头缺失和重新插入的 degraded/recovery 行为。
8. 单客户端运行 30 分钟，记录 FPS、CPU、RSS 和内核日志。
9. 更新 M2 evidence；meaningful bug 写独立报告并更新索引。
10. 写 M2 阶段总结，确认提交范围后再 commit 和开 MR/PR。

### M2 验收

- 主机和 ARM 构建通过。
- 浏览器画面持续变化。
- `/status` 与实际摄像头状态一致。
- multipart boundary、`Content-Length` 和 JPEG 字节正确。
- 客户端刷新、断开和重连不会使服务失效。
- 30 分钟运行无不可解释崩溃、失控内存增长或内核错误。
- M2 evidence 和阶段总结完整。

## M3：Motion Event（Completed）

M3 保留 M2 UVC MJPEG pass-through，通过 latest JPEG 以 3 FPS 抽样、libjpeg `1/4` scaling 解码到 160x120 grayscale，再执行逐像素帧差、5% threshold、1500 ms cooldown 和 JSONL 追加。

已完成：

- 可解释的 changed ratio motion score。
- 可配置 sample FPS、scale、两层 threshold 和 cooldown。
- JSONL 事件追加写入和逐行解析。
- `/status` 输出真实 motion state、score、采样 FPS 和 event count。
- 静止 0 误报、10/10 挥手、持续运动 cooldown、摄像头恢复和 30 分钟并行稳定性证据。
- 主机 `make verify`、ARM 固定构建脚本和 GCC analyzer。

详细证据见 [M3 evidence](../verification/evidence/M3_motion_event.md)。M3 PR 合入 `develop` 前不开始 M4 源码或 systemd 修改。

## M4：systemd 与故障注入

交付：

- 安装并启用 `imx6ull-sense.service`。
- `kill -9` 后自动恢复。
- reboot 后自动启动。
- camera missing 显示 degraded，恢复策略有证据。
- bad config 以明确日志和非零状态 fail safe。
- 形成 runbook、故障注入 evidence 和阶段总结。

## M5：发布包装

交付：

- 综合测试报告。
- README 与架构图反映最终实现。
- 三分钟 Demo 流程和面试问答。
- 已知限制和 fallback 说明。
- `develop -> main` release MR/PR。
- 发布标签 `v0.1-mvp`。

## Fallback

- 某个 UVC 摄像头格式或 BSP 不兼容时，换另一款 Linux 免驱 UVC 摄像头。
- MJPEG 性能不足时按顺序降低分辨率、FPS 和 JPEG 质量。
- OV5640 只在 MVP 闭环后投入；连续两个晚上不通则停止。
- HDMI、AI、录像和 OTA 不得占用 M2-M4 主线时间。
- 如果浏览器 stream 未闭环，暂停所有增强项。

## Git 与文档约束

- 完整 Git 规则见 [Git 工作流](../how-to/git-workflow.md)。
- 实际代码修改在对应里程碑分支进行。
- 每个可验证小闭环形成独立 commit；不把未验证代码、构建产物和无关文档混在一起。
- 一个里程碑达到验收标准后开一个 MR/PR 合并回 `develop`。
- 每阶段必须有 evidence 和 stage summary。
- meaningful bug 必须有独立 bug report 和索引项。
- 正式运行事故才进入 postmortem。
- 路线改变时更新本计划，并新增或更新 ADR。

## 阶段完成文档检查

每个阶段关闭前确认：

- `verification/evidence/MN_*.md` 有真实命令、输出和结果。
- `stage_summaries/MN_*.md` 有验收结论、关键变化、风险、ADR 和下一阶段条件。
- Bug 报告及索引已更新，或明确写“无”。
- Runtime Incidents 已链接 postmortem，或明确写“无”。
- 当前计划、README 和 HANDOFF 的状态一致。
