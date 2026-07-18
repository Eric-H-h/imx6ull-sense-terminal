# 架构文档

本目录描述系统当前边界和已经明确标注的目标结构。

- [系统总览](overview.md)：项目边界、当前链路和目标闭环。
- [组件](components.md)：daemon 模块和依赖方向。
- [运行时](runtime.md)：线程、最新帧、故障和退出时序。
- [部署](deployment.md)：WSL、开发板、RNDIS 和 M4 目标路径。
- [架构决策](decisions/)：长期设计选择及其原因。

架构文档描述“系统如何组成”。执行顺序写入 [`plans/current.md`](../plans/current.md)，真实测试结果写入 [`verification/`](../verification/)。尚未实现的内容必须标记为 Planned。
