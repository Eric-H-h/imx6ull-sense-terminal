# M3-OPS-WSL-002：PowerShell 到 WSL 的脚本换行与后台生命周期

## Bug ID

`M3-OPS-WSL-002`

## 阶段

M3 Motion Event 验收自动化。

## Environment

- Windows PowerShell 调用 Ubuntu WSL。
- 复杂命令通过 `wsl.exe -d Ubuntu -- bash -s` 执行。
- 测试需要持续 30 分钟并周期采集 HTTP、SSH 和进程数据。

## Symptom

1. PowerShell here-string 通过管道传入 Bash 后，部分末行带 `\r`，出现 `$'command\r': command not found` 或路径末尾包含 CR。
2. 在短生命周期 `bash -s` 中启动的后台 subshell 随父 shell 退出，runner PID 消失，首个样本未写入。

## Reproduction

- 将带 Windows CRLF 的 here-string 直接管道给 `wsl ... bash -s`。
- 在该非交互 shell 内用 `( long_running_task ) &` 启动 30 分钟任务后立即退出父 shell。

## Evidence

首次 runner PID `350532` 很快消失，`runner.log` 为空且没有第 0 个样本。进度查看还出现：

```text
cat: '/tmp/.../progress.log'$'\r': No such file or directory
```

改用持久 runner 后，7 个采样点全部 `status_rc=0 ssh_rc=0`，最终 `m3_d3_verdict: PASS`。

## Root cause

PowerShell 字符串和原生进程 stdin 的换行边界没有统一为 LF；同时，短生命周期 WSL shell 不是长期任务的可靠 supervisor，后台子进程不能假设会在父 shell 退出后继续存活。

## Fix / workaround

- 写入 runner stdin 前显式移除 CR，并使用 UTF-8 no BOM。
- 使用 `Start-Process -WindowStyle Hidden` 启动独立 `wsl ... bash -s`，让 Bash 在前台持有整个测试生命周期。
- 不需要远端 stdin 的 SSH 命令继续使用 `ssh -n`。
- 简单只读查询可以直接调用 WSL 命令，避免不必要的多层 shell。

## Verification

新的 runner 连续运行 30 分钟，7/7 个 HTTP 和 SSH 样本成功；PID、RSS、VSZ 和线程数稳定，最终验收 PASS。

## Impact on plan

首次计时未生效，需要重新开始30分钟测试；没有修改产品代码，也没有污染正式证据。

## Status

Resolved。

## 我当时的错误判断

认为 `trap '' HUP` 足以保证非交互 WSL shell 的后台任务存活，并低估 PowerShell 管道重新编码换行的影响。

## 正确排查顺序

1. 先确认产品 daemon 和网络门禁。
2. 检查 runner PID、首个样本和 runner stderr。
3. 区分产品进程退出与测试编排进程退出。
4. 固定 stdin 编码、换行和父进程生命周期。
5. 从第 0 分钟重新开始，不拼接不完整证据。

## 可复用经验

跨 Windows/WSL 的长期任务需要明确的进程所有者、stdin 策略和日志文件；“命令已返回成功”不等于后台任务仍在运行。

## 从现象到验证

可以说明如何把产品故障与测试框架故障分离，并通过 PID、首样本、退出码和日志建立门禁，避免把无效30分钟等待写成稳定性证据。

## Follow-up

后续长期板端验证优先固化为仓库脚本或正式 runbook；M4 的长期服务生命周期由 systemd 管理，不复用临时后台 shell。
