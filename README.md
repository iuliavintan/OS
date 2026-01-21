# vimonOS

## Overview
This project builds a small 32-bit x86 OS image and runs it in QEMU.

## Quick Start (Linux)
1. Run the setup script to install missing tools and build the cross compiler:
   - `./setup_env.sh`
2. Restart your shell or load your rc file:
   - `source ~/.zshrc` or `source ~/.bashrc`
3. Build and run:
   - `make run`

## What the Setup Script Does
`setup_env.sh` will:
- Verify required tools (`nasm`, `qemu-system-i386`, `gcc`, `ld`, `objcopy`, `make`).
- Install missing packages using `apt-get` (Linux only).
- Build the cross compiler using `cross2.sh` if `i686-elf-gcc` is not present.
- Add toolchain exports to your shell rc file (`ASM`, `CC`, `HOSTCC`, `LD`, `OBJCOPY`, `QEMU`).

## Notes
- The setup script is Linux-only and assumes `apt-get` is available for automatic installs.
- If you are on a different distro, install the equivalent packages manually and ensure
  `i686-elf-gcc` is in your PATH.
