# Pattern ledger

One row per cross-repository pattern ([shared/patterns](shared/patterns)): has this repository been checked for it,
at which commit, and what was found. `kb.py sync` adds an `unchecked` row whenever a new pattern arrives; checking it
means following the pattern's "How to check a repository" section and filling in the row. Status values:
[shared/vocabulary.md](shared/vocabulary.md#ledger). A row that is `affected` or `fixed` names the local issue, and that
issue names the pattern in its `pattern:` field; its Evidence says where, in one clause, and the issue has the rest.
Write rows with `kb.py ledger` (date and commit filled in) rather than by hand.

When code that a row describes changes a lot, check the pattern again and update Checked, Commit and Evidence.

| Pattern | Status | Checked | Commit | Issue | Evidence |
|---|---|---|---|---|---|
| PAT-001 | not-affected | 2026-11-24 | 8ab64f0 |  | symm27.c:283-286 (CPU, at most one boinc_time_to_checkpoint() per main-loop pass, inside the if) and symm27_gpu.c:227 (GPU, once per batch): each 1 is followed by save_ckpt() and one boinc_checkpoint_completed(); no begin/end_critical_section |
| PAT-002 | n/a | 2026-11-24 | 8ab64f0 |  | out is written once, after the last chunk (symm27.c:291-294, symm27_gpu.c:236-239); the checkpoint holds the hits found so far (symm27.c:195-196), so nothing is appended on resume |
| PAT-003 | affected | 2026-11-24 | 8ab64f0 | LYR-004 | save_ckpt() symm27.c:193-202: (b) writes and fclose unchecked, (c) remove() + rename() on Windows, (d) fopen failure returns silently and the callers report the checkpoint anyway |
| PAT-004 | not-affected | 2026-11-24 | 8ab64f0 |  | gated by boinc_time_to_checkpoint() in both apps (symm27.c:244-246, 283; symm27_gpu.c:227); the own 10 s cadence exists only in standalone builds without USE_BOINC (symm27.c:248, symm27_gpu.c:229) |
| PAT-005 | not-affected | 2026-11-24 | 8ab64f0 |  | load_ckpt() treats any open or parse failure as no checkpoint (symm27.c:203-205) and the task starts again from k0, but out is only created at the end (symm27.c:291), so no finished work is truncated; the cost is the lost progress |
| PAT-006 | affected | 2026-11-24 | 8ab64f0 | LYR-003 | symm27.c:291-296 and symm27_gpu.c:236-241: fprintf/write_hits/fclose on out unchecked, checkpoint removed, then boinc_finish(0) |
| PAT-007 | not-affected | 2026-11-24 | 8ab64f0 |  | CPU: boinc_fraction_done() at least once a second from the main loop (symm27.c:279-281), after setup(), which took 2.2 s on one core and 0.7 s with 4 threads (Apple M4 Pro, 2026-11-24); GPU: after each batch (symm27_gpu.c:226), about 22 ms per batch on the M4 Pro |
| PAT-008 | n/a | 2026-11-24 | 8ab64f0 |  | no boinc_get_status, quit_request, abort_request or direct_process_action in the repository; boinc_init() with default options (symm27.c:254, symm27_gpu.c:102), the library handles quit and abort |
| PAT-009 | n/a | 2026-11-24 | 8ab64f0 |  | no trickle messages: no boinc_send_trickle_up in the repository |
| PAT-010 | not-affected | 2026-11-24 | 8ab64f0 |  | the BOINC thread's only wait is pthread_cond_timedwait() with an absolute deadline in a loop that re-checks its condition (symm27.c:278-279); no sleep, poll, select or device read; files are regular files |
| PAT-011 | affected | 2026-11-24 | 8ab64f0 | LYR-005 | symm27_gpu.c:105: the device only from --device (default 0); init_data is never read |
| PAT-012 | affected | 2026-11-24 | 8ab64f0 | LYR-006 | symm27_gpu.c:127: the GPUs of every platform that passes the vendor filter go into one list; count and list both CL_DEVICE_TYPE_GPU, no fallback to other types |
| PAT-013 | affected | 2026-11-24 | 8ab64f0 | LYR-007 | symm27_gpu.c:33-39, 124-128, 148: no driver, no GPU, a failed build and any failed OpenCL call end in boinc_finish(3..6) |
| PAT-014 | n/a | 2026-11-24 | 8ab64f0 |  | the kernels (symm27_gpu.c:41-93) use only integer types (uint, ulong, 32-bit limbs); no double, no cl_khr_fp64 |
| PAT-015 | affected | 2026-11-24 | 8ab64f0 | LYR-008 | symm27_gpu.c:150-152, 180: local size 64 for every launch, CL_KERNEL_WORK_GROUP_SIZE and CL_DEVICE_MAX_WORK_GROUP_SIZE never read |
| PAT-016 | not-affected | 2026-11-24 | 8ab64f0 |  | 8 launches per batch of at most 2,048 k (symm27_gpu.c:133-137, 184-207); measured on an Apple M4 Pro 2026-11-24 at k=1e12: 512 batches in 11.08 s, so all launches of one batch together take about 22 ms. A GPU 40x slower stays under 1 s per batch; slower GPUs not measured |
| PAT-017 | n/a | 2026-11-24 | 8ab64f0 |  | no Android GPU build (README: GPU apps for Linux x86_64, Windows x86_64, macOS arm64); cl_dynload.c is planodia's copy with the namespace fallback (cl_dynload.c:29-85, 119-121) in case one is added |
| PAT-018 | not-affected | 2026-11-24 | 8ab64f0 |  | integer arithmetic only (floating point only sizes GPU batches and reports progress); measured identical files 2026-11-24: CPU app 1 vs 8 and 10 threads, Apple OpenCL GPU app, Windows x86_64 MinGW-w64 GCC 16 build under Wine 8, for k=1..8191, 1..65535 and 1e12..1e12+1048575 |
| PAT-019 | not-affected | 2026-11-24 | 8ab64f0 |  | out "wb" (symm27.c:291, symm27_gpu.c:236), checkpoint "wb"/"rb" (symm27.c:194, 204); in is read with "r" and fscanf of two integers, which accepts CRLF (symm27.c:262-263) |
| PAT-020 | not-affected | 2026-11-24 | 8ab64f0 |  | C with pthreads (winpthreads on MinGW); the mask and search threads exit (symm27.c:174, 276, 290), but there is no thread_local or __thread in the repository and the threads call no C++ code |
| PAT-021 | affected | 2026-11-24 | 8ab64f0 | LYR-009 | tools/*.sh: fixed /root paths of the build container (/root/boinc, /root/lyra, /root/out, /root/winincl), not overridable; the pattern's grep has no hits |
| PAT-022 | not-affected | 2026-11-24 | 8ab64f0 |  | CPU Linux builds fully static (tools/build_boinc_target.sh:18); the GPU Linux build runs in an Ubuntu 20.04 chroot (glibc 2.31) and prints the newest GLIBC symbol version it needs (tools/build_gpu_linux_focal.sh:14), README: 2.29 |
| PAT-023 | n/a | 2026-11-24 | 8ab64f0 |  | no data download: the residues are computed at start-up (gen_dob(), symm27.c:29-39); gendob.py only writes a reference file |
| PAT-024 | n/a | 2026-11-24 | 8ab64f0 |  | no CUDA build; OpenCL kernels are compiled by the driver at run time |
| PAT-025 | affected | 2026-11-24 | 8ab64f0 | LYR-001 | symm27.c:276: the CPU app computes every k in its own pthreads, which never check the BOINC status; no boinc_init_parallel() |
| PAT-026 | not-affected | 2026-11-24 | 8ab64f0 |  | qsort() in gen_dob() sorts distinct residues (symm27.c:38) and in the GPU app distinct (k, residue index) pairs (symm27_gpu.c:97, 219): no ties |
| PAT-027 | not-affected | 2026-11-24 | 8ab64f0 |  | every clEnqueueWriteBuffer/ReadBuffer/NDRangeKernel, clSetKernelArg, clFlush and clWaitForEvents goes through CK() and ends the task on error (symm27_gpu.c:161, 190-207, 210); only the clFinish() before a retry at :214 is unchecked |
| PAT-028 | not-affected | 2026-11-24 | 8ab64f0 |  | guards USE_BOINC (every script passes -DUSE_BOINC: build_boinc_target.sh:16, build_gpu_linux_focal.sh:11, build_gpu_windows.sh:12), EXPECTED_GPU_VENDOR (default empty = any vendor), LYRA_NO_INT128 (same results); macOS, Android and Windows arm64 builds have no script to check (LYR-009) |
| PAT-029 | n/a | 2026-11-24 | 8ab64f0 |  | the scripts fetch nothing; the BOINC tree, compilers, MinGW headers and the Ubuntu 20.04 chroot come from the build container (AGENTS.md, Building); no version is recorded (LYR-009) |
| PAT-030 | n/a | 2026-11-24 | 8ab64f0 |  | no boinc_temporary_exit() and no deliberate exit without boinc_finish() except the CPU app's return 2 when out cannot be created (symm27.c:291), a computation error |
| PAT-031 | n/a | 2026-11-24 | 8ab64f0 |  | no thread_local, __thread or _Thread_local in the repository (git grep) |
| PAT-032 | not-affected | 2026-11-24 | 8ab64f0 |  | no allocation per k: each search thread allocates its buffers once (symm27.c:211), hits are realloc'd only when a hit is found (:232); the GPU app allocates one candidate array per batch (symm27_gpu.c:217). Not profiled |
| PAT-033 | not-affected | 2026-11-24 | 8ab64f0 |  | two batch slots: batch n+1 is enqueued and flushed before the host waits for batch n and verifies its candidates (symm27_gpu.c:184-215); the GPU app used about 0.2 s of CPU time for 1,048,576 k (11 s of GPU time) on an Apple M4 Pro, 2026-11-24 |
| PAT-034 | n/a | 2026-11-24 | 8ab64f0 |  | no work generator or work-unit template here (git grep rsc_fpops: no hits); the bound is set outside this repository, and the repository has no task-length data from volunteers |
