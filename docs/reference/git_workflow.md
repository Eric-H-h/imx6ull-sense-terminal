# Git 操作手册

## 1. Git 命令在哪里执行

仓库实际位于 WSL：

    /home/eric/projects/imx6ull-sense-terminal

所有 Git 命令默认在 WSL 内执行。不要使用 Windows Git 直接操作 UNC 路径：

    \\wsl.localhost\Ubuntu\home\eric\projects\imx6ull-sense-terminal

原因是 Windows Git 通过 UNC 路径读取 WSL 仓库时，可能错误报告文件权限或 modified 状态。

从 Windows PowerShell 进入 WSL：

    wsl -d Ubuntu
    cd /home/eric/projects/imx6ull-sense-terminal

如果必须在 PowerShell 中执行单条 Git 命令：

    wsl -d Ubuntu -- bash -lc "cd /home/eric/projects/imx6ull-sense-terminal && git status --short --branch"

## 2. 每次开始工作

先确认仓库位置、当前分支和工作区：

    cd /home/eric/projects/imx6ull-sense-terminal
    git rev-parse --show-toplevel
    git status --short --branch
    git log --oneline -5

当前 M1 应显示：

    ## codex/m1-uvc-capture

如果出现不属于当前任务的改动，不要删除、覆盖或直接切分支，先确认改动来源。

## 3. 里程碑分支

develop 是集成分支，main 只保存稳定发布版本。每个里程碑从最新的 develop 创建独立分支。

开始下一里程碑前：

    git switch develop
    git status --short --branch
    git switch -c codex/m2-mjpeg-stream

计划分支：

    codex/m1-uvc-capture
    codex/m2-mjpeg-stream
    codex/m3-motion-event
    codex/m4-systemd-fault
    codex/m5-docs-demo

只有在工作区干净、上一里程碑已经验收并合并后，才能从 develop 创建下一分支。不要在工作做到一半时才切分支。

## 4. 查看和暂存改动

提交前先检查：

    git status --short
    git diff --stat
    git diff

优先暂存明确的文件，不默认使用 git add .：

    git add docs/02_camera_capture.md
    git add docs/stage_summaries/M1_usb_uvc_camera_capture.md

检查真正要进入提交的内容：

    git diff --cached --stat
    git diff --cached

如果暂存了错误文件，只撤销暂存，不删除文件内容：

    git restore --staged path/to/file

## 5. Commit

只有完成一个可验证闭环后才提交。每个提交应有对应的测试命令、输出或文档证据。

    git commit -m "docs: record uvc camera enumeration"
    git status --short --branch
    git log -1 --oneline

常用提交类型：

    feat: 新增用户可见能力
    fix: 修复已确认问题
    docs: 更新日志、阶段总结或 Bug 报告
    test: 增加或更新测试
    chore: 构建、脚本、systemd 或仓库维护

不要把未验证代码、无关文件和真实测试日志混在一个大提交中。

## 6. 当前 M1 的推荐提交流程

完成 UVC 枚举和第一帧采集后：

    git status --short
    git diff -- docs/02_camera_capture.md
    git add docs/02_camera_capture.md
    git add docs/stage_summaries/M1_usb_uvc_camera_capture.md
    git diff --cached
    git commit -m "docs: record m1 uvc capture baseline"
    git status --short --branch

如果 M1 出现 meaningful bug，还要暂存独立 Bug 报告和索引：

    git add docs/bug_reports/M1-CAM-UVC-001_video_device_not_created.md
    git add docs/bug_reports/README.md

阶段总结和 Bug 文档均完成后再关闭 M1。

## 7. Push 和 MR/PR

仓库配置 remote 后，先检查目标地址：

    git remote -v

首次推送当前分支：

    git push -u origin codex/m1-uvc-capture

一个里程碑达到验收条件、文档和 Bug 记录完整、工作区干净后，再创建 MR/PR 合并回 develop。不要把工作分支直接合并到 main。

MVP 完成后才执行 develop -> main 的发布合并，并创建 v0.1-mvp 标签。

## 8. 禁止事项

- 不在 UNC 路径下使用 Windows Git 操作本仓库。
- 不使用 git reset --hard 或 git checkout -- 清理不明改动。
- 不在有未确认改动时切换里程碑分支。
- 不使用 git add . 无检查地暂存全部文件。
- 不提交 secrets.local.md、编译产物或敏感信息。
- 不在没有板端验证证据时宣称里程碑完成。
