# Pitch distance queries reconstructed

Three complete distance operations now execute named C++23 bodies in the shared
game/debugger engine. The ordered production manifest grows from 660 to **663**
replacements, preserving its previous prefix. The converted block contains all
129 original instructions of these operations. The complete engine conversion
remains active.

## Selection and scope

The preceding team-query conversion exposed distance lookup as a shared
dependency. Entry-read observations over 600 actual engine ticks measured:

| Operation | Entry | Original instructions | Arsenal calls | Cruzeiro calls |
|---|---|---:|---:|---:|
| Point to point | `101ef8d0` | 51 | 28,026 | 21,673 |
| Actor to cell | `1022e760` | 31 | 253,643 | 213,712 |
| Actor to attacking goal | `10245160` | 47 | 3,346 | 1,995 |

Both pairings use Manchester United as the opponent and seed 12345. The compact
129-instruction block accounts for 285,015 and 237,380 observed calls,
respectively. These counts establish reuse in the measured intervals, not a
CPU percentage or a whole-game coverage estimate. Original listings and inputs
remain private.

## Preserved contract

Distances come from the live original single-precision lookup table, promoted
to the floating return register. The table uses a nine-element row stride.
The implementation retains the original absolute-value dependency and the
signed-byte narrowing of coordinates and absolute deltas. It does not replace
the table with a host geometric formula or add bounds checks to the unbounded
queries.

Point-to-point lookup spills all four coordinate bytes before computing either
delta. The narrowed row delta must be in 0..8 and the column delta in 0..11;
otherwise a live fallback constant is loaded. The actor-to-cell query reads
the actor's second coordinate after the first absolute-value call and loads
the table pointer after both calls. The attacking-goal query caches its table
pointer early and selects row four, column eleven for side zero or column zero
for every nonzero side byte. Its two branches preserve their distinct register
residues and instruction sequences.

All three retain their 64-byte frames, original integer saves, LR/TOC,
volatile and temporary residues, condition bits, PC, exact instruction charges,
ordered accesses and fault prefixes. Late reads and early cached values remain
distinct when a callback changes coordinates, spill bytes or the table slot.
Normal execution does not replay these original bodies. Per-entry original
selection, instruction profiling and outcome counters remain available through
the adapter. The generated code's State layout is unchanged.

The two absolute-value calls retain the existing standard-library adapter.
A local dependency dispatcher calls that adapter directly only when its
four-instruction register-only leaf has enough budget and has no observer,
original-body selection or instruction profiler. That leaf cannot invoke a
callback or transfer a context. Every observable or interruptible case retains
the shared dispatcher and its continuation scope. There is no change to the
general dispatcher or a second absolute-value implementation.

The manifest fingerprint is
`a4f7d16d0ca73d90f7638020783ae8e236c6c2c9e2eac2de95a50afd8b300b76`.
Full generation retains 1,953,334 instructions, 1,922 pages and zero
missing/pending targets. Only `page_101ef.cpp`, `page_1022e.cpp` and
`page_10245.cpp` change among 1,923 generated C++ files. Complete source,
incoming-edge and destination-hook audits pin the three contracts.

## Validation

- **16,357 independent-original component comparisons:** 15,884 normal
  returns, 467 interrupted outcomes and six own/dependency negatives. All 129
  original source sites are reached. Complete state, ordered effects,
  dependency ABI, source order and accounting agree. Independent expectations
  cover every pair of legal pitch cells (11,664 combinations), plus fallback
  and goal-side results. The matrix includes signed-byte/high-bit inputs,
  narrowing after absolute value, table/stack/TOC aliases, live changes between
  dependency calls, relocated TOC, unaligned/null pointers, nonfinite table
  values, exact budget stops and accessed-byte holes.
- **99 actual-production comparisons:** 57 returns (including three
  discriminating own negatives) and 42 budget stops, through direct,
  generated and CFM paths, global/individual original controls, selected
  original absolute-value dependencies and observed dependency boundaries.
  Stops immediately around both leaf calls exercise the direct-call budget
  guard and the shared dispatcher fallback. Fresh
  natural match progression records **90, 890 and 3** dedicated C++23 calls
  and returns, respectively, with no original selections, faults or dispatch
  escapes. Nine separate original-profile comparisons cover all three routes.
- **42 nonlocal comparisons**, including 24 restore/handler faults,
  original and semantic context dependencies and turbo controls. Interruptions
  originate in the query frame and inside its observed absolute-value call.
  Abandoned query frames do not execute stale epilogues or callbacks.
- **Seven focused CTests pass**, including actual match completion, timeline
  undo/redo and the existing standard-library production/profile controls.
  The dependent team-query production regression also passes all 48
  comparisons and records productive calls to each of its four entries. Its
  fresh captures use a new private directory, preserving the prior reference
  inputs. Seven source audits and 42 changed manifest assertions pass. UBSAN
  instruments the three new harnesses, body, adapter, private diagnostics and
  independent original reference; all three runs pass with empty sanitizer
  stderr. Unchanged archives are not newly instrumented.

## Performance boundary

Twenty alternating pairs compare the preceding accepted executable against
this conversion, with one warmup and ten measured 600-tick intervals for each
matchup. Restore, cloning and full-state comparisons occur outside the measured
interval. Both processes start from the same captured inputs with deterministic
host-clock handling. No build or test jobs run concurrently with these timings.

| Home club | Baseline median | C++23 median | Median paired time reduction | Faster pairs |
|---|---:|---:|---:|---:|
| Arsenal | 0.390967 s | 0.391868 s | -0.98% | 2/10 |
| Cruzeiro | 0.363946 s | 0.367116 s | -0.69% | 1/10 |

Both 126,967,858-byte final snapshots agree exactly, as do instruction, budget,
clock, import and sampler accounting. The measured conversion is slightly
slower; it establishes **no speedup**. The benefit of this block is replacing
three heavily reused generated bodies with named, independently verified
C++23 operations. The earlier approximately 31.6% dispatch-runtime improvement
is a separate result and is not attributed to these queries.

## Installed acceptance

Both game and debugger candidates are signed ARM64 applications and match their
installed copies. The preceding signed applications remain private references.
All 845 protected original-input, save, preference and handoff files retain their
hashes. Original application resources are unchanged.

- Game executable SHA-256:
  `193c1ea9d3023b3bb9313de591018546133800af4be5b592fa6aeb25b0f3fb64`.
- Debugger executable SHA-256:
  `b15a3516a827def7a001b7724ef06020a7ff3146b1fa4fff67993681b8a8d19e`.

The installed debugger passes its worker protocol and full-time checks:
Cruzeiro versus Manchester United, seed 12345, 789 actual steps, 1–2 at 92:38,
with completed-step rejection and undo/redo. The installed game reaches its
main menu, displays the original quit confirmation and exits normally; its
mapped executable agrees with the installed hash. These are installed-product
checks, separate from the per-entry coverage above.

Cycle cleanup removes 21 verified disposable outputs and launch directories
(1,717,011,450 logical bytes), recovering approximately 1.72 GB of measured
free space. Reusable libraries/objects, current applications, preceding signed
references, captured inputs, source and evidence remain. All 845 protected
files and installed/candidate identities are verified again afterward.

Save/Load is **not retested** in this cycle. Whole-port completion remains open.
