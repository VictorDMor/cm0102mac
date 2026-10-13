# Remove repeated environment lookup from engine dispatch

The native runtime now captures import call/result tracing once, at the first
engine dispatch. The setting remains available through `CM_AOT_TRACE_IMPORTS`
at process launch. Semantic dependency calls no longer search and lock the
process environment on every entry to `run_until`.

This improves the runtime shared by the game and match debugger. It does not add
an original-function replacement: the accepted semantic manifest remains at
**656 entries**, and the complete C++23 engine reconstruction remains active.

## Measured reason for prioritizing this change

A native macOS CPU sample of repeated 600-tick Arsenal/Manchester United
intervals found **4,424 environment-search leaf samples out of 16,952 main-thread
samples (26.1%)**. The stacks led through semantic match operations and nested
`run_until` calls. The lookup included locking and scanning even when tracing
was disabled. These are sampled host CPU observations, not guest instruction
counts or a completion percentage.

The same 20-second sampling procedure after the change contains **zero
environment-search samples among 16,723 main-thread samples**. Both complete
profiling runs execute 42,000 ticks, 10,634,288,910 guest instructions and
7,074,340 semantic sampler calls. Sampling confirms removal of the identified
hot path; the unprofiled paired measurements below establish the timing result.

## Before/after measurement

The baseline retains the accepted `e5b78bd` runtime body; the candidate changes
only the dispatch trace-policy initialization. Both are Release ARM64 C++23,
with the same generated program, semantic modules, native bridge and benchmark
driver. Each process restores the same private initial match snapshot. There is
one warmup pair and ten measured pairs per matchup, alternating execution order.
Each interval runs 600 real shared-engine ticks, with turbo and PC profiling off.
Setup, snapshot loading, state comparisons and snapshot writes are outside the
timed interval. No other build or benchmark runs concurrently.

| Matchup, seed 12345 | Before median | After median | Median paired time reduction | Faster pairs |
| --- | ---: | ---: | ---: | ---: |
| Arsenal / Manchester United | 0.625761 s | 0.428716 s | 31.585% | 10/10 |
| Cruzeiro / Manchester United | 0.577318 s | 0.394401 s | 31.653% | 10/10 |

Paired reductions range from 29.081–33.164% and 30.364–32.470%, respectively.
These are results for the measured intervals on this Mac, not a guarantee for
every screen, season or machine.

The two builds produce byte-identical **126,967,858-byte final snapshots** for
each matchup. These include every register, generated temporary, mapped memory
byte, allocation table, PC and native/instruction count. Each process also
compares every repeat against its first result; instruction budget and import
counts match across builds. The intervals execute 151,918,413 and 141,446,757
guest instructions, respectively. Restored states have no host platform;
non-clock imports use the unchanged State-only native bridge. Neither interval
requests a clock input. No shortened simulation or skipped bridge work accounts
for the improvement.

## Diagnostic behavior and validation

The environment variable's presence, including an empty value or `0`, still
enables tracing. Absence disables it. Unlike the prior per-call lookup, changing
the host environment after the first dispatch no longer changes dispatch
tracing. This process-level policy is documented in the launch diagnostics.
Guest state, import callbacks, transcript handling and nonlocal exception routing
are unchanged.

Eight focused CTests pass:

- Four fresh-process tracing cases check absent, empty, zero and one values,
  exact nested call/result text, peer states, import exceptions and recovery.
- Actual sampler production checks and its nested nonlocal-transfer checks
  exercise normal execution, faults, budgets and restored guest contexts.
- Actual match completion checks cover Arsenal's initial interval, Cruzeiro
  reaching full time, original/C++23 final-state agreement and debugger undo/redo.
- The private season transcript test checks recorded native effects, original
  callbacks, replay equality and malformed/divergent input rejection.

Private baselines, snapshots, profiling data, build/test logs and timing records
are retained under `.local/dispatch-runtime-optimization`; the earlier CPU sample
is under `.local/match-cost-profile`. No original game payload or private runtime
data is added to Git. Ordinary game Save/Load is **not retested**.

## Installed delivery

Both applications are installed as strictly verified signed ARM64 bundles.
The game executable SHA-256 is
`c82e61010361ebbca1b919afa8ab777c436a27cec8b94ad439401f2639e81e37`;
the debugger executable SHA-256 is
`0a913d25f65fb4f30a21eab3878571179a852dbe4c2feee9e81204789abe9098`.
The running game maps that exact installed executable, renders its original
menu and exits normally through the original confirmation. The installed
debugger passes its two-pairing worker protocol and a separate full-time check:
Cruzeiro/Manchester United completes after 789 steps, 1–2 at 92:38, with readable
full-time events, rejection of further steps, undo/redo and normal cleanup.
All 845 protected game, runtime, save and handoff files retain their hashes.
Previous signed application references remain private.
