# Working on Lyra (notes for AI agents and new contributors)

Lyra is the prime-tuplet search of the bitboinc BOINC project: it looks for symmetric 27-tuplets of consecutive
primes, 27 consecutive primes whose gaps read the same forwards and backwards, spanning 432. One source file,
`symm27.c`, is the CPU application; `symm27_gpu.c` includes it and adds the OpenCL application. Both must write the
same result file, byte for byte. [README.md](README.md) explains the method. The code's comments and messages are in
Turkish, written in ASCII. Written 2026-11-24 against commit 8ab64f0; file:line references below refer to that commit.

## What one work unit is

Input file `in`: two numbers, `kstart kend` (inclusive, 64-bit). For every k in that range and every one of the
286,720 residues `dob` (the residues modulo 37# = 7,420,738,134,810 that are compatible with the 27-pattern, built at
start-up by `gen_dob()`, `symm27.c:29-39`), X = k * 37# + dob is a candidate start of the central 11-tuplet (offsets
0, 30, ..., 168; the 27-tuplet would start at X - 132). Per k:

1. Stage 1: a bit-mask sieve over all residues with the primes 41 to 199 (`PBIT` 200, 34 primes). One mask per prime
   and residue class of k * 37# is built once (`setup()`, `build()`, `symm27.c:154-176`) and the masks are ANDed per
   k (`worker()`, `:220-222`).
2. Stage 2: the survivors against the primes 211 to 19,997 (`PMAX` 20000), with a multiply-high reduction
   (`:224-227`).
3. Stage 3: Fermat base 2 on all 11 positions (`fermat2()`, 128-bit Montgomery, `:94-99`), then `verify()`
   (`:130-141`): Miller-Rabin with 20 bases on each position (`isp()`, LYR-011), no other prime between them
   (`consecutive()`), and the symmetric extension to 13, 15, ..., 27 primes.

Output `out`, written once at the end (`write_hits()`, `:143-150`):

```
symm27 v2 k=[k0,k1] dobs=286720 surv=<sieve survivors> fermat1=<survivors whose X passes Fermat> hits=<n>
<X>,                         one line per central 11-tuplet found, ascending
13-tuplet                    then, for each length 13..27 that occurs, a heading and
<start>: 0, 12, 42, ...      one line per extension: its first number and the offsets
```

Measured on 2026-11-24 on an Apple M4 Pro with the BOINC client running (load average about 16; single runs, not
benchmarks): at k = 10^12 (X about 7.4e24) a k has about 5.0 sieve survivors and 1.5 Fermat passes on its first
position, and 1,048,576 k gave 5 hits, one of them a 13-tuplet. That range took 66 s on 10 threads of the CPU app
(291 s of CPU time) and 11.1 s on the M4 Pro's GPU. The CPU app needs about 155 MB (maximum resident set).

## Layout

