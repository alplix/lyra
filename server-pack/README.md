# Lyra server pack (symmetric 27-tuplet search, idea/method: Natalia Makarova)

Contents: ready binaries (apps/lyra/3 = CPU `lyra_mt` for Linux x86_64/i686/aarch64/armhf/ppc64le/riscv64, Windows x64/x86/arm64,
macOS arm64/x86_64, Android aarch64; apps/lyra/1 = OpenCL GPU for NVIDIA/AMD/Intel on Linux+Windows, Apple GPU on macOS),
templates, work generator, validator/compare/assimilator scripts, config snippets.

Status (be honest): proven on our beta server: Linux/Windows x64 CPU, NVIDIA/AMD/Intel/Apple GPU results validate against each other.
Mac, Android, Windows x86/arm64 and ppc64le/riscv64 builds are open but have little or no real-world confirmation yet.
NOT tested: this installer on a fresh server (it follows the standard BOINC layout; tell me if anything fails).

## 1. Install
    ./install.sh /path/to/project          # run as the BOINC project user
## 2. plan classes
Append the blocks in conf/plan_class_spec.lyra.xml to your plan_class_spec.xml.
## 3. config
Add the app to project.xml, and the <task> and two <daemon> blocks from conf/config.lyra.xml to config.xml
(the validator/assimilator use the `mysql` client with ~/.my.cnf of the project user: adjust the DB lines in bin/lyra_*.sh if yours differs).
Set credit values in the validator line: `--credit_by_plan_class <GPU> <CPU>`.
Then: `bin/stop; bin/start`.
## 4. Create tasks
Work unit = k range [a,b], input file "a b", quorum 2 (every range computed twice and compared).
The generator keeps ~200 unsent results and remembers the next k in lyra_state.json. It is off until you create the file `lyra_enabled`:
    touch lyra_enabled          # start;  rm lyra_enabled to pause
    bin/lyra_generator.py --force --count 5     # manual test run;  --dry-run only prints
Range: start k = 9425346484752129657862217 // 37#, end = 71# // 37#  (see the top of bin/lyra_generator.py). Change `wu_k` for work unit size
(10M k per unit now; a CPU core does ~3M k per hour).
## 5. Results
Validated results are collected in lyra_results/: 11tuplets.txt, continuations.txt (13..27-tuplets), progress.txt (one line per WU),
FOUND_27.txt if a 27-tuplet is ever found. Do not delete lyra_results/.
## Notes
- Keep file_deleter running as usual; the assimilator copies results out before the files are deleted.
- Credit: edit the numbers above; GPU is about 50x faster per unit than one CPU core, so GPU credit per unit should not be much lower than CPU's.
