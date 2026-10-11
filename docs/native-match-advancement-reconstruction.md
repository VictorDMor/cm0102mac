# Match advancement reconstruction

## Complete production block

The watched-match caller, background-match caller and shared advancement state
machine now have named, structured C++23 implementations in `native/match_engine.cpp`.
The default production dispatch uses these bodies. Their football, roster, RNG,
commentary and result-processing dependencies retain their existing implementations.
This delivers advancement reconstruction; the entire football engine remains open.

Generation appends three `match_engine_adapter` entries after the accepted 598
replacements, giving **601**. The unchanged prefix digest is
`946618ec9658d018c53f6bce0dad8f775615d81ad667b37daa40be0ae96ed3c6`.
The complete manifest digest is
`3e5f9144eac7e1691b75eb8e182403bac503d2eb186015ab6d856c3c8b157646`.
The application root, 1,953,334 indexed instructions, 1,922 pages and five source
fixups are preserved, with zero missing or pending static targets.

Intervals are end-exclusive. Pins cover address, assembly, operations, flows and
fallthrough, including six page fragments and the complete bodies.

| Entry | End | Instructions | Operation | SHA-256 |
| --- | --- | ---: | --- | --- |
| `0x101ccdb0` | `0x101cd250` | 296 | Watched/background advancement orchestration | `527418ba2a681667ed734b16006cc80e36568a22bd035c60cec4920b59caa6b4` |
| `0x101cd5c0` | `0x101cd87c` | 174 | Background fixture advancement and completion | `af00bafe26be1f80c76935e07dbc09158f51549602b92942c2aee4a6b6cfd2d6` |
| `0x101d3f60` | `0x101d5658` | 1,470 | Shared advancement state machine | `c21ff22c3c40908b1dd40760a36f6a4444dc33f499b042205187c7bb451eae10` |

Total: **three complete entries / 1,940 indexed instructions**. The unindexed
`0x101cd6ec` word is original NOP padding and is not counted as another instruction.
All indexed instructions are reachable in the exported control-flow graph. The
full source-index flow/fallthrough scan finds zero exterior entrances into these
interiors and no unresolved computed jumps. Missing, duplicated, reordered or
changed bodies and misplaced destination hooks fail the generator audits.

## Preserved advancement behavior

Watched advancement retains acknowledgement and pending commentary, sound and
clock barriers, background servicing, event flags, signed minute conversion and
priority-fixture replacement. Existing maximum-speed presentation behavior stays
in its established dependencies. A null watched fixture still follows the
original background path.

Background advancement retains its low-byte force argument, three/six-tick
hardware gates, counter rollover, signed fixture index and null-fixture guard.
Ordinary advancement calls the shared engine once. Forced advancement retains
the original loop until status 2 or 8. Completion counts once, constructs the
original result objects, publishes them, destroys the finished match and detaches
its fixture. Allocation failure retains the reporting calls and line `0x704`.

Shared advancement retains the signed low-byte work counter, transition handling,
stopped phases, eligible-club notifications, configured abort, period/score
publication, restart announcements and player selection. Clock progression,
random draw order, action/timer processing, live partner selection, ball-owner
changes and team-target refresh remain ordered as in the original.

The actor work queue is read again on every iteration. Actions may append repeat
actors; its count is not a count of players on the pitch. The shared engine keeps
returns 1 (continued), 2 (completed), 4 (presentation/period boundary) and 8
(aborted), including their original guest effects.

Floating calculations preserve the accepted export's separate double multiply
and subtraction, single rounding before negation, signed integer conversion,
sticky status bytes and enabled-summary XOR. Host FMA or algebraic rewrites would
change the probability contract. Time conversions retain the original double
packing and rounding. IEEE exceptional inputs and enabled conversion/inexact
faults are checked against the independent original reference.

## ABI, control and diagnostics

