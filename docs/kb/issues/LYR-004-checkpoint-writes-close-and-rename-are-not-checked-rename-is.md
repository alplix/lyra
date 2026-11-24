---
id: LYR-004
title: Checkpoint writes, close and rename are not checked, rename() is used instead of boinc_rename(), and the checkpoint is reported even when it was not written
type: bug
status: confirmed
severity: low
platforms: [all]
area: checkpoint
action: fix
next: Make save_ckpt() check every write and the fclose, use boinc_rename() under USE_BOINC and return success; call boinc_checkpoint_completed() only on success, boinc_end_critical_section() otherwise
blocked_by: []
pattern: PAT-003
aliases: []
reported: 2026-11-24
source: audit 2026-11-24 (knowledge-base pattern check)
where: symm27.c:193-202, 283-286; symm27_gpu.c:227 @ 8ab64f0
fixed_in:
---

## Symptom

Not reported. Expected on a full disk, under antivirus that holds files open, or when a Windows host is killed at the
wrong moment: the task loses its progress and starts again from the first k. Results are not affected (the output is
written once at the end).

## Cause

`save_ckpt()` (`symm27.c:193-202`) has the right shape (write `<checkpoint>.tmp` in binary mode, then rename it over
the checkpoint), but:

* (b) neither the `fprintf` calls nor the `fclose` are checked (`:194-197`): on a full disk a cut file is renamed
  over a good checkpoint; `load_ckpt()` then rejects it (fewer hit lines than its header says, `:206`) and the task
  starts again from k0;
* (c) on Windows it removes the old checkpoint and then calls `rename()` (`:198-201`): a kill between the two leaves
  no checkpoint (the `.tmp` file is never read), and a failed `rename()` is not noticed. `boinc_rename()` replaces
  the file in one step and retries while it is busy ([boinc-api.md](../shared/boinc-api.md#files));
* (d) when `fopen` fails, `save_ckpt()` returns silently (`:194`) and both callers report the checkpoint to the client
  all the same (`symm27.c:283-286`, `symm27_gpu.c:227`).

Verified by reading the code at 8ab64f0. A kill and restart with a good checkpoint resumes correctly (measured
2026-11-24, AGENTS.md "Checkpoint/resume contract").

## Fix

Let `save_ckpt()` return 1 only when every write, the `fclose` and the rename succeeded; use `boinc_rename()` when
built with `USE_BOINC` (plain `rename()` stays for standalone builds). The callers call
`boinc_checkpoint_completed()` only on success and `boinc_end_critical_section()` otherwise, so the critical section
is still closed (PAT-001).

## Test

With a stub BOINC library that logs its calls (kampylos has one in `tests/boinc_stub/`), make the run directory
read-only during a run: no `boinc_checkpoint_completed()` may follow a failed write, and the run must go on.

## Log

- 2026-11-24: found while checking pattern PAT-003, verified at 8ab64f0 by reading the code.
