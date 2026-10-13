# Team spatial queries reconstructed

Four complete team spatial operations now execute named C++23 bodies in the
shared game/debugger engine. The ordered production manifest grows from 656 to
**660** replacements, retaining the previous prefix exactly. These operations
cover 1,624 original instructions; the remainder of the engine is still an
active conversion project.

## Selection and scope

After the preceding dispatch-runtime improvement, native CPU sampling still
showed substantial time in generated pages `101e3` and `101e4`. Entry-read
observations over 600 actual engine ticks identified a coherent group of team
queries used tens of thousands of times in each measured match.

| Operation | Entry | Original instructions | Arsenal calls | Cruzeiro calls |
|---|---|---:|---:|---:|
| Nearest eligible actor | `101e32e0` | 668 | 2,426 | 2,352 |
| Minimum team distance | `101e3f10` | 68 | 15,826 | 14,296 |
| Nearby actor weight | `101e4020` | 66 | 8,136 | 6,125 |
| Both teams' column limits | `101e4130` | 822 | 13,324 | 13,057 |

Both pairings use Manchester United as the opponent and seed 12345. These are
observed call counts, not CPU percentages or a whole-game coverage estimate.
Original code, private input states and raw observations remain private.

## Preserved behavior

Each query scans the original twenty actor slots, preserving the current
position, role and availability checks. The nearest-actor query excludes role
exactly one and permits availability state one. The minimum-distance query
excludes odd roles; the nearby-weight query excludes role exactly one. Both
require availability state zero unless the caller's signed low-byte override
is set. Negative current positions exclude an actor in every operation.

The nearest-actor query retains full-width coordinates and the first actor in a
distance tie. Distance and weight queries narrow coordinates through the
original byte stores and loads. Live TOC constants control the initial minimum,
exact-match sentinel, exact-match distance result and nearby radius. An exact
match contributes three to the weight; another actor within the radius
contributes one. The weight query continues scanning after a match.

Column limits scan the away team first, starting at six, then the home team,
starting at five. They preserve signed-byte comparisons, wrapping final stores,
the second column read for a winning candidate and each original unrolled
slot's scratch-register allocation. A callback can change the value between
those two reads.

The named loops preserve original frame sizes, integer/floating saves, LR/TOC,
volatile and temporary residues, condition bits, FPSCR, PC, exact instruction
charges, ordered memory effects and interruption prefixes. Normal operation
does not replay these four original bodies. Existing position, role, column
and distance dependencies retain their production dispatch paths. Diagnostic
original selection, profiling and per-entry outcome counters remain separate.
Private diagnostics grow without changing the generated code's State layout.

The manifest fingerprint is
`797dc66686e0b700d23ec291d36cb087f6d31d2fadaf0ab5c08be2c0afcbf8ed`.
Full generation retains 1,953,334 instructions, 1,922 pages and zero
missing/pending targets. Only `page_101e3.cpp` and `page_101e4.cpp` change among
1,923 generated C++ files. Complete body, page-fragment, incoming-edge and
destination-hook audits pin the original contracts.

## Validation

- **7,456 independent-original component comparisons:** 363 normal returns,
  7,085 interrupted outcomes and eight own/dependency negatives. All 1,624
  source sites are reached. Full state, ordered effects, dependency ABI,
  instruction order and accounting agree. The matrix includes every budget
  and accessed-byte hole along representative paths, interruptions after each
  dependency, signed-width inputs, role masks, ties, early returns, relocated
  TOC, stack/constant aliases, nonfinite values and repeated column reads.
- **48 actual-production comparisons:** forty normal returns, eight budget
  stops and four own negatives, with direct/generated/CFM entry paths and
  global/individual original controls. A fresh natural match tick records
  respectively **2, 32, 11 and 22** C++23 calls and returns, with no original
  selections, faults or dispatch escapes. Twelve original-profile comparisons
  exercise the four entries through all three paths separately.
- **28 nonlocal comparisons**, including sixteen restore/handler faults,
  original and semantic register-context dependencies and turbo controls.
  Abandoned frames do not execute stale epilogues or callbacks.
- **Five focused CTests pass**, including actual match completion: 100 Arsenal
  ticks and a Cruzeiro match reaching full time after 789 ticks, with original
  state/accounting agreement and debugger undo/redo. Seven boundary audit tests
  and 41 changed manifest assertions pass. UBSAN instruments the new body,
  adapter, private diagnostics, independent original reference and three
  harnesses; all three runs pass with empty sanitizer stderr. Unchanged
  archives are not newly instrumented.

## Paired performance measurement

Identical captured real-match inputs are restored before each 600-tick interval.
Each pairing receives one warmup pair and ten measured pairs, alternating order.
Timing excludes creation, restoration and comparison. The baseline is the
preceding accepted implementation with its environment-lookup optimization
already applied; it executes these original generated queries directly, without
an extra diagnostic-original wrapper. Turbo and instruction profiling are off.

| Match | Median paired time reduction | Faster pairs |
|---|---:|---:|
| Arsenal / Manchester United | 0.46% | 6 / 10 |
| Cruzeiro / Manchester United | 0.34% | 6 / 10 |

All 44 executions, including warmups, preserve instruction/import accounting and
complete final-state equality: 126,967,858 snapshot bytes per pairing. No clock
imports occur inside these intervals. Explicit compiler inlining of the four
bodies and their two hot eligibility/column helpers avoids the initial outlined
helper overhead. The remaining small, mixed timing differences establish
**no consistent speedup**. This cycle's benefit is semantic ownership of four
heavily used operations. The preceding measured 31.6% dispatch improvement is a
separate result, not a gain attributable to this conversion.

## Installed acceptance

Both ad-hoc signed ARM64 applications are installed, retaining their prior
signed versions privately. Metadata-preserving APFS copies and atomic exchanges
preserve the installed layout. Candidate and installed executable hashes agree:

- Game: `f51bf607f03820ee1d0bb19985da1ecd9c791a5f3ee7e699fad8282a067d8284`.
- Debugger: `4860e56b46a41c5fe378409e9022c1ac87e1b61024126efbc02e4e30c157abf5`.

Strict signature checks, the four original-program resource files per app and
845 protected original/game/runtime/save/handoff files pass verification.
The installed debugger passes the two-worker creation, progression, reset,
goal, withdrawal, substitution and timeline protocol checks. A separate
installed run completes Cruzeiro/Manchester United after 789 steps at 92:38,
with undo/redo, completed-step rejection and normal backing cleanup.

The installed game renders its original menu and exits normally through the
original confirmation. Its running executable mapping and hash match the
signed candidate. Menu startup is not claimed as team-spatial runtime coverage;
dedicated counters and actual engine comparisons establish that separately.

Save/Load is **not retested** in this cycle. Whole-port completion remains open.

After acceptance, seventeen validated owned disposable outputs are removed,
recovering 1.30 GB of measured available space. Current objects/libraries and
both installed/candidate apps remain, together with prior signed references,
captured inputs, original inputs, baseline binary, source and validation logs.
All 845 protected files and application identities are rechecked after cleanup.
