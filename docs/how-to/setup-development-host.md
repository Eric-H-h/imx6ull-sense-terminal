# 准备 WSL 开发环境

## 前置条件

- Windows 已安装 Ubuntu WSL。
- 仓库位于 `/home/eric/projects/imx6ull-sense-terminal`。
- 安装软件包时可以输入 WSL 用户的 sudo 密码。

## 安装基础工具

```sh
sudo apt update
sudo apt install -y build-essential git
```

验证：

```sh
make --version
gcc --version
git --version
```

## 构建主机版本

```sh
cd /home/eric/projects/imx6ull-sense-terminal
make -C app/daemon verify
```

`verify` 会依次执行 clean、主机 daemon 构建和全部单元测试。修改 Makefile 后先用 `make -C app/daemon -n verify` 检查命令展开。

## 交叉编译

M3 起使用固定脚本，避免不同 shell 的 PATH 和手工参数不一致：

```sh
./scripts/build-arm.sh
file app/daemon/build/arm/imx6ull-sense
```

默认使用 `~/.local/toolchains/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin` 和 `~/.local/sysroots/imx6ull-libjpeg-deb10-1.5.2-armhf`。可通过 `ARM_TOOLCHAIN_BIN`、`ARM_JPEG_ROOT`、`ARM_CC`、`ARM_READELF` 和 `ARM_OUTPUT_DIR` 覆盖。

脚本会验证输入、生成 ARM EABI5 产物、检查 `NEEDED` 动态库，并清理可能与主机构建混用的中间对象。实际工具链版本见 [M0 bring-up](../verification/evidence/M0_bringup.md)，libjpeg 与 M3 产物证据见 [M3 evidence](../verification/evidence/M3_motion_event.md)。

## 常见误区

- 不要把 WSL x86_64 构建产物复制到 ARM 开发板。
- Git 操作应在 WSL 路径执行，不要使用 Windows Git 操作 UNC 仓库。
