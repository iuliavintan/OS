#!/bin/bash
set -euo pipefail

export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"
BINUTILS_VERSION=2.42
GCC_VERSION=14.2.0

echo "===Cross Compiler Setup for $TARGET===="

[[ "$OSTYPE" == linux-gnu* ]] || { echo "This script is for Linux only."; exit 1; }

if command -v $TARGET-gcc >/dev/null 2>&1; then
  echo "Found existing $TARGET-gcc. Skipping rebuild."
  $TARGET-gcc --version
  exit 0
fi

sudo apt update
sudo apt install -y build-essential bison flex libgmp-dev libmpc-dev libmpfr-dev texinfo wget

mkdir -p ~/src
cd ~/src

echo "===Building binutils $BINUTILS_VERSION for $TARGET==="
wget -nc https://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VERSION.tar.xz
tar -xf binutils-$BINUTILS_VERSION.tar.xz

mkdir -p build-binutils
cd build-binutils
../binutils-$BINUTILS_VERSION/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make -j"$(nproc)"
make install

# >>> FIX: go back to src before downloading/extracting GCC <<<
cd ~/src

echo "===Downloading GCC $GCC_VERSION sources==="
wget -4 -nc https://ftp.gnu.org/gnu/gcc/gcc-$GCC_VERSION/gcc-$GCC_VERSION.tar.xz
# ensure a clean source dir
rm -rf gcc-$GCC_VERSION
tar -xf gcc-$GCC_VERSION.tar.xz

echo "===Building GCC stage-1 (C only, no headers)==="
mkdir -p build-gcc
cd build-gcc
../gcc-$GCC_VERSION/configure \
  --target=$TARGET \
  --prefix="$PREFIX" \
  --disable-nls \
  --enable-languages=c \
  --without-headers \
  --disable-multilib

make all-gcc -j"$(nproc)"
make all-target-libgcc -j"$(nproc)"
make install-gcc
make install-target-libgcc

cd ~/src

echo "===Verifying the installation==="
if command -v $TARGET-gcc >/dev/null 2>&1; then
  echo "$TARGET-gcc is installed successfully."
  $TARGET-gcc --version
else
  echo "Error: $TARGET-gcc is not found in the PATH."
  exit 1
fi

echo "===Cleaning up temporary build directories...==="
rm -rf ~/src/build-binutils ~/src/build-gcc

echo "===Cross compiler successfully built==="
echo 'Add to your ~/.bashrc:'
echo 'export PATH="$HOME/opt/cross/bin:$PATH"'
