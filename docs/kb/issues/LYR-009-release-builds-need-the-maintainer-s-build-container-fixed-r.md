---
id: LYR-009
title: Release builds need the maintainer's build container: fixed /root paths, two helper scripts missing, no script for the macOS, Android and Windows arm64 builds
type: bug
status: confirmed
severity: low
platforms: [all]
area: build
action: fix
next: Call build_boinc_target.sh by its name, add runtest.sh, make the /root paths overridable, script the macOS, Android and Windows arm64 builds, record the BOINC and compiler versions
blocked_by: []
pattern: PAT-021
aliases: []
reported: 2026-11-24
source: audit 2026-11-24 (knowledge-base pattern check)
where: tools/build_all_targets.sh:1-15 @ 8ab64f0
fixed_in:
---

## Symptom

Nobody but the maintainer can rebuild a released binary, and nobody, the maintainer included, can tell later which
BOINC library and compiler went into it.

## Cause

* `tools/build_all_targets.sh` changes to `/root` and runs `bash build_one.sh` (`:1`, `:4`) and `bash runtest.sh`
  (`:5`). The first is in the repository under another name, `tools/build_boinc_target.sh` (its usage line still
  says `build_one.sh`, `:2`); the second is not in the repository at all.
* Every script uses fixed paths of the build container, none of them overridable: the BOINC tree `/root/boinc`
  (`build_boinc_target.sh:4`, `build_gpu_linux_focal.sh:4`, `build_gpu_windows.sh:2`), the sources `/root/lyra`, the
  output `/root/out`, Windows headers in `/root/winincl`, Wine at `/usr/lib/wine/wine64` with
  `WINEPREFIX=/root/.wine64` (`build_all_targets.sh:2`, `:13`). `build_gpu_linux_focal.sh` runs inside an Ubuntu
  20.04 chroot that is not described here.
* The README names builds that have no script: macOS (Apple clang, `-mmacosx-version-min=11.0`) for the CPU and the
  arm64 GPU app, Android aarch64 (NDK), Windows arm64 (llvm-mingw in `/opt/llvm-mingw`). `build_all_targets.sh`
  builds `windows_x86.exe` but does not test it (`:14`).
* No script records the BOINC commit or the compiler and MinGW versions it used. (PAT-029 is `n/a` here only because
  nothing is downloaded: every dependency comes from the container.)

Verified by reading the scripts at 8ab64f0. The pattern's grep finds none of these (it looks for home and drive
paths); `/root` is the build container's home.

## Fix

As in `next`. Results do not change; the gain is that a fix can be built and checked on every platform by others.

## Test

In a fresh x86_64 Debian or Ubuntu container with the cross compilers, Wine and a BOINC checkout:
`BOINC=<dir> OUT=<dir> bash tools/build_all_targets.sh` builds and tests every scripted target.

## Log

- 2026-11-24: found while checking pattern PAT-021, verified at 8ab64f0 by reading the scripts.
