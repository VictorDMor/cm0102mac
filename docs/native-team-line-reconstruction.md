# Team-line queries reconstructed

This block converts the remaining direct generated dependencies of movement
choice and related team-line queries into named C++23 bodies. Selection follows
the priority of accurate full conversion; no timing benchmark or speed
optimization is part of this cycle.

| Operation | Entry | Original instructions |
|---|---|---:|
| Read the complete tactical flags word | `101d56f0` | 2 |
| Query the team's line-adjustment flag | `101d7d30` | 22 |
| Calculate an actor's excess beyond the team column limit | `101e4e10` | 36 |
| Calculate a supplied column's excess | `101e4ea0` | 11 |
| Check whether the opposing row lane is clear | `101e4ed0` | 101 |

The five complete bodies cover 172 original instructions and have 13, 3, 6, 6
and 1 indexed direct call sites respectively. The flag wrapper and supplied-column
query close the two remaining direct generated dependencies of the reconstructed
movement selector. This does not imply that their entire transitive dependency
graph or the complete match engine is reconstructed.

## Preserved behavior

The flags getter returns the whole word. Its wrapper reads bit `0x400` from the
team record selected by the signed low byte of the side argument. It retains
ordered argument spills and reloads, so aliases can affect the effective record.

Column queries choose the side-one limit only when the signed side byte equals
one; all other values select the other limit. Subtraction wraps and the result
narrows to a signed byte. The actor form queries the live column before checking
the designated-actor exemption, then queries the side only when necessary.
Moving the exemption before the column query would change effects and faults.

The lane query first enforces the side-dependent target-column condition. It
scans twenty original actor slots on the side selected by whether the supplied
side byte is zero. Any active actor at the exact target cell blocks it, including
a goalkeeper. A non-goalkeeper also blocks the same row in the selected direction.
Noncanonical side values retain the original asymmetric behavior. Repeated row
and column queries remain separate live reads, with original comparison and
short-circuit order. The scan preserves its terminal pointer and scratch state.

Every body preserves guest ABI, registers/flags, stack effects, LR/TOC/PC,
instruction accounting, ordered memory faults and dependency interruptions.
The ordinary implementations do not replay generated instructions. Independent
references remain private and execute the pinned indexed original bodies.

## Acceptance

All **10,358 independent comparisons** pass: 9,771 returns, 579 interruptions
and eight discriminating own/dependency negatives. Every original instruction
site is covered. Comparisons include complete architectural state and memory,
ordered effects, dependency ABI, instruction-site order and exact accounting.

The matrix covers every tactical side byte and individual flag bit, signed
column differences and byte limits, actor exemption, live query mutations,
both directions and noncanonical sides, active/inactive actors, goalkeeper
exclusions, first/middle/final slots, exact-cell obstruction, adjacent rows,
argument/stack aliases, null and unaligned pointers, relocated TOC, every budget
boundary and accessed-byte hole of representative paths, and each dependency
interruption in those paths.

Seven boundary audits and 45 updated historical manifest checks pass. The prior
672-entry manifest prefix is preserved exactly; the new manifest contains **677**
entries, SHA-256
`bf8519e9fb7ff93e6741076af5a6d27074007dfe67a4d22ebac6c95c0b244822`.
Full generation retains 1,953,334 instructions and 1,922 pages. Only the three
entry-containing translation units `page_101d5.cpp`, `page_101d7.cpp` and
`page_101e4.cpp` change among 1,923 units; the lane body also has an audited
original fragment on unchanged page `101e5`.

## Shared-engine acceptance

Six focused CTest groups pass: the independent component, actual production,
original profile selection, nonlocal transfers, the affected movement-choice
caller and the shared full-time regression. Sixty production comparisons include
fifty returns, ten budget stops and five discriminating negatives, through direct,
generated-destination and CFM descriptor calls. Fifteen profile controls pass.
Full state, ordered effects, accounting and import ABI agree for every entry.

A separate composition check advances Arsenal/Manchester United, seed 12345,
for 600 actual ticks with all five roots selected as original in one state and
as C++23 in the other. Complete final state, per-tick results/accounting and
interval import boundaries agree. All five roots occur naturally, with these
cumulative modern calls and matching return counts in the production test:

| Operation | Calls/returns |
|---|---:|
| Tactical flags | 56,877 |
| Line-adjustment flag | 5,390 |
| Actor column excess | 11,254 |
| Supplied column excess | 14,124 |
| Clear row lane | 45 |

Forty-two nonlocal comparisons include twenty-four restore/handler faults. Every
new root and the nested flags getter abandon their interrupted C++ frames and
callbacks correctly. Original register-context mode requires total accounting
equality; modern mode compares exact accounting outside the measured existing
context dependency. Nested measurements do not double-count calls. There are no
unknown transfers or dispatch escapes.

The affected movement-choice caller passes its 48-comparison production matrix.
The shared completion regression passes its initial 100 Arsenal ticks and the
Cruzeiro/Manchester United full match: 789 ticks, full time at 5,558 seconds,
original/C++23 final state and accounting, fault negatives, full-time event,
undo/redo and rejection of another completed step.

Three UBSAN checks pass with empty stderr. Instrumented units are the new body,
adapter, runtime diagnostics, three validation harnesses and the independent
original reference; unchanged linked archives are not instrumented. All seventeen
direct callees of the movement selector now have semantic owners.

## Installed acceptance

Both signed ARM64 applications share the accepted implementation. Their internal
executable SHA-256 values are:

- Game: `7bbd743a555c1a506aac15075248e38f04ded111ad2fe66535f275e3a8fbc40b`.
- Debugger: `a179587957550f6544fae3b63f8e858effda9353ce49bcbc97ec23ae4ab5a8f7`.

Installation verifies the four original-program resources and preserves the
previous signed applications as private references. All 845 protected original
data, save, preference and private handoff files retain their hashes.

The installed debugger passes its two-pairing worker protocol and completes
Cruzeiro/Manchester United, seed 12345, in 789 steps at 1–2 and 92:38. Full-time
event, completed-step rejection, undo/redo and normal backing cleanup pass. The
game reaches the original menu and exits through its original confirmation;
the running mapped executable matches the installed hash. Menu smoke does not
establish per-entry coverage; the natural match captures above establish it.

Cycle cleanup removes seventeen owned outputs: reproducible check executables,
sanitizer objects and the exited menu-smoke run. It recovers approximately 1.29 GB
of filesystem free space while retaining accepted libraries/objects, current and
previous signed applications, original inputs, snapshots, source recipes and
evidence. Protected hashes and installed identities are verified after cleanup.

Save/Load is **not retested**. The complete C++23 port remains open.
