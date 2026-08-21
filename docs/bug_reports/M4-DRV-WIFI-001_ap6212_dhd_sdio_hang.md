# Bug M4-DRV-WIFI-001：AP6212 DHD/SDIO 拉起导致板端用户态失去响应

## 状态

- 归属阶段：M4
- 类型：内核 / Wi-Fi / SDIO / 启动服务
- 状态：Mitigated（项目范围已解决，底层 BSP 根因待独立深挖）
- 发现日期：2026-08-19
- 是否阻塞：曾阻塞；当前不阻塞 M4，项目部署继续使用 USB RNDIS

## 环境

- 主机：Windows + WSL Ubuntu
- 开发板：EBF6ULL S1 Pro，i.MX6ULL，AP6212 Wi-Fi
- 系统：野火 Debian 10 镜像，内核 `4.19.35-imx6`
- 启动介质：eMMC，根文件系统 `/dev/mmcblk1p2`
- 串口：`ttymxc0`，实际使用 `115200 8N1`、无流控
- 相关服务：`autowifi.service`、`connman.service`、`networking.service`
- 项目网络主链路：USB RNDIS，板端地址 `192.168.7.2`

## 现象

系统启动后可能出现以下不同表象：

- 串口能够输出启动日志，但在登录提示附近停止响应，键盘输入无回显。
- USB OTG 重新插拔后没有新的串口日志。
- 板端仍可能响应 `ping`，但 SSH 卡在 banner 交换阶段，无法进入用户态会话。
- 启动日志出现 `wl_android_wifi_on failed (-35)`、`unbalanced disables for wlan-en-gpio`。
- `Raise network interfaces` 和 `dnsmasq` 的失败信息同时出现，容易被误判为唯一根因。

该故障具有间歇性：有时 DHD 初始化失败，有时打印 `wl_android_wifi_on : Success` 后仍然失去用户态响应。

## 复现步骤

最小隔离测试使用一次性 U-Boot 内核参数同时屏蔽 `autowifi.service` 和 `connman.service`，启动后只执行：

```sh
ifconfig wlan0 up
```

关键结果：

```text
[dhd-wlan0] wl_android_wifi_on : in g_wifi_on=0
...
[dhd-wlan0] wl_android_wifi_on : Success
```

命令没有返回 `IFCONFIG_RC`，串口随后无响应，`Ctrl+C` 和 SysRq-w 均未产生有效输出。该结果表明，即使排除 ConnMan 与 `autowifi` 的并发控制，单独拉起 `wlan0` 也能触发问题。

## 关键证据

### 1. 厂商镜像基线

检查本地 2025-08 构建时期的厂商包：

```text
D:\BaiduNetdiskDownload\i.MX6ULL_野火\8-SDK源码压缩包\ebf-image-builder_20250812.tar.gz
```

构建脚本会安装 `autowifi.service`，但没有发现默认执行 `systemctl enable autowifi.service`，归档中也没有预置的 enable 符号链接。构建默认还会 mask `wpa_supplicant.service`，并 disable `dnsmasq.service`、`udhcpd.service` 和 `connman-wait-online.service`。

故障板上却存在：

```text
/etc/systemd/system/multi-user.target.wants/autowifi.service
  -> /lib/systemd/system/autowifi.service
```

因此，`autowifi` 自动启动不是所检查厂商构建包的默认启用状态。

### 2. 多套 Wi-Fi 控制路径

`autowifi` 脚本同时执行：

```text
connmanctl enable wifi
ifconfig wlan0 up
wpa_supplicant -B ...
udhcpc ...
```

与此同时 `connman.service` 仍为 active，形成 ConnMan 与脚本直接控制 `wlan0`、`wpa_supplicant`、DHCP 的拆分所有权。这会增加时序不确定性，但隔离测试证明它不是触发故障的必要条件。

### 3. DHD/SDIO 日志和源码定义

历史启动日志包含：

```text
wl_android_wifi_on failed (-35)
unbalanced disables for wlan-en-gpio
```

厂商 DHD 源码将 `-35` 定义为 Broadcom 私有错误：

```text
BCME_SDIO_ERROR = -35
```

