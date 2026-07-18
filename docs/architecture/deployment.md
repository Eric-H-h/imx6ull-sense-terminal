# 部署架构

## 当前开发链路

```text
Windows
  -> WSL Ubuntu: 构建和 Git
  -> USB RNDIS: 192.168.7.2
  -> EBF6ULL S1 Pro: /tmp 下运行和验证
  -> Windows 浏览器: HTTP 8080
```

仓库实际路径：

```text
/home/eric/projects/imx6ull-sense-terminal
```

Git 命令必须在 WSL 路径中执行，避免 Windows Git 通过 UNC 访问时误判文件状态。具体操作见 [Git 工作流](../how-to/git-workflow.md)。

## 当前板端访问

- 默认账户和地址：`debian@192.168.7.2`
- 物理接口：开发板 USB OTG
- Windows 驱动：`USB RNDIS Adapter` 或等价 Remote NDIS 驱动
- Wi-Fi `192.168.18.210` 不是默认部署路径

连接和部署步骤见 [连接与部署开发板](../how-to/connect-and-deploy-board.md)。

## M4 目标部署

```text
/usr/local/bin/imx6ull-sense
/etc/imx6ull-sense/config.json
/var/lib/imx6ull-sense/
/var/log/imx6ull-sense/events.jsonl
systemd: imx6ull-sense.service
```

上述正式路径在完成 M4 板端验证前属于目标状态，不应当作当前已部署事实。
