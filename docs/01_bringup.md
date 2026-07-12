# 01 Bring-Up

## Goal

Confirm the board, development environment and deployment loop are usable.

## Checklist

- [ ] Serial login works.
- [x] SSH login works.
- [x] Board IP is known.
- [x] PC can transfer files to board.
- [x] Cross compiler works.
- [x] `hello_imx6ull` runs on board.
- [x] WSL build tools installed.
- [x] Host scaffold builds in WSL.

## Commands To Record

```sh
uname -a
cat /proc/cpuinfo
ip addr
df -h
dmesg | grep -iE "error|fail|timeout|panic|mmc"
```

## Results

### 2026-06-30 WSL host baseline

Project path:

```text
/home/eric/projects/imx6ull-sense-terminal
```

Git state:

```text
## No commits yet on main
?? .gitignore
?? HANDOFF.md
?? README.md
?? app/
?? config/
?? docs/
?? scripts/
?? systemd/
```

Host kernel:

```text
Linux ERICHOU 6.6.87.2-microsoft-standard-WSL2 #1 SMP PREEMPT_DYNAMIC Thu Jun 5 18:30:46 UTC 2025 x86_64 x86_64 x86_64 GNU/Linux
```

Tool check:

```text
make: not found
gcc: not found
cc: not found
clang: not found
git version 2.43.0
```

Sudo check:

```text
$ sudo -n true
sudo: a password is required
```

Conclusion:

- WSL is usable and the repository is present.
- Git is installed.
- Host build is currently blocked because `make` and a C compiler are not installed.
- Installing `build-essential` needs an interactive sudo password or a user-run command.

Next command to run in WSL:

```sh
cd /home/eric/projects/imx6ull-sense-terminal
sudo apt update
sudo apt install -y build-essential git
make -C app/daemon
./app/daemon/imx6ull-sense -c config/config.json
```

### 2026-07-01 WSL toolchain and host scaffold

Tool check after installing `build-essential`:

```text
$ make --version
GNU Make 4.3

$ gcc --version
gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0

$ git --version
git version 2.43.0
```

Build:

```text
$ make -C app/daemon
make: Entering directory '/home/eric/projects/imx6ull-sense-terminal/app/daemon'
make: Nothing to be done for 'all'.
make: Leaving directory '/home/eric/projects/imx6ull-sense-terminal/app/daemon'
```

Run:

```text
$ ./app/daemon/imx6ull-sense -c config/config.json
imx6ull-sense daemon scaffold
config: config/config.json
next: implement capture_v4l2 -> jpeg_encoder -> http_server -> motion_detector
```

Build artifact:

```text
app/daemon/imx6ull-sense
```

Conclusion:

- WSL build tools are installed.
- The current host-side daemon scaffold builds and runs.
- This binary is an x86_64 WSL build, not yet an ARM board binary.

### 2026-07-01 Board baseline

Board shell prompt observed:

```text
debian@npi:~$
```

Board login accounts:

```text
normal user: debian
root user: root
passwords: stored only in local untracked secrets.local.md
```

Kernel:

```text
$ uname -a
Linux npi 4.19.35-imx6 #1.2508stable SMP PREEMPT Sat Aug 23 03:32:38 UTC 2025 armv7l GNU/Linux
```

CPU:

```text
$ cat /proc/cpuinfo
processor       : 0
model name      : ARMv7 Processor rev 5 (v7l)
BogoMIPS        : 12.00
Features        : half thumb fastmult vfp edsp neon vfpv3 tls vfpv4 idiva idivt vfpd32 lpae
CPU implementer : 0x41
CPU architecture: 7
CPU variant     : 0x0
CPU part        : 0xc07
CPU revision    : 5

Hardware        : Freescale i.MX6 UltraLite (Device Tree)
Revision        : 0000
Serial          : 022c31d7692206e9
```

Network:

