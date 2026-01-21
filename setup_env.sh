#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "$0")" && pwd)"

if [ "$(uname -s)" != "Linux" ]; then
    echo "This script supports Linux only."
    exit 1
fi

tools=(
    nasm
    qemu-system-i386
    gcc
    ld
    objcopy
    make
)

missing_tools=()
for tool in "${tools[@]}"; do
    if command -v "$tool" >/dev/null 2>&1; then
        echo "Found $tool"
    else
        missing_tools+=("$tool")
    fi
done

if [ "${#missing_tools[@]}" -gt 0 ]; then
    echo "Missing tools: ${missing_tools[*]}"
    if command -v apt-get >/dev/null 2>&1; then
        packages=(
            build-essential
            nasm
            qemu-system-x86
            bison
            flex
            libgmp-dev
            libmpc-dev
            libmpfr-dev
            texinfo
            wget
        )
        missing=()
        for pkg in "${packages[@]}"; do
            dpkg -s "$pkg" >/dev/null 2>&1
            if [ $? -ne 0 ]; then
                missing+=("$pkg")
            fi
        done
        if [ "${#missing[@]}" -gt 0 ]; then
            sudo apt update
            sudo apt install -y "${missing[@]}"
        fi
        missing_tools=()
        for tool in "${tools[@]}"; do
            if command -v "$tool" >/dev/null 2>&1; then
                echo "Found $tool"
            else
                missing_tools+=("$tool")
            fi
        done
        if [ "${#missing_tools[@]}" -gt 0 ]; then
            echo "Still missing tools after install: ${missing_tools[*]}"
            exit 1
        fi
    else
        echo "apt-get not found. Install required packages manually."
        exit 1
    fi
fi

if command -v i686-elf-gcc >/dev/null 2>&1; then
    echo "Found i686-elf-gcc."
else
    if [ -x "$project_root/cross2.sh" ]; then
        "$project_root/cross2.sh"
    else
        echo "cross2.sh not found at $project_root/cross2.sh"
        exit 1
    fi
fi

rc_file="$HOME/.bashrc"
if [ -n "${ZSH_VERSION-}" ]; then
    rc_file="$HOME/.zshrc"
else
    shell_name="$(basename "${SHELL-}")"
    if [ "$shell_name" = "zsh" ]; then
        rc_file="$HOME/.zshrc"
    fi
fi

line_path='export PATH="$HOME/opt/cross/bin:$PATH"'
line_asm='export ASM=nasm'
line_cc='export CC=i686-elf-gcc'
line_hostcc='export HOSTCC=gcc'
line_ld='export LD=i686-elf-ld'
line_objcopy='export OBJCOPY=i686-elf-objcopy'
line_qemu='export QEMU=qemu-system-i386'

lines=(
    "$line_path"
    "$line_asm"
    "$line_cc"
    "$line_hostcc"
    "$line_ld"
    "$line_objcopy"
    "$line_qemu"
)
for line in "${lines[@]}"; do
    if [ -f "$rc_file" ]; then
        grep -F "$line" "$rc_file" >/dev/null 2>&1
        if [ $? -ne 0 ]; then
            echo "$line" >> "$rc_file"
        fi
    else
        echo "$line" > "$rc_file"
    fi
done

echo "Setup complete."
echo "Restart your shell or run: source \"$rc_file\""
echo "Then run: make run"
