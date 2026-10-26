# libfakechroot (hxroot derivative)

High-performance, minimal fakechroot replacement derived from `hxroot`, engineered for Termux on Android aarch64.

## Problem Solved

Standard upstream `fakechroot` (dex4er) fails in modern environments:
- Heavyweight perl script wrappers and brittle relocation engines.
- Panics on modern glibc dynamic linker mechanisms and symbol mismatches.
- Fails on Android storage because it lacks link2symlink / fakehardlink support.
- Blocks `debootstrap --variant=fakechroot` from completing package installation.

## Key Architecture

`libfakechroot` combines the speed of `LD_PRELOAD` interception with `hxroot`'s container capabilities:
- **Zero Overhead**: Written in lightweight C; compiled in <1 second via `zig cc -target aarch64-linux-gnu`.
- **Low Symbol Footprint**: Maximum symbol requirement capped at `GLIBC_2.26`. Runs on Debian Sid, Debian 12 (bookworm), Ubuntu, Arch, and Chainguard with zero symbol panics.
- **Link2Symlink Hardlink Emulation**: Automatically converts hard links into relative symlinks (`HxL2s=1`), preventing dpkg extraction errors on Android filesystem boundaries.
- **Dynamic Linker Relocation**: Automatically expands ELF interpreters within the rootfs (`$FAKECHROOT_BASE/lib/ld-linux-aarch64.so.1`) or falls back safely to host glibc loaders without crashing.
- **Bionic Linker Awareness**: Bypasses `--argv0` argument injection when invoking Android's native `/system/bin/linker64` or `/system/lib64/ld-android.so`.
- **chroot Interception**: Intercepts `chroot(path)` syscalls at runtime, dynamically updating `FAKECHROOT_BASE` without requiring root privileges.
- **Fake Root Identity**: Simulates `uid=0`, `gid=0` across `getuid`, `geteuid`, `getgid`, `getresuid`, and `getresgid`.

## Environment Variables

- `FAKECHROOT`: Set to `true` to enable interception.
- `FAKECHROOT_BASE`: Absolute path to the fake chroot rootfs.
- `FAKECHROOT_EXCLUDE_PATH`: Colon-separated list of paths excluded from path redirection (defaults to `/dev:/proc:/sys`).
- `FAKECHROOT_DEBUG`: Set to `1` to output detailed syscall rewrite diagnostics.

## Quick Build

```sh
cd ~/ghq/github.com/bendzgerona/bai/devices/bai.bphone/libfakechroot
./build.sh
```

## Usage Example

```sh
# Run command inside Debian container
./fakechroot -b ~/debian /bin/bash -c "id; pwd; ls -la /"

# Use with debootstrap
./fakechroot debootstrap --variant=fakechroot sid /path/to/target http://deb.debian.org/debian
```
