# CS598 APE HW3 - Abelian Sandpile

Zicheng Song, Fall 2026. The optimized program is portable C and requires no university access or special hardware.

## Layout

`sandpile.c` is the final optimized implementation. `baseline/sandpile_baseline.c` is an exact copy of the upstream solver at tag `upstream-baseline`. The batch, queue, and forward sources preserve measured ablations; `sandpile_alternate.c` is the named copy of the final alternating-sweep variant. `tests/test_correctness.py` checks exact output and stability. `scripts/benchmark.py` reproduces measurements. `results/`, `profiling/`, `figures/`, and `report/` contain evaluation evidence and the paper.

## Build and run

Requirements: GCC or Clang, Make, Python 3.

```sh
    make all variants
    ./baseline/sandpile_baseline.exe 1001
    ./sandpile.exe 1001
```

Both normal runs write identity.png. SANDPILE_DUMP=grid.bin exports raw values; SANDPILE_STATS=1 prints counters.

Docker:

```sh
    docker build -t hw3 .
    docker run --rm hw3
```

The default container command is a quick N=101 smoke run. Use `docker run --rm hw3 ./sandpile.exe 1001` for the full workload.

Report (optional, requires Tectonic):

```sh
cd report
tectonic report.tex
```

## Correctness

```sh
    make test
    make sanitize
    make valgrind
```

Exact baseline equality and raw-grid stability passed for N=1,2,3,4,5,10,31,101,201. Three complete N=1001 baseline and optimized runs also matched bit-for-bit; the full PNG SHA-256 is `4883dba5b09236b78477ca25c9395411fc1f82f4233aaa378193d7925269dbbe`. The 1001x1001 reference is `identity-ref.png`, and the per-run hashes are retained in `results/`. ASan/UBSan passed at N=201; Valgrind passed at N=101 with zero errors and leaks.

## Benchmarks

```sh
    python3 scripts/benchmark.py --n 401 --repeats 3 --variants baseline optimized batch queue forward
    python3 scripts/benchmark.py --n 1001 --repeats 3 --variants baseline optimized --out results/reproduced_n1001
```

GCC 15.2.0 used -O3 -g -Werror -lm on a four-vCPU Xeon Silver 4216 VM. N=1001 median time fell from 611.89 to 270.22 s (2.264x); median RSS rose from 12484 to 14792 KiB.

## Optimizations

Batch toppling applies floor(v/4) topples at once. A compact row-major Pacman domain list avoids geometry checks. One-cell padded contiguous storage removes hot-loop boundary branches. Alternating sweep direction reduces propagation passes. At N=401: baseline 16.50 s, batch 14.16 s, compact forward 7.62 s, and alternating 7.25 s.

Rejected: a full mask scan took 19.50 s because it scanned all N squared cells. The correct active queue took 53.81 s and processed 982807074 events. perf was blocked by perf_event_paranoid=4; gprof attributed 99.94 percent to stabilize.

## Limitations

The method remains sequential and scan based. Padding and the domain list increase memory modestly. Timings are machine dependent.
