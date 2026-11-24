---
id: LYR-011
title: Verification above 3.3e24 is probabilistic (20 fixed Miller-Rabin bases), and the search runs above that bound
type: design-note
status: documented
severity: info
platforms: [all]
area: science
action:
next:
blocked_by: []
pattern:
aliases: []
reported: 2026-11-24
source: audit 2026-11-24
where: symm27.c:101-117 @ 8ab64f0
fixed_in:
---

## Symptom

None. Recorded so that the comment at `symm27.c:101` is not taken for a bug, and so that it is clear what "verified"
means for a hit.

## Cause

`isp()` (`symm27.c:101-117`) divides by the 20 primes up to 71 and then runs Miller-Rabin with those 20 bases. With
the first 13 prime bases the test is proven correct below 3.317e24; above that no bound is proven for any fixed set of
bases, so a composite that is a strong pseudoprime to all 20 bases would pass. None is known, and for a number that
has also passed the sieve and the base-2 Fermat test the chance is negligible, as the comment and the README say. At
k = 10^12 the numbers are about 7.4e24 (a hit found there on 2026-11-24: 7420743117229754604142259), so the search's
hits are probable primes. `test_arith.c` checks `isp()` against GMP and against known strong pseudoprimes up to
3.3e24; it passed on 2026-11-24 (macOS arm64, with and without `LYRA_NO_INT128`).

## Fix

None needed. If proven primes are wanted, the few hits can be proven on the server side (ECPP, or BPSW as an
independent second test).

## Test

`test_arith` (needs GMP), see AGENTS.md "Tests".

## Log

- 2026-11-24: written down while reading the verification code at 8ab64f0.
