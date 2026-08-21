# Git 工作流

## Git 命令执行位置

仓库在 WSL 中克隆和使用。所有 Git 命令在 WSL 的仓库根目录执行。Windows Git 通过 UNC 路径访问时可能错误报告权限或 modified 状态。

```sh
git rev-parse --show-toplevel
```

应得到当前克隆路径，而不是 `\\wsl.localhost\...`。

## 每次开始工作

```sh
git rev-parse --show-toplevel
git status --short --branch
git log --oneline -5
```

如果存在来源不明的改动，不删除、不覆盖、不切分支，先确认来源。

## 分支模型

- `develop`：日常集成分支。
- `main`：稳定发布分支。
- 不要把功能分支直接合并到 `main`。

本仓库里程碑曾使用 `codex/mN-*`（例如 `codex/m4-systemd-fault`）。新工作从最新 `develop` 拉出说明性分支即可。

M0–M5 合入 `develop` 之后，再执行 `develop -> main` 并创建 `v0.1-mvp`。

只有上一里程碑已验收、已合并且工作区干净时，才从 `develop` 创建下一分支。

## 检查和暂存

```sh
git status --short
git diff --stat
git diff
```

按明确文件清单暂存，不默认执行 `git add .`：

```sh
git add path/to/file
git diff --cached --stat
git diff --cached
```

误暂存时只撤销暂存：

```sh
git restore --staged path/to/file
```

## Commit

每个 commit 对应一个可验证结果，并有命令、输出、日志或文档证据。不要把未验证代码、构建产物、无关文件和真实测试日志混在一个大 commit 中。

常用类型：

```text
feat: 用户可见能力
fix: 已确认问题修复
docs: 文档、日志或阶段总结
test: 测试与验证
chore: 构建、脚本、systemd 或仓库维护
```

## Push 和 MR/PR

里程碑达到验收标准、阶段总结和 Bug 记录完整、提交范围清楚后，再 push 并创建 MR/PR 合并回 `develop`。不要把工作分支直接合并到 `main`。

M0-M5 完成后，执行 `develop -> main` 发布合并并创建 `v0.1-mvp` 标签。

## 禁止事项

- 不使用 Windows Git 操作 WSL UNC 仓库。
- 不使用 `git reset --hard` 或 `git checkout --` 清理来源不明的改动。
- 不在有未确认改动时切换里程碑分支。
- 不无检查地执行 `git add .`。
- 不提交 secrets、编译产物、临时目录或原始大体积抓帧。
- 不在缺少板端证据时宣称里程碑完成。