```text
$ ip addr
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 scope host lo
       valid_lft forever preferred_lft forever
    inet6 ::1/128 scope host
       valid_lft forever preferred_lft forever
2: eth2: <NO-CARRIER,BROADCAST,MULTICAST,DYNAMIC,UP> mtu 1500 qdisc pfifo_fast state DOWN group default qlen 1000
    link/ether 4e:33:9c:bd:d4:d6 brd ff:ff:ff:ff:ff:ff
3: eth1: <NO-CARRIER,BROADCAST,MULTICAST,DYNAMIC,UP> mtu 1500 qdisc pfifo_fast state DOWN group default qlen 1000
    link/ether 36:b1:fc:0a:94:e7 brd ff:ff:ff:ff:ff:ff
4: sit0@NONE: <NOARP> mtu 1480 qdisc noop state DOWN group default qlen 1000
    link/sit 0.0.0.0 brd 0.0.0.0
5: wlan0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc pfifo_fast state UP group default qlen 1000
    link/ether c0:f5:35:38:f8:0a brd ff:ff:ff:ff:ff:ff
    inet 192.168.18.210/24 brd 192.168.18.255 scope global wlan0
       valid_lft forever preferred_lft forever
    inet6 fe80::c2f5:35ff:fe38:f80a/64 scope link
       valid_lft forever preferred_lft forever
6: usb0: <NO-CARRIER,BROADCAST,MULTICAST,UP> mtu 1500 qdisc pfifo_fast state DOWN group default qlen 1000
    link/ether 9e:6f:b8:94:65:fa brd ff:ff:ff:ff:ff:ff
    inet 192.168.7.2/30 brd 192.168.7.3 scope global usb0
       valid_lft forever preferred_lft forever
```

Storage:

```text
$ df -h
Filesystem      Size  Used Avail Use% Mounted on
udev             81M     0   81M   0% /dev
tmpfs            49M  2.2M   47M   5% /run
/dev/mmcblk1p2  7.1G  615M  6.1G   9% /
tmpfs           244M     0  244M   0% /dev/shm
tmpfs           5.0M  4.0K  5.0M   1% /run/lock
tmpfs           244M     0  244M   0% /sys/fs/cgroup
tmpfs            40K     0   40K   0% /mnt/.psplash
/dev/mmcblk1p1   40M   19M   22M  46% /boot
```

Observed facts:

- Board OS is Linux `4.19.35-imx6`.
- CPU is ARMv7 on Freescale i.MX6 UltraLite.
- Wi-Fi interface `wlan0` has IP `192.168.18.210/24`.
- Root filesystem is `/dev/mmcblk1p2`, 7.1G total, 6.1G available.

Next M0 checks:

```sh
ping 192.168.18.210
ssh debian@192.168.18.210
scp app/daemon/imx6ull-sense debian@192.168.18.210:/tmp/
```

The current `app/daemon/imx6ull-sense` is x86_64 and is only useful for testing the transfer path. An ARM build is still needed before running the daemon on the board.

### 2026-07-01 Host-to-board network check

Ping from WSL to board `wlan0` IP:

```text
$ ping -c 4 192.168.18.210
PING 192.168.18.210 (192.168.18.210) 56(84) bytes of data.
64 bytes from 192.168.18.210: icmp_seq=1 ttl=64 time=0.783 ms
64 bytes from 192.168.18.210: icmp_seq=2 ttl=64 time=0.318 ms
64 bytes from 192.168.18.210: icmp_seq=3 ttl=64 time=0.361 ms
64 bytes from 192.168.18.210: icmp_seq=4 ttl=64 time=0.445 ms

--- 192.168.18.210 ping statistics ---
4 packets transmitted, 4 received, 0% packet loss, time 3039ms
rtt min/avg/max/mdev = 0.318/0.476/0.783/0.182 ms
```

Conclusion:

- WSL can reach the board over `wlan0`.
- File transfer is verified in the SSH/SCP section below.

### 2026-07-01 SSH and SCP path

Non-interactive SSH check from WSL:

```text
$ ssh -o BatchMode=yes -o ConnectTimeout=5 -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null debian@192.168.18.210 true
Warning: Permanently added '192.168.18.210' (ED25519) to the list of known hosts.
Debian GNU/Linux 10

embedfire.com Debian Image 2025-08-23

Support/FAQ: www.firebbs.cn/forum.php

default username:password is [debian:temppwd]
```

SCP transfer from WSL to board:

