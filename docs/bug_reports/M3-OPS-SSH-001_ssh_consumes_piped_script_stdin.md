# Bug M3-OPS-SSH-001：SSH 消费管道脚本的标准输入

## 状态

- 归属阶段：M3 — JPEG benchmark 与板端并行验证
- 类型：测试自动化 / SSH / Shell stdin
- 状态：Resolved
- 发现日期：2026-07-23
- 修复方式：自动化中的 SSH 使用 `ssh -n` 或显式重定向 `</dev/null`

## Environment

- 开发主机：Windows + WSL Ubuntu
- 执行方式：Base64 编码的多行脚本通过管道交给 WSL Bash
- 远端：EBF6ULL S1 Pro，Debian 10，地址 `192.168.7.2`
- 传输通道：USB RNDIS + OpenSSH

## Symptom

两次多阶段脚本都在第一条 SSH 命令之后提前结束：

1. SSH 创建板端部署目录成功，但后续 SCP 没有执行，板端目录为空。
2. 30 秒并行 benchmark 成功，但后续 `/status`、curl 结果、dmesg 和 daemon 清理没有执行。

外层执行单元仍返回 exit 0，容易误认为整段脚本已经完成。

## Reproduction

当脚本本身来自 stdin：

```sh
printf '%s' "$SCRIPT" | base64 -d | bash
```

脚本内直接执行：

```sh
ssh user@board 'remote command'
scp file user@board:/tmp/
echo "deploy_exit:0"
```

可观察到远端 SSH 命令输出，但后续 SCP 和标记输出缺失。

## Evidence

首次部署后板端目录为空。单独重传后：

```text
scp_exit:0
imx6ull-sense 29872 bytes
sha256: cfa3f2d9e4eed3441744158c83702c2a33ec97179639be087eddb9e057726edd
```

并行测试同样在 SSH benchmark 返回后停止，未输出预期的 `STATUS AFTER BENCH` 和清理标记。

## Root cause

`ssh` 默认继承并读取标准输入。当前 Bash 正在从同一条管道读取脚本文本，因此 SSH 把尚未被 Bash 解析的后续脚本内容作为自己的 stdin 消费。

这不是 SCP、USB RNDIS、板端权限或 SSH 服务故障。根因位于本地自动化的 stdin 所有权。

## Fix/workaround

不需要远端 stdin 的 SSH 命令统一使用：

```sh
ssh -n user@board 'remote command'
```

等价方式：

```sh
ssh user@board 'remote command' </dev/null
```

对于确实需要传递远端脚本的场景，应先把脚本完整封装为 SSH 命令参数或独立文件，不能让 SSH 与本地 Bash 争用同一个脚本输入流。

## Verification

加入 `ssh -n` 后，同一补证脚本连续完成：

```text
STATUS AFTER BENCH
no_new_dmesg_lines
daemon_stopped:yes
post_stop_curl_exit:7
```

板端 SHA-256 与 WSL 临时 ARM binary 一致，证明重传完整。

## Impact on plan

- 没有修改产品代码。
- 没有影响 benchmark 的 30 秒核心结果。
- curl 最终下载字节统计没有保留，因此 evidence 明确记录该限制。
- 延长 B3 证据收集和临时 daemon 清理时间。

## 我当时的错误判断

看到板端目录为空时，先怀疑 SCP 没有成功、目标路径错误或网络中断。实际上 SSH 创建目录已经成功，而紧随其后的本地命令根本没有被 Bash 执行。

只看外层 exit code 也会误判，因为最后实际执行的是成功的 SSH，而不是原计划最后的验证命令。

## 正确排查顺序

1. 检查每个阶段是否都有独立完成标记，不能只看总 exit code。
2. 对比脚本中最后出现的输出与预期下一条命令。
3. 确认 SCP 是否真正启动，而不是先排查远端权限。
4. 检查脚本的输入来源以及 SSH 是否继承 stdin。
5. 使用 `ssh -n` 重试。
6. 重传后用文件大小和 SHA-256 验证。
7. 补做状态、日志和清理验证。

## 可复用经验

- 从 stdin 执行的自动化脚本中，SSH 应默认使用 `-n`，除非明确需要远端读取 stdin。
- 多阶段脚本应为每一步输出完成标记和 exit code。
- “脚本返回 0”不等于“计划中的每条命令都执行过”。
- 部署后必须检查远端文件和哈希，不能只相信 SCP 所在脚本的总状态。

## 面试可讲内容

板端 benchmark 自动化曾出现 SSH 成功但后续 SCP、状态采集和清理未执行的问题。通过对照输出边界发现，脚本本身由管道喂给 Bash，而 SSH 继承 stdin 后消费了剩余脚本文本。使用 `ssh -n` 隔离 stdin，并增加逐阶段标记和 SHA-256 验证后，部署和清理链路完整通过。

## Follow-up

1. 后续板端自动化 SSH 默认使用 `-n`。
2. 关键部署步骤保留独立 exit code、文件大小和哈希。
3. M3 evidence 保留 curl 字节统计缺失说明，不以推断值替代。