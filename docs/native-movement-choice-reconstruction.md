# Neighbouring-cell movement choice reconstructed

Four complete operations reconstruct the selector that evaluates neighbouring
pitch cells, its adjustment-byte query and the two pitch-region predicates it
uses. This is a 749-instruction block within the continuing full C++23 port.

| Operation | Entry | Original instructions |
|---|---|---:|
| Select a neighbouring cell | `101d8260` | 704 |
| Read the column-pressure adjustment byte | `101d8d60` | 2 |
| Classify a row outside the central band | `101ee9c0` | 10 |
| Classify a cell in the selected end area | `101eea30` | 33 |

## Selection and behavior

A fresh 12-second native CPU sample of accepted commit `025ee5e` identifies
117 leaf samples in generated page `101d8` and 112 in `101ee`, among the remaining
generated simulation work. This is page-level selection evidence, not an exact
CPU percentage for this block. A separate 600-tick Arsenal/Manchester United
interval observes 1,495 completed selector-entry markers and 8,830 adjustment
queries. The already reconstructed spatial queries remain a larger aggregate
cost; these changes advance a complete remaining football decision.

The selector keeps the original column-major traversal of the neighbouring
cells, signed-byte coordinates and flag narrowing. It applies column pressure,
turning eligibility, role/region restrictions, occupancy, lateral preferences,
nearest-opponent distance, forward/backward progress, central and wide-route
weights, and the previously requested destination. Random draws and repeated
queries occur in their original order. Equal scores replace the prior candidate,
so the later cell wins; unordered floating comparisons retain their original
behavior. Missing candidates write minus-one coordinates, while the early
zero-attribute return preserves the output bytes.

Live counts, context, constants and output pointers are reloaded at the original
sites. Aliased row/column outputs are written column first, then row. The 192-byte
selector frame retains its floating suffix save/restore dependencies, integer
saves, scratch registers, LR/TOC/PC, instruction accounting and ordered fault
prefixes. The three queries are unframed; both region predicates use registers
only. Ordinary bodies are named C++23 operations and do not replay generated
instructions. A single-precision addition operation extends the existing semantic
call helper; existing helper operations are unchanged.

## Independent component evidence

All **6,518 comparisons** pass: 2,059 normal returns, 4,454 interruptions and five
discriminating own/dependency negatives. The independent indexed reference covers
all 749 original sites. Complete architectural state, memory, ordered effects,
dependency ABI, source order and instruction accounting agree.

The matrix checks the independently expected final equal-score cell, all flags
and adjustment bytes, exhaustive signed-byte central-band classification,
end-area boundaries and side selection, field edges, signed coordinate wrap,
lateral directions, zero/full/high-bit occupancy, exclusions, live count/output
changes, random branches, aliases, null/unaligned arguments, relocated TOC,
nonfinite values/constants, budget stops, accessed-byte holes and dependency
interruptions. The dependency negative changes the distance of one candidate and must change
the resulting choice.

## Shared-engine evidence

Natural match progression captures every entry. The first capture interval records
three selector calls, 36 adjustment queries, three central-band queries and 62
end-area queries, each with a normal return. The production matrix passes 48
comparisons: 40 returns, eight budget stops and four discriminating negatives.
Each entry executes through direct, generated-destination and CFM descriptor
paths, with global and per-entry original controls. Twelve profile-mode cases
prove original selection. Full state, ordered effects, accounting and import
boundaries agree, with no original-body residue or dispatch escape.

Thirty-five nonlocal comparisons, including twenty restore/handler faults, cover
the selector frame, its nested adjustment query, its floating suffix save and
restore, and the standalone adjustment getter. Interrupted native frames and
callbacks remain abandoned. The two register-only region predicates have no
memory or dependency callback boundary to intercept; their individual budget,
ABI and productive-call checks still apply.

The nonlocal harness measures the complete existing register-context dependency
family, including nested transfers during floating saves/restores. Original
context mode requires exact total accounting. Modern context mode requires exact
accounting outside those measured dependencies; their previously established
coarser instruction charges are not attributed to this selector. Nested context
charges are counted once, with balanced observer boundaries.

All five focused CTest groups pass, including the shared-engine completion
regression: 100 Arsenal/Manchester United ticks, and Cruzeiro/Manchester United
through 789 ticks and full time at 5,558 seconds. Original/C++23 final state and
instruction totals agree; fault negatives, full-time event, undo/redo and completed
step rejection pass. Seven boundary audits and 44 updated manifest assertions
pass. The full generation retains 1,953,334 instructions and 1,922 pages with zero
missing/pending targets. Only `page_101d8.cpp` and `page_101ee.cpp` change among
1,923 translation units. The manifest grows from 668 to **672** entries with its
exact earlier prefix preserved, SHA-256
`cfd7c91dac9af6ee945faa871e8a923b67ac1245a9d81e1b1db8e44195d71e84`.

UBSAN instruments the two new implementation units, diagnostics, all three
harnesses and the independent reference. All three checks pass with empty stderr;
unchanged linked archives are not instrumented. The new single-precision addition
is exercised through the instrumented selector and its original reference.

## Observed use

Separate 600-tick intervals against Manchester United, seed 12345, record these
productive modern entry calls. These counts are execution witnesses, not timings.

| Operation | Arsenal | Cruzeiro |
|---|---:|---:|
| Cell selection | 1,495 | 1,379 |
| Adjustment byte | 8,830 | 8,487 |
| Central-band query | 7,441 | 6,034 |
| End-area query | 39,017 | 37,553 |

## Historical timing observation

Before the priority was clarified to accurate conversion first and optimization
afterwards, two batches each compared ten paired 600-tick intervals per club
against accepted commit `025ee5e`. Both batches retain identical complete final
snapshots and instruction/import accounting. The first batch's median paired
time reductions are 0.27% for Arsenal and -0.78% for Cruzeiro; the confirmation
batch records -0.24% and 0.95%. Pooling all forty measured pairs gives 0.11% and
0.77%, with the candidate faster in twelve of twenty pairs for each club.
The variation does not establish a consistent speedup. Performance improvement
is not an acceptance requirement for this reconstruction.

## Installed acceptance

Both signed ARM64 applications share this implementation. Installation verifies
all four original-program resources, preserves the previous signed applications
as private references, and keeps all 845 protected original-data, save, preference
and handoff files unchanged. Internal executable SHA-256 values are:

- Game: `16e994df607fe8566b837578f903d69646dd8c0da739dd949def7da99aa96572`.
- Debugger: `3fb265674b5e403f20bb466176f26693cd8e765007d0c5c3f006f617bdd76b5c`.

The installed debugger passes its two-pairing protocol regression, including
lineup/substitution controls, goals, injury withdrawal, rejected commands, reset,
independent state and timeline operations. Cruzeiro/Manchester United, seed
12345, completes 789 real steps at 1–2 and 92:38; full-time event, completed-step
rejection and undo/redo pass. The installed game reaches its original menu and
exits normally through its original confirmation. The running mapped executable
matches the installed game hash. Menu smoke is not individual-entry coverage;
the shared-engine checks and productive counters above provide that evidence.

Cycle cleanup removes nineteen owned outputs: reproducible executables/objects
and the exited menu-smoke run, recovering approximately 1.58 GB of free space.
It retains the accepted build libraries/objects, both current applications, their
previous signed references, original inputs, baseline executable, snapshots,
source recipes and evidence. Protected-file hashes and installed identities are
verified again after cleanup.

Save/Load is **not retested** for this block. The complete C++23 port remains open.
