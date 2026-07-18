# Bug 复盘索引

本目录用于保存搭建、开发和调试过程中值得再次阅读的 Bug 或 blocker，目标是个人学习、经验复用和面试复盘，不是实际运行事故档案。

正式运行后造成服务中断、功能不可用、数据丢失或恢复失败的事故，记录到 [`../operations/postmortems/`](../operations/postmortems/)；两类记录不得用同一份文档混写。

## 记录规则

任何影响阶段推进或具有明确学习价值的 Bug 都必须有独立复盘。对应阶段总结必须链接该 Bug 报告，本索引也必须同步登记。命令输错、一次性工具超时或没有复用价值的操作失误不创建报告。

每份 bug 报告至少包含：

- bug ID
- 归属阶段
- 环境信息
- 现象
- 复现步骤或触发条件
- 证据和关键日志
- 根因
- 修复或绕过方式
- 验证结果
- 对计划的影响
- 状态
- 我当时的错误判断
- 正确排查顺序
- 可复用经验
- 面试可讲内容
- 后续跟进项

新报告使用 [`_template.md`](_template.md)。判断标准不是“是否出现错误”，而是“以后是否值得再次阅读”。

## 命名规则

Bug ID 和文件名使用以下形态：

```text
<STAGE>-<CATEGORY>-<NNN>_<short_slug>.md
```

示例：

```text
M0-NET-WIFI-001_usb_rndis_board_link.md
M1-CAM-UVC-001_video_device_not_created.md
M4-SVC-SYSTEMD-001_restart_policy_not_applied.md
```

如果根因尚未确认，也要先创建报告，状态标记为 `Open` 或 `Mitigated`，记录当前假设，后续验证后再更新。

## 与运行事故的边界

| 情况 | 记录位置 |
| --- | --- |
| 开发板接线、驱动、构建、网络、V4L2 调试踩坑 | `docs/bug_reports/` |
| 已部署服务在正式或长期运行中产生实际影响 | `docs/operations/postmortems/` |
| 运行事故同时暴露了值得学习的技术问题 | 两边分别记录并互相链接 |

## 跨阶段汇总

| Bug ID | 阶段 | 类型 | 是否阻塞 | 摘要 | 状态 | 报告 |
| --- | --- | --- | --- | --- | --- | --- |
| M0-NET-WIFI-001 | M0 | 网络 / 部署 | 是，直到 USB RNDIS 修复 | Wi-Fi 路径不稳定；USB RNDIS 初期受数据线和 Windows 驱动绑定影响 | M0 已解决 | [M0-NET-WIFI-001](M0-NET-WIFI-001_usb_rndis_board_link.md) |
| M1-DRV-PXP-001 | M1 | 内核 / V4L2 / PXP | 否，UVC 路径可绕过 | 查询 PXP Video Output 节点时触发内核 Oops | Mitigated，根因待确认 | [M1-DRV-PXP-001](M1-DRV-PXP-001_pxp_query_kernel_oops.md) |

## 当前需关注风险

| 风险 | 相关阶段 | 当前缓解方式 |
| --- | --- | --- |
| 板端 Wi-Fi 可能仍不适合作为部署路径 | M1-M5 | 默认使用 USB RNDIS `192.168.7.2` |
| Windows Git 通过 UNC 路径访问 WSL 仓库时可能误判文件状态 | 全阶段 | Git 命令统一在 WSL 内部 `/home/eric/projects/imx6ull-sense-terminal` 下执行 |
| UVC 摄像头可能暴露意外格式或设备编号 | M1 | 先做枚举和一帧采集，再写大段 V4L2 代码 |
| PXP V4L2 查询路径会触发内核 Oops | M1 及后续 | 不把 PXP 输出节点当作摄像头；需要 PXP 时另行定位 BSP 驱动 |
