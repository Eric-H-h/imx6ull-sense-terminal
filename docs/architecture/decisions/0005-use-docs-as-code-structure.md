# ADR-0005：文档采用单一事实源分类

## Status

Accepted

## Context

早期文档按 `00` 到 `05` 里程碑编号保存在 `docs/` 根目录，后来又建立 tutorial、how-to、reference、explanation、architecture 和 verification 目录，造成旧架构和新架构并存。

## Considered Options

- 继续保留两套入口。
- 只保留按里程碑编号的文档。
- 按内容用途分类，同时保留阶段总结、学习型 Bug 和运行事故三类项目记录。

## Decision

长期文档按用途建立唯一事实源：

- `plans/` 保存当前执行顺序。
- `architecture/` 和 ADR 保存系统结构与决策原因。
- `tutorials/`、`how-to/`、`explanation/`、`reference/` 保存长期知识。
- `verification/evidence/` 保存真实验收证据。
- `stage_summaries/` 保存阶段结论。
- `bug_reports/` 保存个人学习型调试复盘。
- `operations/postmortems/` 保存正式运行事故。

旧根文档迁移后删除，内部链接一次性更新。Git 历史负责保存旧路径，不维护第二套兼容内容。

## Consequences

### Positive

- 每类信息只有一个维护位置。
- 阶段总结不再复制完整日志。
- 当前计划不再混入大段原理和操作手册。

### Negative

- 需要一次性更新现有内部链接。
- 已经熟悉旧路径的 session 必须重新读取 `docs/README.md`。

## Verification

迁移完成后检查旧路径引用、Markdown 相对链接、当前阶段和 Git 状态描述。
