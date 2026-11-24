---
id: LYR-012
title: At most 100,000 hits per task are kept; later hits are dropped without a message, and a checkpoint holding exactly 100,000 is rejected
type: design-note
status: documented
severity: info
platforms: [all]
area: io
action:
next:
blocked_by: []
pattern:
aliases: []
reported: 2026-11-24
source: audit 2026-11-24
where: symm27.c:185, 205, 238; symm27_gpu.c:224 @ 8ab64f0
fixed_in:
---

## Symptom

None in practice. A task with more than 100,000 central 11-tuplets would report only the first 100,000, and its
header's `hits=` would show 100,000.

## Cause

`hits` has `MAXHITS` = 100,000 slots of 160 bytes (16 MB, `symm27.c:185`, `:268`). Committing a chunk copies hits only
while `nh < MAXHITS` (`symm27.c:238`; GPU app `symm27_gpu.c:224`); the rest are dropped without a message.
`load_ckpt()` refuses a checkpoint with n >= `MAXHITS` (`symm27.c:205`), so a task that reached the cap would start
again from k0 at every restart.

Measured density on 2026-11-24 (Apple M4 Pro, 8ab64f0): 5 hits in 1,048,576 k at k = 10^12 and 7 hits in 65,535 k at
k = 1..65,535. The cap is out of reach for any work unit of a realistic size.

## Fix

None needed. If work units ever grow that far, end with an error at the cap instead of dropping hits, and accept
n == `MAXHITS` in `load_ckpt()`.

## Test

In a scratch copy, set `MAXHITS` to 2, build it and run `in` = `1 65535` (7 hits): the header shows `hits=2`.

## Log

- 2026-11-24: written down while reading the code at 8ab64f0.
