# Bug 复盘索引

本目录用于保存每个 meaningful bug 或 blocker 的独立复盘文档。

## 记录规则

任何影响阶段推进的 bug 都必须有独立复盘。对应阶段总结必须链接该 bug 报告，本索引也必须同步登记。

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
- 后续经验或跟进项

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
