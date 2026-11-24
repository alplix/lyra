---
id: LYR-007
title: Should OpenCL start-up and run-time failures end in a temporary exit with a retry limit instead of a computation error?
type: question
status: new
severity: medium
platforms: [opencl]
area: gpu-runtime
action: decide
next: Decide: may a missing driver or GPU (exit 3, 4) and an OpenCL error during the run (5) end in boinc_temporary_exit() with a retry limit instead of a computation error?
blocked_by: []
pattern: PAT-013
aliases: []
reported: 2026-11-24
source: audit 2026-11-24 (knowledge-base pattern check)
where: symm27_gpu.c:32-39, 124-128, 148, 212 @ 8ab64f0
fixed_in:
---

## Symptom

Open question for the maintainers. Today every failure of the GPU app ends the task with `boinc_finish(code)`
(`LYRA_EXIT`, `symm27_gpu.c:32-36`), which the client reports as a computation error:

| code | when | where |
|---|---|---|
| 2 | `out` cannot be created | `:236` |
| 3 | no OpenCL library, or no platform | `:124-125` |
| 4 | no GPU of the expected vendor, or `--device` beyond the list | `:128` |
| 5 | any failed OpenCL call (`CK`) | `:39` |
| 6 | the kernels do not build | `:148` |
| 7 | the candidate buffers overflow even at the smallest batch | `:212` |
| 8 | internal: too many prime segments | `:179` |

## Cause

Following pattern PAT-013: codes 3, 4 and 6 are "no usable device or initialisation failed"; 5 is a run-time error
(a driver reset or timeout, the GPU taken by another program); 7 is deterministic for the work unit (the same
survivors on every device) and 8 is a bug. A temporary exit would let the client retry the task on the same host
after the condition cleared (driver update, reboot) instead of reporting an error and sending the work unit to another
host. The client aborts a task after 100 temporary exits without a checkpoint it has seen
([boinc-api.md](../shared/boinc-api.md#exit-paths), PAT-030), so a retry limit is needed either way.

Read at 8ab64f0.

## Fix

Waiting for the decision. If yes: `boinc_temporary_exit()` with a delay of some minutes and the reason for 3, 4, 5
and 6, with a counter kept in the slot directory and reset when a checkpoint is written, and `boinc_finish()` with an
error code for 7 and 8 as now.

## Test

On Linux, run a BOINC build under a client with the vendor's ICD file made unreadable (or `OCL_ICD_VENDORS` pointing
at an empty directory): the task must wait and retry with the reason shown, not end as a computation error.

## Log

- 2026-11-24: recorded while checking pattern PAT-013 at 8ab64f0.