它不是 Linux errno 中同数字的普通语义。源码路径显示 `wl_android_wifi_on` 会依次执行 Wi-Fi 上电、SDIO bus resume 和 `dhd_net_bus_devreset`；失败后的多层清理路径会关闭 regulator，因此 `unbalanced disables` 更像二次清理现象，而非最初故障点。

### 4. 板级资料

排查时以 [`../reference/hardware/local-board-documents.md`](../reference/hardware/local-board-documents.md) 为资料入口，并对照厂商设备树中的 `imx6ull-mmc-npi.dts`、`imx-fire-btwifi-overlay.dts` 以及 DHD 驱动源码。当前 `/boot/uEnv.txt` 使用：

```text
dtoverlay=/usr/lib/linux-image-4.19.35-imx6/overlays/imx-fire-btwifi.dtbo
```

## 根因

### 项目层面已验证的根因

板上后来被启用的 `autowifi.service` 会在每次启动时自动拉起当前项目并不需要的 `wlan0`，从而进入 AP6212/DHD/SDIO 的不稳定初始化路径。一旦该路径阻塞，串口登录、SSH 等用户态交互会一起表现为“系统卡住”。

`/etc/network/interfaces` 的误写、热点相关服务失败和 ConnMan/脚本竞争会制造额外错误并放大时序问题，但不是唯一根因。决定性证据是：屏蔽 ConnMan 和 `autowifi` 后，单独执行 `ifconfig wlan0 up` 仍能复现卡死。

### 尚未完全闭合的底层根因

目前只能把范围缩小到 AP6212 的 DHD/SDIO 打开路径。日志在 `wl_android_wifi_on : Success` 后停止，但没有内核函数级时间戳、栈回溯或硬件波形，因此不能诚实地断言具体卡在 `dhd_bus_start`、固件同步、SDIO 中断还是电源时序。该部分不阻塞本项目，保留为 BSP 专项问题。

## 修复或绕过

项目不依赖板载 Wi-Fi，采用最小改动：

1. 临时在 U-Boot 的当次启动参数中加入 `systemd.mask=autowifi.service`，先获得稳定用户态；没有执行 `saveenv`。
2. 保留厂商 unit 文件 `/lib/systemd/system/autowifi.service`，只删除其持久 enable 链接：

   ```sh
   unlink /etc/systemd/system/multi-user.target.wants/autowifi.service
   ```

3. 保留 `connman.service`，不继续修改厂商 Wi-Fi 管理栈。
4. 保持 `wlan0` 为 down；M4/MVP 的部署和管理继续使用 USB RNDIS `192.168.7.2`。

第一次在带有内核命令行 mask 的启动中执行 `systemctl disable autowifi.service` 虽返回 0，但 systemd 提示 `/run/systemd/generator.early/autowifi.service` 已被 mask，并未删除真实 enable 链接。随后改用精确 `unlink` 并验证目标路径，避免误以为已经永久禁用。

## 验证

移除一次性 U-Boot mask 后正常重启，内核命令行不再包含 `systemd.mask=autowifi.service`。板端只读检查结果：

```text
systemctl is-enabled autowifi.service -> disabled
systemctl is-active autowifi.service  -> inactive
systemctl is-active connman.service   -> active
systemctl is-active networking.service -> active
systemctl is-active ssh.service       -> active
autowifi enable link                  -> absent
wlan0                                 -> DOWN
```

正常启动日志中不再出现 `Starting =booting wifi`、DHD 自动上电或扫描信息，串口用户态可正常进入。

第一次从 WSL 复核时，OTG 数据线尚未插入，Windows 只有历史 RNDIS phantom 设备，板端 `usb0` 为 `carrier=0`。此时 `192.168.7.2` 被 Windows `xray_tun` 默认路由接管，造成 ping 和 TCP/22 看似成功、却没有真实 SSH banner 的假阳性。该结果不能作为板端证据。

重新插入 OTG 数据线后的有效证据为：

```text
Windows USB RNDIS Adapter -> Up
Windows RNDIS address     -> 192.168.7.1/30
WSL route                 -> 192.168.7.2 dev eth5 src 192.168.7.1
board usb0                -> UP, carrier=1, 192.168.7.2/30
ping                      -> 4/4, 0% packet loss
SSH banner                -> SSH-2.0-OpenSSH_7.9p1 Debian-10+deb10u2
SSH command               -> success
```

