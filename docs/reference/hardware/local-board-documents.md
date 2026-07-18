# 本地板级资料索引

本项目在 Windows 主机上有本地厂商资料和硬件资料。遇到开发板硬件、pinout、启动行为、摄像头接线、设备树或 i.MX6ULL 外设细节时，必须先查这些资料，不要凭经验猜测。

## 野火教程文档

```text
D:\BaiduNetdiskDownload\i.MX6ULL_野火\1-野火开源图书_教程文档
```

用途：

- 开发板 bring-up 流程。
- Linux 应用和驱动示例。
- 野火 EBF6ULL 相关使用说明。
- 与当前板卡镜像匹配的排查命令。

## NXP 官方手册

```text
D:\BaiduNetdiskDownload\i.MX6ULL_野火\5-NXP官方手册
```

用途：

- i.MX6ULL Reference Manual 细节。
- 外设行为和寄存器级事实。
- CSI、USB、GPIO、clock、IOMUX、boot 等芯片级参考。
- 当野火教程过于上层时，用于确认芯片级事实。

## EBF6ULL S1 Pro 硬件资料和原理图

```text
D:\BaiduNetdiskDownload\i.MX6ULL_野火\2-硬件资料\ebf_6ull_hardware_20240710\hardware\EBF6ULL S1 Pro
```

这个目录尤其重要。涉及板级连线的问题时，原理图必须作为事实来源。

用途：

- 摄像头接口 pinout 和电压。
- USB host 连接和供电。
- GPIO、reset、电源和时钟信号。
- 后续尝试 OV5640 时的 CSI 相关引脚。
- 排查硬件不匹配或设备树假设错误。

## 使用规则

当 bug 涉及板级硬件、摄像头 bring-up、USB、CSI、设备树、电源、pinmux 或启动行为时：

1. 先检查相关本地资料路径。
2. 在对应 `docs/` 日志中记录使用的具体文档名和页码/章节。
3. 如果原理图或 NXP 手册能回答问题，不要凭记忆继续推进。
