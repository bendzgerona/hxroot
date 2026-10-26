#!/bin/sh
set -e
DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$DIR"

echo "==> Compiling libfakechroot.so using zig cc (targeting aarch64-linux-gnu)..."
zig cc -target aarch64-linux-gnu -shared -fPIC -O2 -Isrc src/*.c -o libfakechroot.so

echo "==> Verifying GLIBC symbol requirements..."
MAX_GLIBC="$(nm -D libfakechroot.so | grep -o 'GLIBC_[0-9.]*' | sort -uV | tail -n 1)"
echo "Maximum GLIBC requirement: $MAX_GLIBC"

chmod +x fakechroot hxconvert hxunconvert
ln -sf fakechroot hxroot
ln -sf libfakechroot.so libhxroot.so

echo "==> Deploying to ~/.local/{bin,lib}..."
mkdir -p "$HOME/.local/bin" "$HOME/.local/lib"
cp libfakechroot.so "$HOME/.local/lib/libfakechroot.so"
ln -sf libfakechroot.so "$HOME/.local/lib/libhxroot.so"
cp fakechroot "$HOME/.local/bin/fakechroot"
ln -sf fakechroot "$HOME/.local/bin/hxroot"
cp hxconvert "$HOME/.local/bin/hxconvert"
cp hxunconvert "$HOME/.local/bin/hxunconvert"

echo "==> Build complete: libfakechroot.so, libhxroot.so, fakechroot, and hxroot are ready."
