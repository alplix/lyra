---
id: LYR-005
title: GPU app: the device comes only from --device; gpu_opencl_dev_index in init_data is never read
type: bug
status: confirmed
severity: medium
platforms: [opencl]
area: device-selection
action: fix
next: Read gpu_opencl_dev_index from init_data (boinc_get_init_data) and use it when >= 0, on the list BOINC counts (LYR-006); keep --device only for standalone runs
blocked_by: []
pattern: PAT-011
aliases: []
reported: 2026-11-24
source: audit 2026-11-24 (knowledge-base pattern check)
where: symm27_gpu.c:104-105, 127-129 @ 8ab64f0
fixed_in:
---

## Symptom

Not reported. Expected on hosts with more than one OpenCL GPU: a task runs on another GPU than the one the client
assigned, so two tasks can share one GPU while another stays idle. The stderr line `GPU: <name>` shows which device
was taken.

## Cause

The device index is `atoi()` of `--device` (default 0, `symm27_gpu.c:105`); `init_data` is never read (no
`boinc_get_init_data`, `gpu_opencl_dev_index` or `gpu_device_num` in the repository). The client appends
`--device <gpu_device_num>` to the command line of GPU apps, and for OpenCL that number can differ from
`gpu_opencl_dev_index` ([boinc-api.md](../shared/boinc-api.md#gpu-assignment-init_dataxml)). The index is then
applied to the app's own device list, which is built differently from BOINC's (LYR-006).

Verified by reading the code at 8ab64f0.

## Fix

Under `USE_BOINC`, read `gpu_opencl_dev_index` and use it when it is >= 0; keep `--device` as the fallback for
standalone runs. `boinc_get_opencl_ids()` from the BOINC library does this lookup and the platform choice of LYR-006
in one call. Volunteers' tasks then run on the GPU the client scheduled.

## Test

On a host with two GPUs of one vendor, run two tasks under the client and check with the vendor's tool (`nvidia-smi`,
`radeontop`) that each GPU has one; compare with the `GPU:` lines in the two stderr files.

## Log

- 2026-11-24: found while checking pattern PAT-011, verified at 8ab64f0 by reading the code.
