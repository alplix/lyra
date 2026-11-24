---
id: LYR-001
title: CPU app: the search runs in threads of its own, which suspend, CPU throttling and preemption do not stop on Windows and Android
type: bug
status: confirmed
severity: high
platforms: [windows, android]
area: boinc-lifecycle
action: fix
next: Start the CPU app with boinc_init_parallel() before any thread exists, or let the search threads wait while boinc_status.suspended is set (checked once per k)
blocked_by: []
pattern: PAT-025
aliases: []
reported: 2026-11-24
source: audit 2026-11-24 (knowledge-base pattern check)
where: symm27.c:210-243, 254, 276-288 @ 8ab64f0
fixed_in:
---

## Symptom

Not reported yet. Expected on Windows and Android: the volunteer suspends the task, or the client does (computer in
use, on battery, "use at most N % of CPU time", or the task is preempted and kept in memory), and the BOINC manager
shows it suspended, but the app keeps one core per search thread busy until the task is finished. CPU throttling has
no effect, and a preempted task keeps computing next to the task that replaced it.

## Cause

All the computing happens in threads the app starts itself. `main()` calls `boinc_init()` (`symm27.c:254`), starts
`nthreads` search threads (`:276`; 1 by default, `--nthreads` up to 64) and then only waits on a condition variable,
reports progress and writes checkpoints (`:277-288`). The search threads (`worker()`, `:210-243`) take the next chunk
of 256 k, sieve and test it, and commit finished chunks themselves. They never look at the BOINC status and only wait
when they are 1,024 chunks (`RING`) ahead of the oldest unfinished chunk, which another search thread finishes. So
nothing ties them to the thread the library suspends.

What the library does (BOINC `api/boinc_api.cpp` at fc12d39, 2026-11-04; see
[boinc-api.md](../shared/boinc-api.md#timer-signals-and-threads)):

* Windows: the timer thread calls `SuspendThread()` on the thread that called `boinc_init()` only (`:1157-1165`). The
  search threads keep running.
* Android: there is no `setitimer()`; the timer thread sends SIGALRM to that one thread with `pthread_kill()`
  (`:1357`), so only it sleeps in the signal handler (`:1511-1540`). The search threads keep running.
* Linux and macOS: the 10 Hz SIGALRM comes from `setitimer(ITIMER_REAL)` (`:1622`) and is sent to the process. The
  library's timer thread blocks it (`:1499`, `:1784-1785`); the app's threads do not. A thread that runs the handler
  blocks SIGALRM while it does, so the next signals go to other threads, and each of them sleeps in the handler while
  the task is suspended (`:1535-1539`). Expected result: after about 0.1 s per thread every thread is held. This
  follows from how the kernels deliver process signals; it was not tested under a client.

Verified by reading `symm27.c` at 8ab64f0 and the BOINC library at fc12d39; not reproduced under a BOINC client.

The GPU app starts 4 threads only to build the sieve masks (`symm27_gpu.c:118`, `symm27.c:174`), about 0.7 s on an
Apple M4 Pro (2026-11-24); its search runs in the main thread, so it is not affected beyond those first seconds.

## Fix

Proposed, one of:

* `boinc_init_parallel()` instead of `boinc_init()` in the CPU app (`symm27.c:254`), called before any thread is
  created. The library then suspends the whole process (Unix: it forks and stops the child; Windows: every thread).
  The comment at the top of `boinc_api.cpp` warns that the fork variant does not respect critical sections; the
  checkpoint goes through a temporary file (but see LYR-004), so a stop in the middle of a write costs at most that
  checkpoint. It also fits a multi-threaded plan class that passes `--nthreads`.
* Or keep `boinc_init()` and let each search thread check `boinc_status.suspended` (`boinc_get_status()`) before each
  k and wait on the condition variable while it is set; the main loop broadcasts after a resume. One load per k.

For volunteers: the task stops when asked. Results, checkpoints and credit do not change.

## Test

On Windows (or Android), run a CPU task under the client, with `--nthreads 4` and with the default 1, suspend it in the
BOINC manager and watch the process in Task Manager or Process Explorer: its CPU use must drop to about zero within a
second or two. On Linux, `top -H -p <pid>` shows whether every thread is held.

## Log

- 2026-11-24: found while checking pattern PAT-025, verified at 8ab64f0 by reading the code and the BOINC library at
  fc12d39.
