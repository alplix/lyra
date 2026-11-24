#!/usr/bin/env python3
"""Lyra work generator (config.xml <task>, every 5 minutes) for the CPU app "lyra".

One work unit = a range of k (X = k*37# + dob). The input file "in" holds "kstart kend". Both apps draw their ranges from
the same counter (lyra_state.json), so the ranges never overlap and never leave gaps. A GPU work unit is much bigger
(about 50 CPU work units: a fast card does 3 million k in a few seconds, a CPU core about 3 million k per hour).
Keeps TARGET_UNSENT results unsent per app. DISABLED unless the file lyra_enabled exists (or --force).
--dry-run prints only.  --app lyra|lyra_gpu and --count N limit a manual run (with --force).
"""
import fcntl, json, os, subprocess, sys, time

PROJECT_DIR = "/home/bitboinc/projects/bitboinc"
P37 = 7420738134810
P71 = 557940830126698960967415390
K_START = 9425346484752129657862217 // P37      # start requested for the search
K_END = P71 // P37                               # end of the range (71#)
# app: k per work unit (test sizes; production CPU: 3.6M, about 1 h on one core), unsent target, flops per k, credit per work unit
APPS = {
    "lyra":     {"wu_k": 10_000_000, "target": 200, "fpops_per_k": 3.0e6, "credit": 500},
}
MAX_PER_RUN = 60
DELAY_BOUND = 14 * 86400
QUORUM = ["--min_quorum", "2", "--target_nresults", "2", "--max_total_results", "8",
          "--max_error_results", "4", "--max_success_results", "5"]
STATE = os.path.join(PROJECT_DIR, "lyra_state.json")


def log(msg):
    print(f"{time.strftime('%Y-%m-%dT%H:%M:%S')} lyra_generator: {msg}", flush=True)


def unsent(app):
    out = subprocess.run(["mysql", "--defaults-extra-file=/home/bitboinc/.my.cnf", "bitboinc", "-N", "-B", "-e",
                          f"SELECT COUNT(*) FROM result r JOIN workunit w ON r.workunitid=w.id JOIN app a ON w.appid=a.id "
                          f"WHERE r.server_state=2 AND a.name='{app}'"],
                         capture_output=True, text=True, check=True)
    return int(out.stdout.strip() or 0)


def main():
    dry = "--dry-run" in sys.argv
    force = "--force" in sys.argv
    only = sys.argv[sys.argv.index("--app") + 1] if "--app" in sys.argv else None
    count = int(sys.argv[sys.argv.index("--count") + 1]) if "--count" in sys.argv else None
    if not dry:
        os.makedirs(os.path.join(PROJECT_DIR, "pid_lab"), exist_ok=True)
        lock = open(os.path.join(PROJECT_DIR, "pid_lab", "lyra_generator.flock"), "w")
        try:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except OSError:
            return
    if not (force or dry or os.path.exists(os.path.join(PROJECT_DIR, "lyra_enabled"))):
        return
    try:
        nxt = json.load(open(STATE))["next_k"]
    except Exception:
        nxt = K_START
    total = 0
    for app, cfg in APPS.items():
        if only and app != only:
            continue
        want = min(MAX_PER_RUN, cfg["target"] if dry else cfg["target"] - unsent(app))
        if count is not None:
            want = count
        made = 0
        while made < want and nxt <= K_END:
            a, b = nxt, min(nxt + cfg["wu_k"] - 1, K_END)
            name = f"{app}_{a}_{b}"
            if dry:
                log(f"would create {name}")
            else:
                infile = f"lyra_in_{a}"
                r = subprocess.run(["bin/dir_hier_path", infile], cwd=PROJECT_DIR, capture_output=True, text=True)
                dest = r.stdout.strip()
                os.makedirs(os.path.dirname(dest), exist_ok=True)
                with open(dest + ".tmp", "w") as f:
                    f.write(f"{a} {b}\n")
                os.replace(dest + ".tmp", dest)
                nk = b - a + 1
                fpops = nk * cfg["fpops_per_k"]
                r = subprocess.run(["bin/create_work", "--appname", app, "--wu_name", name,
                                    "--wu_template", "templates/lyra_in", "--result_template", "templates/lyra_out",
                                    "--credit", str(cfg["credit"]), "--rsc_fpops_est", str(fpops),
                                    "--rsc_fpops_bound", str(fpops * 100),
                                    "--rsc_disk_bound", "100000000", "--rsc_memory_bound", "600000000",
                                    *QUORUM, "--delay_bound", str(DELAY_BOUND), infile],
                                   cwd=PROJECT_DIR, capture_output=True, text=True)
                if r.returncode != 0:
                    log(f"create_work failed for {name}: {r.stderr.strip()[:300]}")
                    os.unlink(dest)
                    break
                json.dump({"next_k": b + 1}, open(STATE + ".tmp", "w"))   # saved after every work unit
                os.replace(STATE + ".tmp", STATE)
            nxt = b + 1
            made += 1
        total += made
        log(f"{app}: created {made} work units, next k {nxt}")


if __name__ == "__main__":
    main()
