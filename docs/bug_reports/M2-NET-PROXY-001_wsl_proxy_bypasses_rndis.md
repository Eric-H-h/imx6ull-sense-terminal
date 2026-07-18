# Bug M2-NET-PROXY-001：WSL 代理绕过 USB RNDIS 私网请求

## 状态

- 归属阶段：M2 — MJPEG 浏览器推流
- 类型：开发主机网络 / HTTP 验证
- 状态：Resolved
- 发现日期：2026-07-18
- 修复方式：访问板端私网地址时使用 `--noproxy`，并配置 `NO_PROXY`

## Environment

- 开发主机：Windows + WSL Ubuntu
- 板端地址：`192.168.7.2`
- 板端链路：USB RNDIS
- 服务地址：`http://192.168.7.2:8080`
- curl：8.5.0
- WSL 环境变量：`http_proxy=socks5h://127.0.0.1:10808`

## Symptom

板端 daemon 正常运行并监听 `0.0.0.0:8080`，但 WSL 中执行 `curl http://192.168.7.2:8080/status` 长时间无输出，最终在 5 秒后超时。

## Reproduction

```sh
curl -v --connect-timeout 3 --max-time 5 \
  http://192.168.7.2:8080/status
```

## Evidence

```text
Uses proxy env variable http_proxy == 'socks5h://127.0.0.1:10808'
Trying 127.0.0.1:10808...
SOCKS5 connect to 192.168.7.2:8080
Operation timed out after 5002 milliseconds with 0 bytes received
curl: (28) Operation timed out
```

同期板端证据：

```text
./imx6ull-sense -c config.json
LISTEN 0 8 0.0.0.0:8080 0.0.0.0:*
```

这说明进程和监听 socket 正常，超时发生在 WSL 请求路径。

## Root cause

`curl` 自动读取 `http_proxy`，把本应直接走 USB RNDIS 的 `192.168.7.2` 请求发送给本地 SOCKS5 代理。代理无法正确转发该板端私网链路，因此请求没有到达 daemon。

## Fix/workaround

单次命令显式绕过代理：

```sh
curl --noproxy 192.168.7.2 http://192.168.7.2:8080/status
```

当前 shell 配置私网直连：

```sh
export no_proxy="127.0.0.1,localhost,192.168.7.2"
export NO_PROXY="$no_proxy"
```

## Verification

加入 `--noproxy` / `NO_PROXY` 后，`/status`、`/` 和 `/stream` 均可访问。浏览器动态画面、multipart 数据和后续稳定性测试正常，因此该问题状态为 Resolved。

## Impact on plan

- 短暂阻塞 M2 HTTP 验证。
- 不需要修改 daemon 网络代码。
- 后续 WSL 到 USB RNDIS 的所有 curl 验证都应显式绕过代理。

## 我当时的错误判断

初始现象是“HTTP 请求卡住”，容易先怀疑 daemon 没有响应或 HTTP 线程死锁。板端进程和监听端口正常这一事实说明，应先检查客户端实际连接路径，而不是立即修改服务端代码。

## 正确排查顺序

1. 在板端确认 daemon 进程存在。
2. 在板端确认 `0.0.0.0:8080` 正在监听。
3. 使用 `curl -v` 查看目标连接地址。
4. 检查 `http_proxy`、`https_proxy`、`all_proxy` 和 `NO_PROXY`。
5. 对 USB RNDIS 私网地址使用 `--noproxy` 重试。
6. 只有直连仍失败时，再检查防火墙、路由和 daemon HTTP 逻辑。

## 可复用经验

- 能 ping 通不代表 HTTP 工具一定走同一条链路；应用层代理可以改变实际连接路径。
- 调试网络时必须区分目标服务地址、代理地址和真实 TCP peer。
- `curl -v` 中的 `Uses proxy env variable` 和 `Trying 127.0.0.1` 是定位代理劫持的关键证据。

## 面试可讲内容

板端服务已经监听，但 WSL curl 超时。通过 verbose 日志发现请求被本地 SOCKS 代理接管，没有直接走 USB RNDIS。配置 `NO_PROXY` 后立即恢复，证明根因在开发主机请求路径而非 daemon。该排查体现了从进程、监听端口到真实网络路径逐层缩小范围的方法。

## Follow-up

1. 将板端私网地址加入个人 WSL 的持久化 `NO_PROXY` 配置。
2. 后续 evidence 命令统一带 `--noproxy 192.168.7.2`。
3. 若 RNDIS 地址变化，同步更新 `NO_PROXY`。
