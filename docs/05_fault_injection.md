# 05 Fault Injection

## Goal

Prove this is a long-running embedded service, not a one-shot demo.

## Required Cases

| Case | Command | Expected | Result |
|---|---|---|---|
| Kill process | `kill -9 PID` | systemd restarts service | TBD |
| Reboot board | `reboot` | service starts automatically | TBD |
| Camera missing | unplug/disable camera | daemon enters degraded mode | TBD |
| Bad config | corrupt config | fail-safe with clear log | TBD |

## Logs

```sh
journalctl -u imx6ull-sense -n 100
```

