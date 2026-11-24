---
id: LYR-002
title: GPU app: the Fermat kernel is exact only below 2^95, so from k = 5338293918674499 its output differs from the CPU app's, and nothing checks the range
type: bug
status: confirmed
severity: medium
platforms: [opencl]
area: determinism
action: fix
next: Refuse k1 >= 2^95 / 37# (5338293918674499) at start-up in symm27_gpu.c with a clear message, and give the limit to the work generator; or widen the kernel to 128 bits
blocked_by: []
pattern:
aliases: []
reported: 2026-11-24
source: audit 2026-11-24, measured 2026-11-24
where: symm27_gpu.c:63-92, 110-116 @ 8ab64f0
fixed_in:
---

## Symptom

Measured on 2026-11-24 on an Apple M4 Pro (macOS 27.0, Apple clang 21.0.0, Apple OpenCL), CPU app and GPU app built
from 8ab64f0 for standalone runs, 256 k per range:

| k range | CPU app | GPU app |
|---|---|---|
| 5338293918674000 .. 5338293918674255 | surv=1304 fermat1=349 hits=0 | identical file |
| 6000000000000000 .. 6000000000000255 | surv=1300 fermat1=343 hits=0 | surv=1300 fermat1=1300 hits=0 |
| 11000000000000000 .. 11000000000000255 | surv=1259 fermat1=341 hits=0 | surv=1259 fermat1=35 hits=0 |

Between 2^95 and 2^96 every survivor passes the GPU's Fermat test: `fermat1` is wrong and the host verifies every
survivor (the hits stay right, at a large CPU cost). Above 2^96 the GPU tests the wrong numbers, rejects real
candidates, and hits can be lost. Either way the result file differs from the CPU app's for the same work unit.

Not reported by volunteers. Whether the search ever reaches these k depends on the work generator, which is not in
this repository; at k = 10^12 (X about 7.4e24) the two apps give identical files (AGENTS.md, "Tests").

## Cause

The kernel holds X = k * 37# + dob in three 32-bit words (`symm27_gpu.c:88-89`): `(uint)hi` drops the bits of
k * 37# above 2^96. Its Montgomery code doubles a residue with `dbl3()` (`:65`), which shifts the top word left and
loses bit 95, so `R mod n` (`:79`) and the doubling step of the exponentiation (`:83`) are wrong once n >= 2^95.
X + 168 stays below 2^95 for k < 2^95 / 37# = 5,338,293,918,674,499. `main()` reads k0 and k1 (`:110-116`) without an
upper bound. The CPU app uses 128-bit Montgomery arithmetic (n < 2^127, `symm27.c:73-99`), exact for every 64-bit k.

Verified by reading the kernel at 8ab64f0 and by the runs above.

## Fix

Proposed: after reading `in` (`symm27_gpu.c:116`), check k1 < 2^95 / 37# (computed from `P37`, not written as a
number) and otherwise stop with a clear message, as for an unreadable `in` (`boinc_finish(1)`); write the limit where
the work generator is configured. Widening the kernel to four 32-bit words would lift the limit, at a speed cost that
is not measured. Work units below the limit are not affected by either.

## Test

Build both apps, run `in` = `6000000000000000 6000000000000255` through each and `cmp` the outputs: they differ at
8ab64f0. After the fix the GPU app must refuse that range and still give a file identical to the CPU app's for
`5338293918674000 5338293918674255`.

## Log

- 2026-11-24: found while reading the kernel; measured on an Apple M4 Pro at 8ab64f0.