```text
$ scp -o BatchMode=yes -o ConnectTimeout=5 -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null config/config.json debian@192.168.18.210:/tmp/imx6ull-sense-config.json
Warning: Permanently added '192.168.18.210' (ED25519) to the list of known hosts.
Debian GNU/Linux 10

embedfire.com Debian Image 2025-08-23

Support/FAQ: www.firebbs.cn/forum.php

default username:password is [debian:temppwd]
```

Remote file check:

```text
$ ssh -o BatchMode=yes -o ConnectTimeout=5 -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null debian@192.168.18.210 ls -l /tmp/imx6ull-sense-config.json
-rwxr-xr-x 1 debian debian 239 Jul  1 22:46 /tmp/imx6ull-sense-config.json
Warning: Permanently added '192.168.18.210' (ED25519) to the list of known hosts.
Debian GNU/Linux 10

embedfire.com Debian Image 2025-08-23

Support/FAQ: www.firebbs.cn/forum.php

default username:password is [debian:temppwd]
```

Conclusion:

- SSH login from WSL to board is working.
- SCP from WSL to board is working.
- M0 still needs an ARM build and board-side executable run.

### 2026-07-01 Cross compiler check

Check:

```text
$ command -v arm-linux-gnueabihf-gcc
$ arm-linux-gnueabihf-gcc --version
$ command -v arm-linux-gnueabi-gcc
$ arm-linux-gnueabi-gcc --version
```

Observed result:

```text
No ARM cross compiler found in WSL.
```

Sudo check:

```text
$ sudo -n true
sudo: a password is required
```

Next command to run in WSL:

```sh
sudo apt install -y gcc-arm-linux-gnueabihf
```

### 2026-07-02 Local Linaro cross compiler

The apt package was slow, so the local EmbedFire/Linaro toolchain archive was used from:

```text
D:\BaiduNetdiskDownload\i.MX6ULL_野火\6-开发软件\交叉编译工具链\gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf.tar
```

Extracted toolchain path in WSL:

```text
/home/eric/.local/toolchains/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf
```

Compiler check:

```text
$ /home/eric/.local/toolchains/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc --version
arm-linux-gnueabihf-gcc (Linaro GCC 7.5-2019.12) 7.5.0

$ /home/eric/.local/toolchains/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc -dumpmachine
arm-linux-gnueabihf
```

Clean old host build:

```text
$ make -C app/daemon clean
make: Entering directory '/home/eric/projects/imx6ull-sense-terminal/app/daemon'
rm -f imx6ull-sense main.o
make: Leaving directory '/home/eric/projects/imx6ull-sense-terminal/app/daemon'
```

Cross build:

```text
$ make -C app/daemon CC=/home/eric/.local/toolchains/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc
make: Entering directory '/home/eric/projects/imx6ull-sense-terminal/app/daemon'
/home/eric/.local/toolchains/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc -Wall -Wextra -O2   -c -o main.o main.c
/home/eric/.local/toolchains/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc -Wall -Wextra -O2 -o imx6ull-sense main.o
make: Leaving directory '/home/eric/projects/imx6ull-sense-terminal/app/daemon'
```

Artifact architecture:

```text
$ file app/daemon/imx6ull-sense
app/daemon/imx6ull-sense: ELF 32-bit LSB executable, ARM, EABI5 version 1 (SYSV), dynamically linked, interpreter /lib/ld-linux-armhf.so.3, for GNU/Linux 3.2.0, BuildID[sha1]=f37b34efdcde95b0b93203c2e5c1d80481a2aa5c, with debug_info, not stripped
```

Conclusion:

- ARM cross compiler works.
- Current daemon scaffold builds into an ARM EABI5 Linux executable.
- Board-side run is still pending.

### 2026-07-02 Transfer retry blocked by board network

SCP retry:

```text
$ scp -o BatchMode=yes -o ConnectTimeout=5 -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null app/daemon/imx6ull-sense debian@192.168.18.210:/tmp/imx6ull-sense
ssh: connect to host 192.168.18.210 port 22: No route to host
scp: Connection closed
```

Ping check:

```text
$ ping -c 4 192.168.18.210
PING 192.168.18.210 (192.168.18.210) 56(84) bytes of data.
From 192.168.18.195 icmp_seq=1 Destination Host Unreachable
From 192.168.18.195 icmp_seq=2 Destination Host Unreachable
From 192.168.18.195 icmp_seq=3 Destination Host Unreachable
From 192.168.18.195 icmp_seq=4 Destination Host Unreachable

--- 192.168.18.210 ping statistics ---
4 packets transmitted, 0 received, +4 errors, 100% packet loss, time 3028ms
pipe 4
```

