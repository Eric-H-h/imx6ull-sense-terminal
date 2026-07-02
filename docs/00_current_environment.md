# 00 Current Environment

Recorded on 2026-06-30.

## WSL

```text
Linux ERICHOU 6.6.87.2-microsoft-standard-WSL2 #1 SMP PREEMPT_DYNAMIC Thu Jun 5 18:30:46 UTC 2025 x86_64 GNU/Linux
```

## Build Tools

```text
make: not installed
gcc: not installed
```

## Next Required Action

Install base development tools in WSL:

```sh
sudo apt update
sudo apt install -y build-essential git
```

No target-board build has been attempted yet.

