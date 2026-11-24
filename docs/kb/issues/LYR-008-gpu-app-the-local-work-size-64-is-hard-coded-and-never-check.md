---
id: LYR-008
title: GPU app: the local work size 64 is hard-coded and never checked against the device's or the kernel's limit
type: bug
status: confirmed
severity: low
platforms: [opencl]
area: gpu-runtime
action: fix
next: Read CL_KERNEL_WORK_GROUP_SIZE of each kernel (clGetKernelWorkGroupInfo is already loaded) and use the largest power of two <= min(64, limit); the kernels need no particular size
blocked_by: []
pattern: PAT-015
aliases: []
reported: 2026-11-24
source: audit 2026-11-24 (knowledge-base pattern check)
where: symm27_gpu.c:150-152, 180, 191, 195, 200, 204 @ 8ab64f0
fixed_in:
---

## Symptom

Not reported. Expected on a GPU or driver whose work-group limit for one of the kernels is below 64: the first
launch fails with `CL_INVALID_WORK_GROUP_SIZE` (-54), printed as `OpenCL hata -54 (symm27_gpu.c:191)`, and the task
ends as a computation error (exit 5, LYR-007).

## Cause

`LS` (64 unless defined at build time, `symm27_gpu.c:150-152`) is the local size of every launch (`ls`, `:180`;
launches at `:191`, `:195`, `:200`, `:204`), and every global size is rounded up to a multiple of it. Neither
`CL_DEVICE_MAX_WORK_GROUP_SIZE` nor `CL_KERNEL_WORK_GROUP_SIZE` is read; `cl_dynload.c:159` loads
`clGetKernelWorkGroupInfo`, but nothing calls it. The kernels use no `__local` memory, no barriers and no
`reqd_work_group_size`, so any local size works. A kernel's limit can be lower than the device's when it needs many
registers. On the Apple M4 Pro the app runs (2026-11-24).

Verified by reading the code at 8ab64f0; no device with a lower limit was at hand.

## Fix

After building the kernels, read `CL_KERNEL_WORK_GROUP_SIZE` for each, and use the largest power of two not above
min(64, the smallest limit); the global sizes stay multiples of it. Results do not change.

## Test

Print `CL_KERNEL_WORK_GROUP_SIZE` of the four kernels on each test device (or check `clinfo`'s "Max work group
size"); a build with `-DLS=2048` must still run after the fix on a device whose limit is 1024.

## Log

- 2026-11-24: found while checking pattern PAT-015, verified at 8ab64f0 by reading the code.
