---
id: LYR-006
title: GPU app: the GPUs of every platform that passes the vendor filter are put in one list, while BOINC's index counts within one platform
type: bug
status: confirmed
severity: low
platforms: [opencl]
area: device-selection
action: fix
next: Choose one platform (the one whose vendor matches gpu_type) and index its CL_DEVICE_TYPE_GPU devices, as boinc_get_opencl_ids() does
blocked_by: []
pattern: PAT-012
aliases: []
reported: 2026-11-24
source: audit 2026-11-24 (knowledge-base pattern check)
where: symm27_gpu.c:125-129 @ 8ab64f0
fixed_in:
---

## Symptom

Not reported. Expected on Linux and Windows hosts where two OpenCL platforms of the same vendor are installed: the
index the client gives (LYR-005) points at another device, and the same GPU can appear twice in the app's list.

## Cause

The loop at `symm27_gpu.c:127` appends the `CL_DEVICE_TYPE_GPU` devices of every platform for which
`cl_platform_matches_vendor()` is true (up to 16), and the index selects from that combined list (`:128-129`). BOINC
counts GPU devices within the one platform whose vendor matches `gpu_type`
([boinc-api.md](../shared/boinc-api.md#gpu-assignment-init_dataxml)). The two lists differ when more than one platform
passes the filter. `cl_platform_matches_vendor()` matches by the devices' PCI vendor id first
(`cl_dynload.c:196-216`), so a Mesa platform (rusticl or Clover, vendor "Mesa/X.org") that drives the same AMD or
Intel GPU passes next to the vendor's own platform; so do two runtimes of one vendor installed side by side. The
count and the list both use `CL_DEVICE_TYPE_GPU`, and there is no fallback to other device types, so forms (b) and
(c) of the pattern do not apply.

Verified by reading the code at 8ab64f0; not seen on a host.

## Fix

Use the first platform that passes the filter (or, under BOINC, the one `boinc_get_opencl_ids()` picks for
`gpu_type`) and index only its GPU devices. Together with LYR-005.

## Test

On a Linux host with an AMD GPU, ROCm and Mesa's rusticl (`RUSTICL_ENABLE=radeonsi`), `clinfo -l` lists the GPU under
two platforms; the app's `--device 1` must not pick the same GPU through the second platform.

## Log

- 2026-11-24: found while checking pattern PAT-012, verified at 8ab64f0 by reading the code.