Conclusion:

- The board was reachable on 2026-07-01, but `192.168.18.210` is currently unreachable from WSL.
- Re-check the board's current IP before retrying SCP and board-side execution.

Next board-side checks:

```sh
ip addr show wlan0
ping -c 4 192.168.18.195
```

Next WSL retry after the board IP is confirmed:

```sh
ping -c 4 BOARD_IP
scp app/daemon/imx6ull-sense debian@BOARD_IP:/tmp/imx6ull-sense
scp config/config.json debian@BOARD_IP:/tmp/imx6ull-sense-config.json
ssh debian@BOARD_IP chmod +x /tmp/imx6ull-sense
ssh debian@BOARD_IP /tmp/imx6ull-sense -c /tmp/imx6ull-sense-config.json
```

### 2026-07-02 Retry after board power-on

Ping recovered but showed packet loss:

```text
$ ping -c 4 192.168.18.210
PING 192.168.18.210 (192.168.18.210) 56(84) bytes of data.
64 bytes from 192.168.18.210: icmp_seq=3 ttl=64 time=141 ms
64 bytes from 192.168.18.210: icmp_seq=4 ttl=64 time=106 ms

--- 192.168.18.210 ping statistics ---
4 packets transmitted, 2 received, 50% packet loss, time 3017ms
rtt min/avg/max/mdev = 105.695/123.589/141.483/17.894 ms
```

SCP retry still failed due unstable SSH/network:

```text
$ scp app/daemon/imx6ull-sense debian@192.168.18.210:/tmp/imx6ull-sense
ssh: connect to host 192.168.18.210 port 22: Connection timed out
scp: Connection closed

$ scp config/config.json debian@192.168.18.210:/tmp/imx6ull-sense-config.json
Connection to 192.168.18.210 port 22 timed out
scp: Connection closed
```

Direct SSH run attempt:

```text
$ ssh debian@192.168.18.210 'ls -l /tmp/imx6ull-sense /tmp/imx6ull-sense-config.json; timeout 5 /tmp/imx6ull-sense -c /tmp/imx6ull-sense-config.json; echo exit:$?'
ssh: connect to host 192.168.18.210 port 22: No route to host
```

Conclusion:

- ARM build is ready.
- Board-side run is blocked by unstable board Wi-Fi/SSH reachability, not by code or build failure.
- If the board local terminal is available, run the scaffold locally from `/tmp` and paste the output.

Board-local fallback command:

```sh
ls -l /tmp/imx6ull-sense /tmp/imx6ull-sense-config.json
/tmp/imx6ull-sense -c /tmp/imx6ull-sense-config.json
echo $?
```

### 2026-07-03 USB RNDIS board run

After replacing the USB cable and installing the Windows-side RNDIS driver, USB networking became the default M0 board link.

#### Windows 侧 RNDIS 驱动安装与接线

已验证成功的接线方式：

    Windows 主机 USB 口
      -> 支持数据传输的 USB 线
      -> EBF6ULL S1 Pro 的 USB OTG 接口

硬性要求：

- 必须连接开发板的 USB OTG 接口，不能连接普通 USB HOST 接口。RNDIS 需要开发板以 USB device/gadget 身份被 Windows 枚举。
- 必须使用支持数据传输的 USB 线。仅充电线可能有供电表现，但不会建立 USB 数据链路。
- Windows 必须为枚举出的 USB 网络设备绑定 RNDIS 驱动。

Windows 驱动安装步骤：

1. 使用数据线连接 Windows 主机和开发板 USB OTG 接口。
2. 打开“设备管理器”，找到带黄色警告或尚未正确识别的 USB/RNDIS 设备。
3. 右键选择“更新驱动程序”。
4. 选择“浏览我的电脑以查找驱动程序”。
5. 选择“让我从计算机上的可用驱动程序列表中选取”。
6. 设备类型选择“网络适配器”，厂商选择 Microsoft。
7. 选择 USB RNDIS Adapter。部分 Windows 版本可能显示为 Remote NDIS Compatible Device。
8. 安装完成后重新拔插 OTG 数据线，确认网络适配器无黄色警告。

