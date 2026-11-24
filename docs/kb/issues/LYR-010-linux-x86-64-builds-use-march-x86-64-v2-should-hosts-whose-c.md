---
id: LYR-010
title: Linux x86_64 builds use -march=x86-64-v2: should hosts whose CPU lacks SSE4.2 or POPCNT get them?
type: question
status: new
severity: low
platforms: [linux, x86_64]
area: build
action: decide
next: Decide: keep -march=x86-64-v2 for Linux x86_64 and let the plan class require SSE4.2 and POPCNT, or build those apps for baseline x86-64 like the Windows ones
blocked_by: []
pattern:
aliases: []
reported: 2026-11-24
source: audit 2026-11-24
where: tools/build_all_targets.sh:6; tools/build_gpu_linux_focal.sh:11 @ 8ab64f0
fixed_in:
---

## Symptom

Open question for the maintainers. Expected: on a Linux host with an x86_64 CPU from before about 2009 (Intel Core 2,
AMD before Bulldozer), the app can stop at once with SIGILL (illegal instruction), while the Windows app runs on the
same CPU.

## Cause

`-march=x86-64-v2` lets the compiler use SSE3, SSSE3, SSE4.1, SSE4.2, POPCNT and CMPXCHG16B anywhere. The Linux
x86_64 CPU build (`tools/build_all_targets.sh:6`) and the Linux GPU builds (`tools/build_gpu_linux_focal.sh:11`) use
it; the Windows x86_64 builds do not (`build_all_targets.sh:13`, `build_gpu_windows.sh:12`), and Linux i686 uses
`-msse2 -mfpmath=sse` (`:7`). Whether the binaries contain such instructions in code that runs was not checked
(`objdump -d` on a release binary). The client reports the CPU's feature flags to the scheduler; whether the plan
class requires them is set on the server, not in this repository.

Read at 8ab64f0.

## Fix

Waiting for the decision. Baseline x86-64 costs whatever speed v2 gives here (not measured); the alternative is a
plan-class requirement, written down here.

## Test

Run the Linux x86_64 binary under `qemu-x86_64 -cpu core2duo` (a CPU model without SSE4.2): it must give the same
output as on a current CPU, or the plan class must keep such hosts away.

## Log

- 2026-11-24: recorded while reading the build scripts at 8ab64f0.
