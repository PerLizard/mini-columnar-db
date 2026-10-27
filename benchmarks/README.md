# Scan benchmark

Build and run from the repository root:

```sh
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DENABLE_SANITIZERS=OFF -DBUILD_BENCHMARKS=ON -DBUILD_TESTING=OFF
cmake --build build-release --target mini_db_benchmark -j2
./build-release/mini_db_benchmark
```

`scan_benchmark.cpp` generates identical data in a vector and a `Table` for 100,000,
1,000,000 and 10,000,000 rows, rebuilding both representations for each size
using `std::mt19937` with seed 42. Prices are uniform in [0, 100000), volumes in
[0, 100), and symbol IDs in [0, 999]. All four fields are checked for equality
before timing, after checking that all four column sizes equal the row count.
Standard library distribution implementations may differ across
toolchains; the seed reproduces the inputs within the same implementation.

`scan_queries.cpp` implements A (one row scan), B (one column scan), C
(`filterIndices` followed by `sumSelected`), and D (`sumWhere`).
Query functions are compiled separately
from the runner, with LTO disabled. Generation, schema lookup, validation and
printing are outside the timed intervals. C includes allocation, population and
destruction of its intermediate index vector.

For each threshold, every variant gets 3 warmups and 24 timed runs measured with
`steady_clock`. All 24 execution-order permutations of four variants are used
once during measurement, so each variant occupies each position six times.
Times are sorted once; p25, median and p75 use linear interpolation at
`p * (n - 1)` (p = 0.25, 0.5, 0.75). The median remains the average of
the two middle samples.
Every returned sum, including warmups, is checked against an untimed reference
sum and added to a printed checksum. The `expected_sum` output field is that reference
sum. A mismatch or non-finite result fails the run.
No operators or storage classes are modified for this benchmark.

## Initial results

Historical baseline with three variants, 3 warmups and 18 measurements, using
all six permutations three times.

Measured on AMD Ryzen 5 5600H, x86-64, with GCC 15.2.0
(`Ubuntu 15.2.0-16ubuntu1`), Release, no sanitizers or LTO.
`sizeof(Trade) = 32` bytes. Commands are shown above.

| Threshold | Variant | Rows | Matched | Median, ms | Million rows/s | Expected sum |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| > 90000 | A row scan | 1000000 | 99341 | 1.74869 | 571.857 | 4967433.3516813191 |
| > 90000 | B column scan | 1000000 | 99341 | 1.18136 | 846.485 | 4967433.3516813191 |
| > 90000 | C operators | 1000000 | 99341 | 1.65725 | 603.411 | 4967433.3516813191 |
| > 10000 | A row scan | 1000000 | 899734 | 1.73262 | 577.161 | 44997348.774339892 |
| > 10000 | B column scan | 1000000 | 899734 | 1.52206 | 657.003 | 44997348.774339892 |
| > 10000 | C operators | 1000000 | 899734 | 3.12536 | 319.964 | 44997348.774339892 |

Actual selectivities: 9.9341% and 89.9734%. Checksums over all 21 runs per
variant: 104316100.38530773 and 944944324.26113808 respectively; all variants agreed.

In this run B was about 1.48 times faster than A at low selectivity and 1.14 times
faster at high selectivity. C took about 1.40 and 2.05 times as long as B.
The intermediate indices and second pass are a plausible source of that overhead;
this benchmark does not isolate their individual costs. These are warm repeated
scans on one machine without CPU affinity or frequency control, so the results
are a baseline rather than a general claim about either storage layout.

## Repeat measurement with sumWhere

Same machine, compiler, build command and data as above; 3 warmups and 24
measurements per variant, using all 24 permutations. The existing target still
compiles query functions separately with `-fno-lto` and links with `-fno-lto`.

| Threshold | Variant | Rows | Matched | Median, ms | Million rows/s | Expected sum |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| > 90000 | A row scan | 1000000 | 99341 | 1.63433 | 611.872 | 4967433.3516813191 |
| > 90000 | B column scan | 1000000 | 99341 | 1.17292 | 852.572 | 4967433.3516813191 |
| > 90000 | C operators | 1000000 | 99341 | 1.5227 | 656.729 | 4967433.3516813191 |
| > 90000 | D sumWhere | 1000000 | 99341 | 1.17162 | 853.520 | 4967433.3516813191 |
| > 10000 | A row scan | 1000000 | 899734 | 1.69632 | 589.510 | 44997348.774339892 |
| > 10000 | B column scan | 1000000 | 899734 | 1.5439 | 647.711 | 44997348.774339892 |
| > 10000 | C operators | 1000000 | 899734 | 3.11671 | 320.851 | 44997348.774339892 |
| > 10000 | D sumWhere | 1000000 | 899734 | 1.53708 | 650.584 | 44997348.774339892 |

All results agreed with the reference. Checksums over all 27 runs per variant
were 134120700.49539566 and 1214928416.9071774 respectively.
D was close to B at both selectivities and about 1.30 and 2.03 times faster
than C in this run. The small B/D differences do not establish a performance
advantage; both execute a single scan without materializing indices.


## Scaling experiment

The runner now measures all three sizes, each with seed 42 reset so the smaller
inputs are prefixes of the larger inputs. Query algorithms and compiler flags
are unchanged. Each size is destroyed before generating the next one.
Three complete processes were run sequentially on the same machine with GCC
15.2.0, Release, without sanitizers or LTO:

```sh
cmake --build build-release --target mini_db_benchmark -j2
mkdir -p benchmarks/results
for run in 1 2 3; do
    ./build-release/mini_db_benchmark > benchmarks/results/scaling-run-$run.txt
done
```

Raw results, including matched counts, throughput, reference sums and checksums:
[run 1](results/scaling-run-1.txt), [run 2](results/scaling-run-2.txt),
[run 3](results/scaling-run-3.txt). The comparison table is in
[scaling results](results/scaling.md); quartiles describe the 24 measurements
within each process, not a pooled distribution across processes.

The CPU reports 32 KiB L1d and 512 KiB L2 per core, and 16 MiB L3. Row data
occupy about 3.2 / 32 / 320 MB for these sizes; the two scanned columns occupy
1.6 / 16 / 160 MB (decimal MB, logical payload only). Both representations
remain allocated during measurement. These sizes cross cache capacities, but
this experiment does not directly measure cache misses or isolate allocation
costs, CPU frequency changes or scheduling effects.

Across the three processes, B and D remain close at every size. At 100,000 rows
and ~90% selectivity, A is faster (median 0.119–0.128 ms versus B's
0.147–0.150 ms), so the column layout does not win universally. At 10 million
rows, B takes 12.32–12.47 ms at ~10% selectivity versus A's 15.71–16.85 ms.
At ~90%, C grows to 126.45–130.91 ms while B and D remain around 15–16 ms.
The index vector for ~9 million matches alone contains about 72 MB of indices
on this 64-bit platform, before spare capacity and reallocation overhead.
Allocation and memory traffic are plausible contributors; cache counters or
allocation profiling would be needed to attribute the slowdown precisely.
