# Lyra

Search for **symmetric 27-tuplets of consecutive primes**: 27 consecutive primes whose gaps form a palindrome, spanning 432 (offsets 0, 6, 12, 36, 90, 96, 102, 120, 132, 162, 180, 186, 210, 216, 222, 246, 252, 270, 300, 312, 330, 336, 342, 396, 420, 426, 432).

Such a tuplet contains a central 11-tuplet (offsets 0, 30, 48, 54, 78, 84, 90, 114, 120, 138, 168). Lyra searches for that centre at `X = k*37# + dob` and then extends every hit symmetrically to 13, 15, ..., 27 consecutive primes.

## How it works

1. `dob` are the 286,720 residues mod 37# that are compatible with the 27-pattern. The program builds them itself (Chinese remainder theorem), no data file is needed.
2. Stage 1: bit-mask sieve over all `dob` for primes 41..199.
3. Stage 2: fast integer sieve with primes up to 20,000.
4. Stage 3: Fermat base-2 test, then exact verification with GMP (primality and "consecutive" check, then symmetric extension).

The output is deterministic and independent of thread count and of CPU or GPU, so results can be compared bit for bit.

## Programs

- `symm27.c`: CPU version, one process, `--nthreads N` threads sharing the sieve tables (about 150 MB RAM). Checkpointing, optional BOINC API (`-DUSE_BOINC`).
- `symm27_gpu.c`: OpenCL version with the same input, output and checkpoint format. Memory-adaptive batch size.
- `test_verify.c`: unit tests for the verification code.
- `gendob.py`: reference generator for the `dob` residues.

## Build

Needs GMP (and OpenCL for the GPU version).

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
