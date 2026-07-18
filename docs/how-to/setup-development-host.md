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
make -C app/daemon clean all
```

## 交叉编译

先确认项目配置的 ARM hard-float 工具链可用，再执行：

```sh
make -C app/daemon clean all CROSS_COMPILE=arm-linux-gnueabihf-
file app/daemon/imx6ull-sense
```

预期 `file` 输出包含 32-bit ARM、EABI5 和 hard-float 解释器信息。实际工具链版本和首次构建证据见 [M0 bring-up](../verification/evidence/M0_bringup.md)。

## 常见误区

- 不要把 WSL x86_64 构建产物复制到 ARM 开发板。
- Git 操作应在 WSL 路径执行，不要使用 Windows Git 操作 UNC 仓库。
