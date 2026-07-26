# 项目文档入口

本目录采用 Docs-as-Code。每类信息只有一个事实源，文档变更与代码一样通过 Git、commit 和 MR/PR 评审。

## 按需求查找

| 我需要 | 入口 | 内容边界 |
| --- | --- | --- |
| 从零理解并运行项目 | [tutorials/](tutorials/) | 连续学习路径 |
| 完成一个具体操作 | [how-to/](how-to/) | 构建、部署、采集和验证步骤 |
| 理解技术原理 | [explanation/](explanation/) | V4L2、MJPEG、motion 原理 |
| 查询参数和接口 | [reference/](reference/) | 配置、HTTP、硬件和开源参考 |
| 查看系统如何组成 | [architecture/](architecture/) | 当前组件、运行时、部署和 ADR |
| 查看下一步执行顺序 | [plans/current.md](plans/current.md) | 唯一当前计划 |
| 查看实际测试输出 | [verification/](verification/) | 各阶段 evidence 和综合测试报告 |
| 查看阶段完成结论 | [stage_summaries/](stage_summaries/) | 验收摘要、决策、风险和下一阶段条件 |
| 复盘开发调试问题 | [bug_reports/](bug_reports/) | 个人学习型 Bug 报告和跨阶段索引 |
| 操作正式服务 | [operations/runbooks/](operations/runbooks/) | 服务安装、检查和恢复 |
| 查看运行事故 | [operations/postmortems/](operations/postmortems/) | 实际影响、时间线、根因和整改 |
| 准备演示或面试 | [presentation/](presentation/) | Demo 和面试材料 |
| 阅读生成报告 | [reports/](reports/) | 可直接打开的 HTML 报告 |

## 唯一事实源规则

- “接下来做什么”只写入 [当前计划](plans/current.md)。
- “为什么这样选择”写入 [ADR](architecture/decisions/)。
- “怎样操作”写入 `how-to/` 或正式服务的 `operations/runbooks/`。
- “命令实际输出了什么”写入 `verification/evidence/`。
- “阶段是否完成”由 `stage_summaries/` 给出结论。
- Bug 报告和阶段总结链接 evidence，不复制整段原始日志。

## 两类问题记录

`bug_reports/` 用于搭建、开发和调试中的个人学习复盘，强调错误判断、正确排查顺序和可复用经验。

`operations/postmortems/` 只用于正式或长期运行中的实际事故，强调影响、时间线、恢复和永久整改。一个事件同时具有两种价值时分别记录并互相链接，不复制整份内容。

## 文档状态

- M0、M1、M2 已完成，证据和阶段总结已保存。
- M3 功能和板端验收已完成，阶段总结待随代码提交。
- M4、M5 尚未开始。
- 网站源代码不放在 `docs/`；生成 HTML 保留在 `reports/`，源代码位于 `tools/report-sites/`。

文档架构决定见 [ADR-0005](architecture/decisions/0005-use-docs-as-code-structure.md)。
