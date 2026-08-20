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
| M2-NET-PROXY-001 | M2 | WSL 网络 / HTTP 验证 | 是，直到绕过代理 | SOCKS 代理接管 RNDIS 私网 HTTP 请求，导致 curl 超时 | Resolved | [M2-NET-PROXY-001](M2-NET-PROXY-001_wsl_proxy_bypasses_rndis.md) |
| M3-ENV-PROXY-001 | M3 | WSL 环境 / APT / Proxy | 是，直到显式传递代理 | sudo 清理用户代理环境后，APT 直连 Ubuntu archive 超时 | Resolved | [M3-ENV-PROXY-001](M3-ENV-PROXY-001_sudo_apt_drops_proxy.md) |
| M3-OPS-SSH-001 | M3 | 测试自动化 / SSH / stdin | 是，直到隔离 SSH stdin | SSH 消费管道中的剩余 Bash 脚本，导致后续部署、证据和清理命令未执行 | Resolved | [M3-OPS-SSH-001](M3-OPS-SSH-001_ssh_consumes_piped_script_stdin.md) |
| M3-OPS-WSL-002 | M3 | 测试自动化 / WSL / PowerShell | 是，首次30分钟 runner 未存活 | CRLF 和短生命周期父 shell 导致进度命令异常、后台 runner 退出 | Resolved | [M3-OPS-WSL-002](M3-OPS-WSL-002_powershell_wsl_runner_lifecycle.md) |
| M4-DRV-WIFI-001 | M4 | 内核 / Wi-Fi / SDIO | 是，直到停止自动拉起 `wlan0` | AP6212 DHD/SDIO 打开路径可导致板端用户态失去响应 | 项目范围 Mitigated，底层 BSP 根因待独立深挖 | [M4-DRV-WIFI-001](M4-DRV-WIFI-001_ap6212_dhd_sdio_hang.md) |

## 当前需关注风险

| 风险 | 相关阶段 | 当前缓解方式 |
| --- | --- | --- |
| 板端 Wi-Fi 可能仍不适合作为部署路径 | M1-M5 | 默认使用 USB RNDIS `192.168.7.2` |
| Windows Git 通过 UNC 路径访问 WSL 仓库时可能误判文件状态 | 全阶段 | Git 命令统一在 WSL 内部 `/home/eric/projects/imx6ull-sense-terminal` 下执行 |
| UVC 摄像头可能暴露意外格式或设备编号 | M1 | 先做枚举和一帧采集，再写大段 V4L2 代码 |
| PXP V4L2 查询路径会触发内核 Oops | M1 及后续 | 不把 PXP 输出节点当作摄像头；需要 PXP 时另行定位 BSP 驱动 |
| WSL/Windows 代理或 TUN 可能接管板端私网请求 | M2-M5 | curl 使用 `--noproxy 192.168.7.2`；同时核对 RNDIS 网卡、路由、源地址和 SSH banner |
| sudo 后 APT 可能丢失用户代理 | M3-M5 | 对需要联网的单次 APT 命令显式设置 `Acquire::Proxy` |
| 管道执行的自动化脚本可能被 SSH 消费 stdin | M3-M5 | 不需要远端 stdin 的命令统一使用 `ssh -n` 或 `</dev/null` |
| Windows/WSL 长期 runner 可能受 CRLF 和父 shell 生命周期影响 | M3-M5 | UTF-8 LF stdin；由独立前台 WSL 进程持有完整测试生命周期 |
| AP6212 DHD/SDIO 打开路径可能阻塞板端用户态 | M4 及后续 | 禁用 `autowifi` 自动启动，保持 `wlan0` down，部署和验收使用 USB RNDIS |