因此，Wi-Fi 修复和 USB RNDIS 主链路均已真实验证。8080 端口当时拒绝连接是因为 daemon 尚未启动，不是网络故障。

## 对计划的影响

- 本 Bug 在项目范围内已解除阻塞，M4 可以继续。
- 不恢复板载 Wi-Fi，不把 Wi-Fi 作为 M4 验收依赖。
- USB RNDIS 继续作为部署、SSH 和 HTTP 验证主链路。
- AP6212/DHD 的深层根因调查独立于 MVP，不扩张 M4 范围。

## 我当时的错误判断

1. 最初把 `/etc/network/interfaces` 的损坏和 `dnsmasq` 失败视为整个卡死的直接根因；它们确实需要恢复，但不能解释隔离后的 `ifconfig wlan0 up` 卡死。
2. 随后把 ConnMan 与 `autowifi` 的竞争视为必要条件；禁用两者后单独拉起 `wlan0` 仍复现，推翻了该判断。
3. 一度可能把 `-35` 按 Linux errno 解读；核对厂商 DHD 源码后确认它是 Broadcom 的 `BCME_SDIO_ERROR`。

## 正确排查顺序

1. 先区分“内核仍活着”和“用户态仍可调度”：分别验证串口输入、ping、SSH banner 和 SysRq。
2. 使用一次性 U-Boot 参数屏蔽自动启动服务，避免永久修改干扰诊断。
3. 对比厂商原始构建包，确认服务是“存在”还是“默认启用”。
4. 一次只恢复一个变量：先无 Wi-Fi 启动，再单独拉起 `wlan0`，最后才测试网络管理器。
5. 结合驱动自己的错误码定义解读日志，不直接套用同数字的 Linux errno。
6. 修复后必须在无临时 mask 的正常启动中复验 enable/active 状态和 DHD 日志。

## 可复用经验

- systemd unit 文件存在不等于服务默认启用，必须检查 `*.wants/` 链接和厂商构建脚本。
- `systemctl disable` 的退出码为 0 不代表目标链接一定被删除，尤其当同名 unit 被 kernel command line generator 临时 mask 时。
- ping 正常只能证明部分内核网络路径仍工作，不能证明用户态、sshd 或串口 getty 正常。
- 验证私网设备前必须同时检查路由、源地址和对端特征；否则 TUN/VPN 可能对不存在的目标制造 ping/TCP 假阳性。
- 无线驱动的私有负数错误码必须回到对应源码定义中解释。
- 多个网络管理器确实是风险，但应通过单变量隔离证明它是否为必要触发条件。

## 从现象到验证

启动后串口和 SSH 间歇失去响应，同时日志出现 DHD `-35`。先用 U-Boot 临时 mask 获得稳定系统，再对比厂商镜像确认 `autowifi` 并非默认启用。通过“禁用 ConnMan 和 autowifi 后单独 `ifconfig wlan0 up`”把问题缩小到 AP6212 DHD/SDIO 打开路径，证明网络配置错误和服务竞争只是伴随因素。项目不依赖 Wi-Fi，因此采用删除 autowifi enable 链接、保留 ConnMan 和 USB RNDIS 的最小修复，并在无临时参数的正常重启中完成验证。

## 后续跟进

以下方向用于未来 BSP 专项排查，不阻塞当前项目：

1. 导出运行时设备树、regulator、MMC/SDIO 状态，并与 S1 Pro 原理图及 overlay 逐项对照。
2. 校验板端 Wi-Fi firmware、NVRAM 和 CLM 文件的版本及校验和。
3. 建立冷启动、热重启、USB 负载和供电条件矩阵，统计复现率。
4. 在 `wl_android_wifi_on`、`dhd_bus_devreset`、`dhdsdio_probe_attach`、`dhdsdio_download_firmware`、`dhd_bus_init`、`dhd_bus_start`、`dhd_sync_with_dongle` 增加带时间戳的临时插桩。
5. 降低 SDIO `max-frequency` 做 A/B 测试，判断信号完整性或时序裕量是否相关。
6. 必要时使用示波器或逻辑分析仪观察 `WIFI_3V3`、`WL_REG_ON`、SDIO CLK/CMD/DATA。
