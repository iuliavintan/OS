#!/bin/bash

set -e

#preparations

export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"
BINUTILS_VERSION=2.42
GCC_VERSION=14.2.0

echo "===Cross Compiler Setup for $TARGET===="

#checking for Linux

if [[ "$OSTYPE" != "linux-gnu"* ]]; then
  echo "This script is for Linux only."
  exit 1
fi


# Prevent overwriting an existing toolchain
if command -v $TARGET-gcc >/dev/null 2>&1; then
  echo "Found existing $TARGET-gcc. Skipping rebuild."
  $TARGET-gcc --version
  exit 0
fi

#installing dependencies

sudo apt update
sudo apt install -y build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo wget

mkdir -p ~/src
cd ~/src

#downloading binutils

echo "===Building binutils $BINUTILS_VERSION for $TARGET==="
wget -nc https://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VERSION.tar.xz
tar -xf binutils-$BINUTILS_VERSION.tar.xz

mkdir -p build-binutils
cd build-binutils
../binutils-$BINUTILS_VERSION/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make -j$(nproc) # Parallel compilation using all available CPU cores    
make install

# downloading gcc
echo "===Downloading GCC $GCC_VERSION sources==="
wget -4 -nc https://ftp.gnu.org/gnu/gcc/gcc-$GCC_VERSION/gcc-$GCC_VERSION.tar.xz

# Always extract to ensure the directory exists and is up-to-date
if tar -tf gcc-$GCC_VERSION.tar.xz >/dev/null 2>&1; then
  rm -rf gcc-$GCC_VERSION
  tar -xf gcc-$GCC_VERSION.tar.xz
else
  echo "Error: gcc-$GCC_VERSION.tar.xz is not a valid tar archive."
  exit 1
fi



#gcc
cd $HOME/src

# The $PREFIX/bin dir _must_ be in the PATH. We did that above.
which -- $TARGET-as || echo $TARGET-as is not in the PATH

mkdir -p build-gcc
cd build-gcc
../gcc-$GCC_VERSION/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c --without-headers --disable-multilib
make all-gcc -j$(nproc)
make all-target-libgcc -j$(nproc)
make install-gcc
make install-target-libgcc

cd $HOME/src

# Verifying the installation
echo "===Verifying the installation==="

if command -v $TARGET-gcc >/dev/null 2>&1; then
    echo "$TARGET-gcc is installed successfully."
    $TARGET-gcc --version
else
    echo "Error: $TARGET-gcc is not found in the PATH."
    exit 1
fi

#cleanup build directories
echo "===Cleaning up temporary built directories...==="
rm -rf ~/src/build-binutils ~/src/build-gcc

echo "===Cross compiler successfully built==="
echo "Add this to your ~/.bashrc:"
echo "export PATH=\"$PREFIX/bin:\$PATH\""

export PATH="$HOME/opt/cross/bin:$PATH"