| Path | What |
|---|---|
| `symm27.c` | the CPU application and the shared core: residues (`gen_dob()`), 128-bit arithmetic (`n128`, `mul64()`, Montgomery `mmul()`), `fermat2()`, `isp()`, `verify()`, the sieve tables (`setup()`), the checkpoint (`save_ckpt()`, `load_ckpt()`), the search threads (`worker()`) and `main()` |
| `symm27_gpu.c` | the OpenCL application: includes `symm27.c` with its `main` renamed (`:5-7`); kernel source `KSRC` (`:41-93`: `mkctab`, `sieve1`, `seg`, 96-bit `fermat`), batch size from GPU memory, the double-buffered launch loop (`:182-233`); hits are verified on the host with `verify()` |
| `cl_dynload.c`, `.h` | run-time OpenCL loader (dlopen / LoadLibrary, Android linker namespaces) and the vendor filters `cl_platform_matches_vendor()`, `cl_pick_vendor_device()`; a copy shared with other bitboinc apps (`docs/kb/kb.conf`; identical to planodia's at 8ab64f0). Its comments name files of other apps. |
| `test_verify.c` | unit tests of `verify_gen()` on small known tuplets |
| `test_arith.c` | cross-check of the 128-bit arithmetic, `fermat2()` and `isp()` against GMP (test only; needs GMP) |
| `gendob.py` | reference generator of the residues (writes `dob11.bin`) |
| `Makefile` | `standalone` (CPU), `boinc` (CPU with `BOINC_DIR=`), `gpu` (OpenCL, standalone) |
| `tools/` | release build scripts for the maintainer's build container (see Building) |
| `docs/kb/` | knowledge base (below) |

No vendored third-party code. The BOINC library and the OpenCL headers come from the build machines.

## Invariants (break one and results become silently wrong)

- **The same work unit gives the same bytes**, whatever the thread count, CPU or GPU, platform or GPU batch size. The
  CPU app commits chunks of 256 k (`CH`) strictly in order (`symm27.c:235-240`), the GPU app sorts each batch's
  candidates by k and residue index (`symm27_gpu.c:219`), and everything that reaches the file is integer arithmetic.
  Measured identical on 2026-11-24 (see Tests). The validator (not in this repository) can therefore compare bytes,
  and any change to a header counter (`surv`, `fermat1`) or a line is a result change, even when the hits stay the
  same.
- **The tables exist more than once:** `pt11`, `pt27` and the offset 132 in `symm27.c:20-21`, again in the kernel
  (`PT`, `symm27_gpu.c:43`) and in `gendob.py`; `P37` in `symm27.c:19` and in the kernel (`:42`). A change must reach
  every copy. `gen_dob()` must give the same sorted residues as `gendob.py` (a build with `-DDUMPDOB` writes them to
  `dob_c.bin` and stops).
- **The sieve bounds decide `surv`:** `PMAX` (20000) and the start at 41 set which candidates reach the Fermat stage,
  so they change the header. `PBIT` only splits the primes between stage 1 and stage 2.
- **`APPVER`** (2) is written into the header and the checkpoint and checked on resume (`symm27.c:205`): change it
  together with the output or checkpoint format.
- **Number ranges:** the CPU app's Montgomery code needs n < 2^127, true for every 64-bit k (X < 2^107). The GPU
  kernel works on 96 bits and is exact only for X < 2^95, that is k < 5,338,293,918,674,499; above that its file
  differs from the CPU app's and nothing checks the range (LYR-002, measured).
- **`out` and the checkpoint are binary** (`"wb"`): the same bytes on every OS, `\n` line ends.
- **At most 100,000 hits per task** (`MAXHITS`, LYR-012), far above the measured density.

## Checkpoint/resume contract

What the code does at 8ab64f0:

- Files: the checkpoint (logical name `checkpoint`, the third argument) holds a first line
  `APPVER k0 k1 next_k surv fermat1 nhits` and then one line per hit found so far, `X L` (L is the length of the
  symmetric extension, 11 to 27). It is written to `<name>.tmp` and renamed over the old one (`save_ckpt()`,
  `symm27.c:193-202`). `out` is written only at the end, and the checkpoint is then removed (`:291-294`).
- Authority: the checkpoint. `load_ckpt()` (`:203-208`) accepts it only when `APPVER`, k0 and k1 match, the hit count
  is in range, `next_k` is k0 plus a multiple of 256 and every hit line is present; otherwise the task starts from
  k0. The counters and hits cover exactly the committed chunks.
- CPU app: the main thread wakes at least once a second (`:278-279`), reports progress and, when
  `boinc_time_to_checkpoint()` returns 1, writes the checkpoint while it holds the mutex that guards commits, then
  calls `boinc_checkpoint_completed()` (`:283-286`). The search threads keep working meanwhile; chunks that were
  finished but not yet committed are computed again after a restart.
- GPU app: the same calls after each batch's candidates are verified (`symm27_gpu.c:225-227`).
- Standalone builds (without `USE_BOINC`) write a checkpoint every 10 s.
- Verified on 2026-11-24 (Apple M4 Pro, standalone builds): a CPU run on 10 threads killed after 25 s and a GPU run
  killed after 15 s, each restarted from its checkpoint, gave files identical to uninterrupted runs (k = 10^12 to
  10^12 + 1,048,575 and + 2,097,151).
- Known gaps: the checkpoint's writes, close and rename are unchecked, Windows removes the old file before a plain
  `rename()`, and a failed write is still reported (LYR-004); write errors on `out` are not checked before
  `boinc_finish(0)` (LYR-003). Both matter only on a full disk or an unlucky kill.

## BOINC lifecycle and threads

- `boinc_init()` with default options (`symm27.c:254`, `symm27_gpu.c:102`). No status polling: quit and abort are left
  to the library. Input that cannot be read ends in `boinc_finish(1)` in both apps.
- **CPU app:** `main()` starts `--nthreads` search threads (default 1, at most 64; `symm27.c:256-260`, `:276`) and
  from then on only waits, reports progress and checkpoints. The search threads do all the computing and never look
  at the BOINC status, so on Windows and Android a suspended task keeps computing (LYR-001, high). `setup()` builds
  the stage-1 masks with the same number of threads (`:174`): about 2.2 s on one M4 Pro core.
- **GPU app:** builds the masks with 4 threads (`symm27_gpu.c:118`; about 0.7 s), then works in the main thread only;
  `--nthreads` is ignored (`:106`). Its host work is the verification of Fermat survivors: about 0.2 s of CPU time for
  11 s of GPU time at k = 10^12 on the M4 Pro (estimated from the process's CPU time).
- Every GPU failure ends as a computation error today, with codes 2 to 8 (table in LYR-007).

## GPU app

- Device: `--device N` (default 0) indexes the GPU devices of all platforms that pass `EXPECTED_GPU_VENDOR`
  (`symm27_gpu.c:125-129`); `init_data` is not read (LYR-005, LYR-006). The stderr line `GPU: <name>` shows the choice.
- Batch size from `CL_DEVICE_GLOBAL_MEM_SIZE`: 8, 4 or 2 chunks of 256 k, halved until the candidate lists fit in a
  third of the memory (`:132-138`); `SYMM27_BCH=1..32` overrides it. A batch that overflows the candidate buffers is
  halved and done again; at one chunk the task ends (exit 7). At k = 10^12 a batch of 2,048 k has about 10,000 sieve
  survivors against 131,072 Fermat slots (`FINMAX`).
- Kernels: `mkctab` (the residue class of each k per prime), `sieve1` (stage 1, one work item per k and 64-residue
  word), `seg` (stage 2 in five prime segments, compacting the survivors after each), `fermat` (96-bit Fermat test on
  the 11 positions). Integer only; no fp64 is needed. Local size `LS` = 64 for every launch (LYR-008).
- Two batch slots: batch n+1 is enqueued and flushed before the host waits for batch n (`:184-215`), so the host's
  verification overlaps the GPU's work.
- Apple Silicon OpenCL works (M4 Pro, 2026-11-24: one platform, 17.8 GB of reported global memory, batches of 8
  chunks, about 22 ms per batch at k = 10^12).

## Building

```
# standalone apps (checked 2026-11-24 on macOS arm64 with Apple clang 21; Linux also needs the OpenCL headers)
make standalone        # cc -O3 -march=native symm27.c -o symm27 -lpthread
make gpu               # cc -O3 -march=native symm27_gpu.c cl_dynload.c -o symm27_gpu -lpthread -ldl
# without -march=native, as used for the measurements in the knowledge base:
cc -O3 symm27.c -o symm27 -lpthread
cc -O3 symm27_gpu.c cl_dynload.c -o symm27_gpu -lpthread

# the BOINC CPU app against a built BOINC tree
make boinc BOINC_DIR=/path/to/boinc
```

Release builds (README, "Release builds") run in the maintainer's build container and cannot be repeated from this
repository alone (LYR-009):

- `tools/build_boinc_target.sh` (called `build_one.sh` by `tools/build_all_targets.sh`) builds `libboinc` and
  `libboinc_api` from `/root/boinc` for one target, then the CPU app, `-static`. `build_all_targets.sh` runs it for
  Linux x86_64 (`-march=x86-64-v2`, LYR-010), i686 (`-msse2 -mfpmath=sse`), aarch64, armhf, ppc64le and riscv64, and
  Windows x86_64 and x86, and tests all but Windows x86 with a `runtest.sh` that is not in the repository (qemu-user
  for the foreign Linux targets, Wine for Windows).
- `tools/build_gpu_linux_focal.sh`, inside an Ubuntu 20.04 chroot: one OpenCL app per vendor
  (`-DEXPECTED_GPU_VENDOR="nvidia|amd|intel"`), static libstdc++ and libgcc; prints the newest GLIBC version the
  binary needs (README: 2.29).
- `tools/build_gpu_windows.sh`: MinGW-w64, one OpenCL app per vendor, `-DCL_TARGET_OPENCL_VERSION=120`.
- macOS (CPU, and the arm64 GPU app), Android aarch64 (NDK, CPU only) and Windows arm64 (llvm-mingw): no script.

OpenCL is never linked: `cl_dynload.c` loads it at run time, so only the headers are needed to build. The scripts
download nothing and record no versions of the BOINC tree or the compilers (LYR-009).

## Tests

Build them outside the repository (each includes `symm27.c`):

```
cc -O2 test_verify.c -o test_verify -lpthread && ./test_verify      # "6 test, 0 hata" (0 failures)
cc -O2 -I/opt/homebrew/include test_arith.c -o test_arith -L/opt/homebrew/lib -lgmp -lpthread && ./test_arith
#   prints "hata: 0" (0 errors) after about 1 s; add -DLYRA_NO_INT128 to test the portable multiply
```

Both passed on 2026-11-24 on macOS arm64 (GMP from Homebrew), `test_arith` with and without `LYRA_NO_INT128`.

How to check a change today (no harness exists for the BOINC parts):

- Arithmetic or verification: `test_arith` and `test_verify`.
- Anything in the search: build the old and the new app, run both on the same `in`, `cmp` the files. Ranges with
  hits: `1 8191` (2 hits), `1 65535` (7 hits; about 20 s on one M4 Pro core), `1000000000000 1000001048575` (5 hits
  and a 13-tuplet; about a minute on 10 cores, 11 s on the M4 Pro's GPU). Also compare `--nthreads 1` with several
  threads, and the GPU app with the CPU app: the files must be identical. All of these were on 2026-11-24.
- The GPU kernel near its limit: `5338293918674000 5338293918674255` must match the CPU app (LYR-002).
- Resume: run a standalone build, kill it after its first checkpoint (10 s), start it again in the same directory and
  `cmp` the file with an uninterrupted run's.
- Windows: a MinGW-w64 build of `symm27.c` under Wine. Checked on 2026-11-24: MinGW-w64 GCC 16 (UCRT), Wine 8 in an
  x86_64 Debian 12 container, `1 8191` with 4 threads: identical to macOS.
- The BOINC paths of `main()` (checkpoint calls, suspend, device choice): no harness. The knowledge-base patterns
  describe the stub BOINC library to build (PAT-001, "How to test"; kampylos has one in `tests/boinc_stub/`).
- Keep work files outside the repository (for example in `~/lyra-work`).

## Environment notes

- `-march=native` in the Makefile is fine for local runs, not for binaries that leave the machine.
- Build switches: `USE_BOINC` (the BOINC API; without it the apps run standalone and resolve no file names),
  `EXPECTED_GPU_VENDOR` (GPU platform filter, empty = any vendor), `LYRA_NO_INT128` (portable 64-bit multiply, same
  results), `PBIT`, `LS`, `DUMPDOB` (write the residues and stop), `PROF` (defined, but the loop does not use it).
  Environment: `SYMM27_DOBFILE` (standalone only: read the residues from a file, `symm27.c:165`), `SYMM27_BCH` (GPU
  batch size).
- MinGW builds use winpthreads; the code uses `__builtin_ctzll` and `__builtin_popcountll`, so it needs GCC or clang
  (there is no MSVC build).
- Android: only the CPU app is built. `cl_dynload.c` already has the linker-namespace fallback a GPU app there would
  need (PAT-017).

## Conventions for changes

- Small branches, one concern each. One fix per commit, with the test or the measured comparison that shows the bug
  and the fix. The commit message says what was wrong, how it was reproduced or measured, what the fix changes for
  volunteers (checkpoints, credit, run time, results), and names the issue (`KB: LYR-NNN`) and the pattern if any
  (`Pattern: PAT-NNN`).
- Keep the backends in step: the CPU app and the GPU kernel must accept the same candidates and print the same file.
  A change to the sieve, the patterns or the Fermat stage needs the same change in the kernel, and the reverse; then
  `cmp` the CPU and GPU files.
- Do not change the result format, the header counters, `APPVER`, the sieve bounds or the patterns silently: that
  changes results and what the server compares. Record such a change in the knowledge base with its measured effect.
- Numbers in comments (timings, densities) are measurements: date them, and update them when a change moves them.
- A fix to `cl_dynload.c` or `.h` belongs in the other copies too (`kb.py status ../planodia ../phygas ../nostos`
  shows the drift).

## Not in this repository

- The server side: the work generator (which k ranges, `rsc_fpops_est` and the bound), the validator (what it
  compares, the quorum), the assimilator, the plan classes (which vendor build goes to which GPU, CPU feature
  requirements).
- The build container: the BOINC source tree, the cross compilers, the MinGW headers, the Wine prefix, the Ubuntu 20.04
  chroot, llvm-mingw, the NDK, and the helper `runtest.sh`.

## Knowledge base

`docs/kb/` holds this repository's known bugs and platform traps, and the bug patterns shared with the other bitboinc
applications. Workflow: [docs/kb/shared/PROCESS.md](docs/kb/shared/PROCESS.md). It has two parts:

* **This repository's records**, edited here only: `issues/` (one file per issue, `LYR-NNN`), `ledger.md` (this
  repository's result for every shared pattern), `kb.conf` (the issue prefix and the files that are copies of code in
  other repositories), `README.md`, and `INDEX.md`, which `kb.py index` writes.
* **The shared part, `docs/kb/shared/`**, the same in every bitboinc repository that has a knowledge base: the
  procedure (`PROCESS.md`, `REFERENCE.md`), `vocabulary.md`, `boinc-api.md`, the bug patterns
  (`patterns/PAT-NNN-*.md`), `templates/`, the tool `kb.py` and `MANIFEST`. Never change it in one repository only.
  After editing any file there, run `python3 docs/kb/shared/kb.py sync ../kampylos ../nostos ../phygas ../planodia`
  from this repository, naming every other bitboinc repository with a `docs/kb` checked out next to this one,
  including any that are not public. It copies the newest version of each shared file to all of them and gives each
  ledger an `unchecked` row for a new pattern. Then commit `docs/kb` in every repository. `MANIFEST` holds the hashes
  of the last sync, and `kb.py lint` warns when a shared file here differs from it. The prefix table at the end of
  `REFERENCE.md` does not list `LYR` yet; add it with the next shared change.

- Ready to fix, with next steps: `python3 docs/kb/shared/kb.py list --open --action fix` (also `--area`,
  `--platform`). Then `kb.py show LYR-NNN` for the issue and `kb.py show PAT-NNN` for the pattern it links, with the
  BOINC API facts the pattern cites. In the code, read the functions that `where:` and the Cause name.
- All repositories checked out next to this one: `kb.py todo`, then `kb.py todo --pattern PAT-NNN` for the next step
  and place of each issue; `kb.py check PAT-NNN` checks a pattern everywhere.
- Before debugging a symptom: `grep -ril "<words>" ../*/docs/kb/issues docs/kb/shared/patterns`, then `kb.py show`
  the ids it finds. Read other repositories through the shell (`kb.py show`, `git -C ../<repo> grep -n`, `sed -n`):
  opening one of their files in an editor tool can load their whole AGENTS.md as well.
- New report: `kb.py new issue "symptom in a few words" area=... next=...`.
- After a fix: `kb.py set LYR-NNN status=fix-proposed --log "..."` (`status=fixed fixed_in=<commit>` once merged),
  and update the passages of this file it prints. The commit names the issue (`KB: LYR-NNN`) and the pattern, if any
  (`Pattern: PAT-NNN`).
- Before a pull request: `kb.py index && kb.py lint --strict`.
