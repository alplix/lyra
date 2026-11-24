---
id: LYR-003
title: Write errors on out are not checked before the checkpoint is removed and boinc_finish(0) is called
type: bug
status: confirmed
severity: low
platforms: [all]
area: io
action: fix
next: In both apps check fprintf/ferror and fclose on out; on failure keep the checkpoint and end without boinc_finish(0); remove the checkpoint only after a good close
blocked_by: []
pattern: PAT-006
aliases: []
reported: 2026-11-24
source: audit 2026-11-24 (knowledge-base pattern check)
where: symm27.c:291-296; symm27_gpu.c:236-241 @ 8ab64f0
fixed_in:
---

## Symptom

Not reported. Expected on a full disk or a quota: the task ends with status 0 and uploads an `out` with a cut header
or missing hit lines, which the validator rejects or, worse, accepts with hits missing.

## Cause

Both apps write `out` once, at the end: `fopen(outp, "wb")`, the header `fprintf`, `write_hits()`
(`symm27.c:143-150`) and `fclose`. No result is checked; then the checkpoint is removed and `boinc_finish(0)` follows
(`symm27.c:291-296`; `symm27_gpu.c:236-241`). Because the checkpoint is already gone, a restart after a failed write
would have to begin again from k0. When `out` cannot be created at all, the CPU app returns 2 from `main()` without
`boinc_finish()` (the client reports exit code 2 as a computation error) and the GPU app calls `boinc_finish(2)`.

Verified by reading the code at 8ab64f0.

## Fix

Check the `fprintf` results or `ferror(fo)` before closing, and the `fclose` result. On failure print the
`strerror(errno)` text, keep the checkpoint and end with an error status (or a temporary exit, so that the restart
writes `out` again from the checkpoint). Remove the checkpoint only after `out` was closed without error. Nothing
changes for a successful run.

## Test

On Linux, in a standalone run directory, `ln -s /dev/full out` before the run: the run must not end with status 0.

## Log

- 2026-11-24: found while checking pattern PAT-006, verified at 8ab64f0 by reading the code.
