# 如何参与

先在克隆后的仓库根目录确认主机构建和测试通过：

```sh
make -C app/daemon verify
```

交叉编译和上板步骤见 [从零理解并运行项目](docs/tutorials/getting-started.md)。Git 习惯见 [Git 工作流](docs/how-to/git-workflow.md)。

## 文档

文档按用途分类，入口是 [docs/README.md](docs/README.md)。同类事实只写一处：计划写在 `docs/plans/current.md`，原因写在 ADR，操作写在 how-to 或 runbook，命令输出写在 evidence。

## 提交

- 按明确文件清单暂存，不要使用 `git add .`。
- 不要提交 `secrets.local.md`、构建产物、`tmp/`、原始抓帧或演示视频。
- 改行为时附上板上现象、`/status` 或测试命令；问用法时说明开发板、摄像头和主机环境。