The three entries retain 80-, 112- and 208-byte guest frames. Ordered register
saves/restores, scratch pointers, SP/TOC/LR/PC, CR/XER, floating registers, memory
widths and dependency arguments preserve the original ABI. Checkpoints retain
exact instruction-budget and interrupted-memory prefixes. Structured operations
implement the body; original dependencies execute through normal production
dispatch.

Diagnostics extend the existing opaque `RuntimeDiagnostics` owner. The State
header/layout remains unchanged, with SHA-256
`352fe18e156aaa51cfd4aefca141cdceb699ff0dae97bb217c5d597a5a435f6e`.
Const queries allocate nothing; reset affects the owned match counters. Global,
per-entry and profiling selection of original bodies remain separate from modern
calls, returns, faults, budget stops, known transfers, termination and dispatch
escapes. `CM_AOT_MATCH_ENGINE_ORIGINAL` selects the original development path.

## Focused evidence

Component checks pass **1,189 comparisons**: 215 watched, 190 background and 784
shared-engine cases. They include **967 fault/interrupted prefixes**, **670 exact
budget stops**, one negative for each entry and a changed-child negative rejected
by watched-parent equivalence. Normal cases must return normally. The harness
compares all architectural bytes, generated temporary residues, mapped memory,
allocator state, ordered memory effects, dependency register snapshots and
accounting. Controlled dependencies are component fixtures only.

An actual original match was captured at the shared entry after original lineup
selection and kickoff, using a private runtime and verified pre-match input.
The productive original and C++23 hooks agree for **24 actual engine steps** in
complete guest state, ordered effects and accounting. Dedicated modern call/return
counters establish that the candidate executed. The capture supports this bounded
comparison; it does not create arbitrary-club matches or establish a standalone
debugger.

Actual caller-composition checks pass **53 comparisons**, including five own/child
negative controls and eighteen budget stops. Each entry has direct, generated and
CFM execution evidence, global/per-entry original controls and actual shared-child
composition. Nine separate original-profile comparisons pass. Only external
Microseconds input is fixed; football and RNG dependencies retain their production
implementations.

**21 actual nonlocal comparisons**, including twelve restore/handler faults,
verify the original scalar capture/restore path. All three advancement frames and
their callbacks are abandoned when control transfers to the surviving parent.
Both scalar modes and Turbo composition preserve guest images, ordered effects,
actual import boundaries and accounting outside the measured scalar dependency.
Unknown transfers and resumed abandoned continuations are zero.

Seven focused CTests pass: caller/core equivalence, actual live-entry equivalence,
debugger session control, productive entry composition, original profile and
nonlocal transfer. Fourteen boundary/reference audits and 31 changed manifest
assertions pass. A fresh Release ARM64 build produces **2,057 object files**,
including all 1,922 generated page objects. The final semantic body, adapter,
owned diagnostics and four focused harnesses pass UBSAN with empty stderr and
byte-identical ordinary output. Unchanged generated dependencies are not claimed
newly instrumented.

## Installed artifact and remaining work

The signed installed ARM64 executable SHA-256 is
`084136e6a0dd6835430ec520b24928149a5abb9db537665fe7aa24766874857c`.
Candidate, installer source, installed bundle and owned running executable agree.
The owned installed check renders the original menu in an 800×600 content window,
observes the original quit confirmation and exits normally. Its match counters
are zero: actual match-entry coverage comes from the focused production harness,
not the menu. Original game files, current runtime files, saves, prior installed
references and private handoffs are preserved.

The [standalone debugger contract](native-match-debugger.md) still requires fresh
database initialization, selection of arbitrary clubs, isolated match creation,
a separate application UI and consistent, logged player/goal interventions. Its
session controller now executes the shared productive engine and passes pause,
step, scheduled run, deterministic reset, failed-reset preservation and blocking
after partial budget or nonstandard faults.

Dependent football routines and the broader port remain active work. Career
Save/Load is **not retested**; performance is **not benchmarked**. Cycle cleanup
retains the accepted build, application bundles, original inputs and private
evidence while removing reproducible older build outputs.
