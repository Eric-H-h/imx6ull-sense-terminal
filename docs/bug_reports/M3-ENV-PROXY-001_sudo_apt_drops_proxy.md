# Bug M3-ENV-PROXY-001：sudo 清理代理导致 APT 下载超时

## 状态

- 归属阶段：M3 — Motion Event preflight
- 类型：开发主机环境 / APT / Proxy
- 状态：Resolved
- 发现日期：2026-07-19
- 修复方式：为单次 APT 命令显式传入 `Acquire::http::Proxy` 和 `Acquire::https::Proxy`

## Environment

- 开发主机：Windows + WSL Ubuntu 24.04.1 LTS amd64
- 本地代理：`http://127.0.0.1:10808`
- 软件源：`http://archive.ubuntu.com/ubuntu`
- 目标包：`libjpeg-dev`

## Symptom

普通用户 shell 已配置 HTTP/HTTPS proxy，但执行：

```sh
sudo apt install -y libjpeg-dev
```

APT 多次重试后仍无法下载主要开发包，8 分钟只取得两个约 1.5 KB 的 dummy 包。

## Reproduction

```text
Ign: libjpeg-turbo8-dev
Err: libjpeg-turbo8-dev
Connection failed [IP: 91.189.92.24 80]
Fetched 2966 B in 8min 2s
E: Unable to fetch some archives
```

## Root cause

`sudo` 默认不会保留普通用户 shell 的全部代理环境变量。APT 因此没有使用 WSL 中的本地代理，而是直接访问 `archive.ubuntu.com:80`。当前网络对该直连路径不稳定，最终超时。

这不是软件包依赖冲突，也不是 Ubuntu archive 缺少 `libjpeg-dev`。

## Fix/workaround

对本次安装显式设置 APT proxy：

```sh
sudo apt-get \
  -o Acquire::http::Proxy="http://127.0.0.1:10808" \
  -o Acquire::https::Proxy="http://127.0.0.1:10808" \
  -o Acquire::Retries=3 \
  install -y libjpeg-dev
```

该方式只影响当前命令，不写入全局 APT 配置。

## Verification

```text
Get: libjpeg-turbo8-dev 295 kB
Fetched 295 kB in 3s
Setting up libjpeg-turbo8-dev
Setting up libjpeg8-dev
Setting up libjpeg-dev
host_jpeg_header_exit:0
```

随后 host libjpeg 探针成功编译并运行：

```text
libjpeg_probe_ok version=80
host_probe_exit:0
```

## Impact on plan

- 暂时阻塞 M3 JPEG 解码依赖检查。
- 不需要修改项目源码。
- ARM 开发包仍使用独立 Debian 10 armhf sysroot，不能与 Ubuntu amd64 包混用。

## 我当时的错误判断

看到 APT 下载失败时，容易先认为软件源不可用或包名错误。实际候选包已经被 APT 正确解析，失败发生在下载连接阶段。对比普通 shell 的 proxy 与 sudo 后的执行环境，才能定位到代理没有传递。

## 正确排查顺序

1. 确认 APT 已解析到候选版本和下载 URL。
2. 区分依赖解析失败与 archive 连接失败。
3. 查看普通 shell 的 `http_proxy`、`https_proxy`。
4. 判断 `sudo` 后代理是否仍生效。
5. 使用单次 `Acquire::Proxy` 参数重试。
6. 成功后验证头文件和实际编译，不只检查安装命令 exit code。

## 可复用经验

- 普通用户能通过代理联网，不代表 sudo 启动的包管理器也会使用相同代理。
- 单次 APT proxy 参数比直接修改全局配置更适合项目临时依赖安装。
- `Ign` 后连续出现 `Err: Connection failed` 通常表示传输路径问题，不等于包不存在。

## 面试可讲内容

M3 引入 libjpeg 时，APT 能解析包但下载长期超时。通过日志确认请求绕过本地代理直连 Ubuntu archive，根因是 sudo 未保留用户代理环境。使用单次 APT proxy 参数后，295 KB 主包在 3 秒内下载完成，并通过真实编译探针验证依赖可用。

## Follow-up

1. 后续需要 sudo 联网时先明确代理传递方式。
2. 不在仓库或系统配置中记录代理认证信息。
3. M3 构建文档记录 host 与 ARM libjpeg 来源，避免架构混用。
