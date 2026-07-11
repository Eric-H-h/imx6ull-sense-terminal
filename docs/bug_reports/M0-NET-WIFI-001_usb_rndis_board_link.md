# Bug M0-NET-WIFI-001：板端部署链路不稳定

## 状态

- 归属阶段：M0 — 环境与板端基线
- 类型：网络、SSH、SCP、部署链路
- 状态：M0 已解决
- 解决日期：2026-07-03
- 最终默认链路：USB RNDIS `192.168.7.2`

## 环境

- 主机：Ubuntu on WSL2
- 开发板：野火 EBF6ULL S1 Pro / i.MX6ULL eMMC
- 板端内核：`Linux npi 4.19.35-imx6 ... armv7l`
- 原始网络路径：Wi-Fi `192.168.18.210`
- 最终网络路径：USB RNDIS `192.168.7.2`

## 现象

板端 Wi-Fi 路径 `192.168.18.210` 在部署过程中不稳定。

观察到的失败包括：

- 高丢包
- SSH 超时
- SCP 超时
- `No route to host`
- WSL 到板端的可达性不稳定

切换到 USB 路线时，板端 `usb0` 初期也显示无 carrier，Windows 侧没有立即暴露可用的 RNDIS 网卡。

## 触发场景

问题出现在 M0 阶段，尝试从 WSL 向板端传输并运行 ARM scaffold 时。

原计划命令：

```sh
scp app/daemon/imx6ull-sense debian@192.168.18.210:/tmp/imx6ull-sense
scp config/config.json debian@192.168.18.210:/tmp/imx6ull-sense-config.json
ssh debian@192.168.18.210 '/tmp/imx6ull-sense -c /tmp/imx6ull-sense-config.json'
```

## 关键日志

Wi-Fi 丢包：

```text
$ ping -c 4 192.168.18.210
4 packets transmitted, 2 received, 50% packet loss
```

SCP 失败：

```text
ssh: connect to host 192.168.18.210 port 22: Connection timed out
scp: Connection closed
```

SSH 失败：

```text
ssh: connect to host 192.168.18.210 port 22: No route to host
```

USB RNDIS 初始 carrier 问题：

```text
usb0: <NO-CARRIER,BROADCAST,MULTICAST,UP>
carrier: 0
operstate: down
```

## 根因

M0 的阻塞点不是应用 scaffold。ARM 二进制当时已经可以构建。

实际根因是部署链路不稳定：

- Wi-Fi 不足以稳定支撑 SSH/SCP。
- 第一根 USB 线没有建立可用的 USB 网络链路。
- Windows 侧需要正确识别或绑定 RNDIS 网卡驱动路径。

## 修复方式

M0 部署路径从 Wi-Fi 切换为 USB RNDIS。

执行动作：

1. 更换支持数据传输的 USB 线。
2. 安装或选择 Windows 侧 USB RNDIS Adapter。
3. 使用板端 USB 网卡地址 `192.168.7.2`。
4. 通过 USB RNDIS 重新验证 ping、SSH、SCP 和板端执行。

## 验证结果

USB RNDIS ping：

```text
$ ping -c 4 192.168.7.2
4 packets transmitted, 4 received, 0% packet loss
```

板端 USB 状态：

```text
usb0: <BROADCAST,MULTICAST,UP,LOWER_UP>
inet 192.168.7.2/30
```

SCP：

```text
scp app/daemon/imx6ull-sense debian@192.168.7.2:/tmp/imx6ull-sense
scp config/config.json debian@192.168.7.2:/tmp/imx6ull-sense-config.json
```

两次传输退出码均为 0。

板端运行：

```text
imx6ull-sense daemon scaffold
config: /tmp/imx6ull-sense-config.json
next: implement capture_v4l2 -> jpeg_encoder -> http_server -> motion_detector
exit:0
```

## 影响

M0 因部署链路调试被延迟，但不需要修改应用代码。

项目现在有稳定的默认板端访问路径：

```text
debian@192.168.7.2
```

## 对计划的影响

- M0 仍然聚焦在构建、部署、运行闭环，没有提前进入 V4L2 或 MJPEG。
- M1-M5 的默认传输通道从 Wi-Fi 改为 USB RNDIS。
- 后续板端证据默认通过 `debian@192.168.7.2` 采集，除非后续 bug 报告修改该默认路径。

## 后续规则

- M1-M5 默认使用 USB RNDIS `192.168.7.2`。
- Wi-Fi 只作为可选路径，不作为主部署路径。
- 如果 SSH/SCP 再次失败，先检查物理链路和 `usb0` 状态，再考虑应用代码。
- 后续网络问题要创建新的 bug 报告，不再混入本 M0 报告。
