# Lyra

Search for **symmetric 27-tuplets of consecutive primes**: 27 consecutive primes whose gaps form a palindrome, spanning 432 (offsets 0, 6, 12, 36, 90, 96, 102, 120, 132, 162, 180, 186, 210, 216, 222, 246, 252, 270, 300, 312, 330, 336, 342, 396, 420, 426, 432).

Such a tuplet contains a central 11-tuplet (offsets 0, 30, 48, 54, 78, 84, 90, 114, 120, 138, 168). Lyra searches for that centre at `X = k*37# + dob` and then extends every hit symmetrically to 13, 15, ..., 27 consecutive primes.

## How it works

1. `dob` are the 286,720 residues mod 37# that are compatible with the 27-pattern. The program builds them itself (Chinese remainder theorem), no data file is needed.
2. Stage 1: bit-mask sieve over all `dob` for primes 41..199.
3. Stage 2: fast integer sieve with primes up to 20,000.
4. Stage 3: Fermat base-2 test (Montgomery arithmetic on 128-bit integers), then verification of every hit with Miller-Rabin (20 prime bases; deterministic below 3.3e24, probabilistic above) and the "consecutive" check, then symmetric extension.

The output is deterministic and independent of thread count and of CPU or GPU, so results can be compared bit for bit.

## Programs

- `symm27.c`: CPU version, one process, `--nthreads N` threads sharing the sieve tables (about 150 MB RAM). Checkpointing, optional BOINC API (`-DUSE_BOINC`).
- `symm27_gpu.c`: OpenCL version with the same input, output and checkpoint format. Memory-adaptive batch size. The OpenCL library is loaded at run time (`cl_dynload.c`), so the binary starts, and exits with a clear message, on hosts without an OpenCL driver. `--device N` selects the GPU; `-DEXPECTED_GPU_VENDOR=\"nvidia|amd|intel\"` restricts it to one vendor.
- `test_verify.c`: unit tests for the verification code. `test_arith.c`: cross-checks the 128-bit arithmetic against GMP (test only; needs GMP).
- `gendob.py`: reference generator for the `dob` residues.

## Build

Needs only a C compiler and pthreads (and OpenCL for the GPU version). No external libraries; `__int128` is used when available, with a portable fallback for 32-bit targets (`-DLYRA_NO_INT128` forces it).

```
make standalone      # CPU
make gpu             # OpenCL
```

## Usage

```
./symm27 [--nthreads N] in out checkpoint
```

`in` contains `kstart kend`. `out` is written at the end:

```
symm27 v2 k=[a,b] dobs=286720 surv=... fermat1=... hits=...
<X>,              one line per central 11-tuplet, ascending
13-tuplet
<X>: 0, 12, 42, ...    continuations, one section per length (13 ... 27)
```

## Licence

GPL-3.0

## Release builds

`tools/build_boinc_target.sh` compiles the BOINC libraries (from a BOINC source tree) and Lyra for one target, fully static for Linux and Windows; `tools/build_all_targets.sh` runs it for Linux x86_64, i686, aarch64, armhf, ppc64le, riscv64 and Windows x86_64, x86 (paths such as `/root/boinc` and `/opt/llvm-mingw` are those of the build container). Windows arm64 uses llvm-mingw, Android aarch64 the NDK (dynamic PIE, needs only libc, libm, libdl), macOS builds use Apple clang with `-mmacosx-version-min=11.0`. All builds print the same result file for the same input. Result files are written in binary mode, so they are byte-identical on every platform.

GPU release builds: Linux x86_64 is built in an Ubuntu 20.04 chroot (glibc 2.31, the binaries need GLIBC 2.29 or newer; libstdc++ and libgcc are linked statically) by `tools/build_gpu_linux_focal.sh`, Windows x86_64 with mingw-w64 by `tools/build_gpu_windows.sh`, macOS arm64 with Apple clang. Output is byte-identical to the CPU version.
