# Beginner Guide

This guide will be expanded as each milestone is verified on the board.

## 1. Repository

Target path:

```sh
cd /home/eric/projects
git clone TBD imx6ull-sense-terminal
cd imx6ull-sense-terminal
```

For the first local setup:

```sh
cd /home/eric/projects/imx6ull-sense-terminal
git status
```

## 2. Basic Workflow

Every work session:

```sh
git status
git pull    # only after remote exists
```

After a working change:

```sh
git add .
git commit -m "docs: update bring-up notes"
```

## 3. Board Debug Loop

1. Build on host.
2. Copy binary to board.
3. Run on board.
4. Save logs in `docs/`.
5. Commit only after the result is understood.

## 4. Current WSL Baseline

Initial check on 2026-06-30:

```text
WSL distro: Ubuntu
Kernel: Linux ERICHOU 6.6.87.2-microsoft-standard-WSL2
make: not installed
gcc: not installed
```

Before compiling the host-side scaffold or cross-compiling for i.MX6ULL, install the basic build tools in WSL:

```sh
sudo apt update
sudo apt install -y build-essential git
```

Then verify:

```sh
make --version
gcc --version
git --version
```