优先使用 Windows 内置 RNDIS 驱动，不从不明网站下载第三方 INF。

验证命令：

    # 板端本地终端
    ip addr show usb0
    cat /sys/class/net/usb0/carrier

    # WSL
    ping -c 4 192.168.7.2
    ssh debian@192.168.7.2 'ip addr show usb0'

成功标志：

    usb0: <BROADCAST,MULTICAST,UP,LOWER_UP>
    carrier: 1
    inet 192.168.7.2/30

踩坑结论：

- 接到 USB HOST 口时，Windows 不会把开发板识别为 RNDIS gadget。
- 仅充电线或不稳定线材可能让 usb0 保持 NO-CARRIER。
- Windows 发现设备但未绑定 RNDIS 驱动时，不会出现可用 USB 网络适配器。
- 固定排查顺序：OTG 接口 -> 数据线 -> Windows 驱动 -> usb0 carrier -> IP/SSH。此时不要先修改应用代码。

WSL artifact check:

```text
$ file app/daemon/imx6ull-sense
app/daemon/imx6ull-sense: ELF 32-bit LSB executable, ARM, EABI5 version 1 (SYSV), dynamically linked, interpreter /lib/ld-linux-armhf.so.3, for GNU/Linux 3.2.0, BuildID[sha1]=f37b34efdcde95b0b93203c2e5c1d80481a2aa5c, with debug_info, not stripped
```

USB RNDIS ping from WSL:

```text
$ ping -c 4 192.168.7.2
PING 192.168.7.2 (192.168.7.2) 56(84) bytes of data.
64 bytes from 192.168.7.2: icmp_seq=1 ttl=64 time=1.02 ms
64 bytes from 192.168.7.2: icmp_seq=2 ttl=64 time=0.895 ms
64 bytes from 192.168.7.2: icmp_seq=3 ttl=64 time=0.867 ms
64 bytes from 192.168.7.2: icmp_seq=4 ttl=64 time=0.866 ms

--- 192.168.7.2 ping statistics ---
4 packets transmitted, 4 received, 0% packet loss, time 3051ms
rtt min/avg/max/mdev = 0.866/0.912/1.023/0.064 ms
```

Board USB interface over SSH:

```text
$ ssh debian@192.168.7.2 'uname -a; ip addr show usb0'
Linux npi 4.19.35-imx6 #1.2508stable SMP PREEMPT Sat Aug 23 03:32:38 UTC 2025 armv7l GNU/Linux
6: usb0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc pfifo_fast state UP group default qlen 1000
    link/ether 72:a5:3b:17:7c:c6 brd ff:ff:ff:ff:ff:ff
    inet 192.168.7.2/30 brd 192.168.7.3 scope global usb0
       valid_lft forever preferred_lft forever
    inet6 fe80::70a5:3bff:fe17:7cc6/64 scope link
       valid_lft forever preferred_lft forever
```

SCP transfer:

```text
$ scp app/daemon/imx6ull-sense debian@192.168.7.2:/tmp/imx6ull-sense
$ scp config/config.json debian@192.168.7.2:/tmp/imx6ull-sense-config.json
```

Board-side run:

```text
$ ssh debian@192.168.7.2 'ls -l /tmp/imx6ull-sense /tmp/imx6ull-sense-config.json; chmod +x /tmp/imx6ull-sense; /tmp/imx6ull-sense -c /tmp/imx6ull-sense-config.json; echo exit:$?'
-rwxr-xr-x 1 debian debian 12196 Jul  3 00:05 /tmp/imx6ull-sense
-rwxr-xr-x 1 debian debian   239 Jul  3 00:05 /tmp/imx6ull-sense-config.json
imx6ull-sense daemon scaffold
config: /tmp/imx6ull-sense-config.json
next: implement capture_v4l2 -> jpeg_encoder -> http_server -> motion_detector
exit:0
```

Conclusion:

- USB RNDIS `192.168.7.2` is stable and should be used as the default board connection.
- ARM daemon scaffold transfers to the board and runs successfully.
- M0 environment and board baseline is complete.
