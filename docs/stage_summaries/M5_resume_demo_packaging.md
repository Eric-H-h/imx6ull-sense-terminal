# M5：测试报告、演示和发布包装

## 阶段状态

- 状态：Completed；其后已合入 `main`，标签 `v0.1-mvp`，仓库 Public
- 完成日期：2026-08-20
- 分支：`codex/m5-docs-demo`
- Commit：本总结随文档提交

## 阶段目标

在不新增 daemon 功能的前提下，把 M0-M4 已验收事实整理成综合测试报告、演示步骤、设计问答，以及面向公开复刻的 README / LICENSE，为合入 `develop` 之后的 `v0.1-mvp` 发布做准备。

## 验收结果

| 验收项 | 结果 | 证据 |
| --- | --- | --- |
| 综合测试报告 | Passed | [test-report.md](../verification/test-report.md)，数值均可追溯 |
| README / 架构图 | Passed | 根 README 与 [overview](../architecture/overview.md) |
| 演示步骤 | Passed | [demo-script.md](../presentation/demo-script.md) |
| 设计问答 | Passed | [design-faq.md](../presentation/design-faq.md) |
| 公开说明 | Passed | MIT [LICENSE](../../LICENSE)、[CONTRIBUTING](../../CONTRIBUTING.md)；根目录无 HANDOFF，无简历入口 |
| 已知限制与 fallback | Passed | 测试报告与 README |
| HTTP `/status` 字段对齐 | Passed | [http-api.md](../reference/http-api.md) 含 health / camera / event_log |
| 发布合并、标签与 Public | 未在本阶段执行 | M5 PR 合入 `develop` 后再做 `develop -> main`、`v0.1-mvp`，然后改为 Public |

## 板端与运行事实

M5 没有新的板端测试。沿用：

- 开发板/内核：EBF6ULL S1 Pro，Linux 4.19.35-imx6，systemd 241。
- 网络路径：USB RNDIS `192.168.7.2`。不要启用 `autowifi.service`。
- 视频：MJPG 640x480@30。
- Motion：3 FPS，160x120 grayscale，像素差 25，变化比例 5%，cooldown 1500 ms。
- 正式路径：`/usr/local/bin/imx6ull-sense`，`/etc/imx6ull-sense/config.json`，`/var/lib/imx6ull-sense/events.jsonl`。

性能与故障注入结论见 [综合测试报告](../verification/test-report.md)。

## 关键变化

- 代码：无。
- 配置：无。
- 文档：测试报告终稿、演示步骤、设计问答、M5 阶段总结；README 改为公开复刻说明；新增 LICENSE 与 CONTRIBUTING；移除根目录 HANDOFF 和简历要点。

## 学习型 Bug / Blocker

无新增。M5 复用既有索引，不把旧报告再抄一份。

## 实际运行事故

无。

## 未关闭风险

- 演示视频尚未录制；仓库只保存脚本。
- 单客户端稳定性已验证，多客户端压力仍未做。
- HTTP 无认证，只适用于可信局域网。
- AP6212/DHD 仍不适合作为部署路径。
- JSONL 不 `fsync`；`event_count` 是进程内计数。

## 关键决策

- 延续既有 ADR，M5 不新增架构决定。
- 一个仓库同时服务复刻和学习；公开门面不写简历或面试入口。
- 发布标签不在文档分支上提前打；先把 M5 合入 `develop`，再改 GitHub 可见性。

## 下一阶段进入条件

1. 审查 M5 文档清单，按明确文件提交 `codex/m5-docs-demo`。
2. 推送并创建面向 `develop` 的 PR。
3. PR 合入后本地 fast-forward 到最新 `develop`。
4. 执行 `develop -> main` 发布合并并创建 `v0.1-mvp`。
5. 将 GitHub 仓库改为 Public。
6. 需要演示录像时，按演示步骤由操作者录制，不提交视频文件。
