# M5：测试报告、演示和发布包装

## 阶段状态

- 状态：Completed（待提交）
- 完成日期：2026-08-20
- 分支：`codex/m5-docs-demo`
- Commit：本总结随文档提交

## 阶段目标

在不新增 daemon 功能的前提下，把 M0-M4 已验收事实整理成综合测试报告、Demo 脚本、面试/简历材料和与最终实现一致的 README/架构文档，为合入 `develop` 之后的 `v0.1-mvp` 发布做准备。

## 验收结果

| 验收项 | 结果 | 证据 |
| --- | --- | --- |
| 综合测试报告 | Passed | [test-report.md](../verification/test-report.md)，数值均可追溯 |
| README / 架构图 | Passed | 根 README 与 [overview](../architecture/overview.md) |
| 三分钟 Demo | Passed | [demo-script.md](../presentation/demo-script.md) |
| 面试问答与简历要点 | Passed | [interview-qa.md](../presentation/interview-qa.md)、[resume.md](../presentation/resume.md) |
| 已知限制与 fallback | Passed | 测试报告与 README |
| HTTP `/status` 字段对齐 | Passed | [http-api.md](../reference/http-api.md) 含 health / camera / event_log |
| 发布合并与标签 | 未在本阶段执行 | M5 PR 合入 `develop` 后再做 `develop -> main` 和 `v0.1-mvp` |

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
- 文档：新增测试报告终稿、Demo 脚本、简历要点、M5 阶段总结；对齐 HANDOFF、当前计划、README、架构图、HTTP API、how-to 和过期 evidence 状态。

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
- 发布标签不在文档分支上提前打；先把 M5 合入 `develop`。

## 下一阶段进入条件

1. 审查 M5 文档清单，按明确文件提交 `codex/m5-docs-demo`。
2. 推送并创建面向 `develop` 的 PR。
3. PR 合入后本地 fast-forward 到最新 `develop`。
4. 执行 `develop -> main` 发布合并并创建 `v0.1-mvp`。
5. 需要演示录像时，按 Demo 脚本由操作者录制，不提交视频文件。
