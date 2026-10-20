# Native background processing and league loading

Status: active investigation. Turbo is already a native default choice in the
original match-speed selector; see [the installed delivery](native-match-speed.md).
The immediate priority now includes post-match background fixtures and league
loading/new-career creation. General semantic reconstruction remains deferred.

The game is already compiled as native ARM64 C++23. Many routines still express
the original register operations, so a language-standard change alone does not
remove their computational costs. This work changes native execution while
retaining simulation detail, random-number consumption and the save format.

## Whole-game observations

The original background entry `0x101cd5c0` already runs the selected pending
fixture to completion when called in forced mode. Its original one-in-three and
one-in-six scheduling gates apply to incremental mode. Raising that forced flag
again would not establish a performance improvement.

An isolated single-league match reached the final forced call with no pending
detailed fixture: the other fixtures had completed during the watched match and
user-interface pauses. That call took approximately seven microseconds and is
not a useful background-queue benchmark.

A second isolated career selected all playable leagues and the Brazil 2002
start. Creation remained CPU-bound for more than 40 minutes. Its profile changes
between record/attribute preparation, sorting and path resolution, and shared
match simulation. This is diagnostic evidence, not a clean elapsed-time
comparison: profiling, builds and a documented suspension for isolated timing
overlapped the run.

That run captured an actual forced-call manager with 16 detailed fixture records.
A standalone replay cannot yet reproduce the complete host context: post-match
processing also accesses the original live window and filesystem identities.
The incomplete replay is rejected as timing evidence. No matches or native
effects are skipped to make it finish.

The all-league career subsequently finished creation and was saved through the
original UI. Loading, saving a new copy and reopening that copy passed in the
native candidate. An independent daily Continue profile also shows substantial
player/recruitment/list-transition work. The forced fixture entry alone is not
the complete processing bottleneck. A later six-record capture began with its
selected low-detail fixture already calculated; its 4,214,785 instructions and
41 native calls describe postprocessing, not a full detailed-match benchmark.

## Bounded record conversion optimization

Four audited record-field leaves now have generated native scalar-register
versions. Values remain in host integer variables across the complete leaf;
the compiler can eliminate repeated register byte conversions. Memory reads and
writes retain their original order and bounds checks. Stack unwinding publishes
the exact registers, program counter and instruction count even after a fault.

Specialization requires an unlimited instruction budget, safe counter headroom,
and no tracing or active state-observing diagnostics. The final native drawing
observer is eligible because its write callbacks do not inspect or reenter the
guest state. Finite execution, original-PC profiling and other observers retain
the original generated body. Hooks cover actual entry labels, including
intra-page branches; one leaf spans two original code pages.

The four complete instruction contracts are fingerprinted before generation.
Changed, missing, duplicate or unselected instructions fail generation. The
semantic-owner count remains 963; this is a compiler execution optimization.

The initial byte-array shadow-register prototype preserved behavior but was
approximately 2.4 times slower in an isolated leaf test and was rejected. The
scalar implementation passed 1,920 complete-state comparisons, including 880
matching fault cases, aliases, partial writes and native drawing updates.

## Measurement boundary

The measured world operation is the original `0x100c2520` attribute normalization
over a freshly loaded, resolved database containing 153,796 staff and 11,019
clubs. The original loader and normal original dependencies prepare that world;
no saved career is edited. A private capture retains complete initial and final
states. Every timed run must reproduce all final bytes and the same
1,711,959,706 original instructions. This operation makes no native OS calls.

This is a world-processing subsystem measurement. It excludes new-career UI,
league/calendar setup, disk latency, complete rounds and season progression.
The separate raw-database loader screening showed no useful gain from these
four leaves. Near-instant all-league loading and a comparison against Windows
patches remain unproved.

Six alternating baseline/candidate pairs use one warmup and two measured samples
per process, with no build or profiler active. The long-running diagnostic app
was suspended for this interval and resumed afterward; its process CPU time
remained unchanged throughout the timing runs.

| Clock | Baseline median (range), seconds | Candidate median (range), seconds | Median paired reduction | Faster pairs |
| --- | --- | --- | --- | --- |
| Wall | 1.979920 (1.972131–1.989437) | 1.812573 (1.804508–1.833413) | 8.46% | 6/6 |
| Process CPU | 1.964081 (1.957866–1.967743) | 1.797455 (1.780695–1.801167) | 8.53% | 6/6 |

Every final snapshot has SHA-256
`8d6dc447e8fea27dfebf9f17f6cae2c6f504e098ac545747c64d4fe7f913d17f`.
The baseline replay executable is
`56cbbcdfd6b37f6a05857e61ae56f66f7027b8e75327078d660a06a08487f02d`;
the candidate linked against the production archive is
`38352ef7fe2da3a2c77527da2be2c94f8bbc0a4abf15461bfd1d94e92f99b289`.
Both meet the predeclared threshold of at least five faster pairs and more than
1% median paired reduction for both clocks.

## Passive drawing write notifications

Scalar memory writes now check a passive observer's conservative address
interval before making its virtual callback. Native drawing maintains that
interval as surfaces are registered. Writes overlapping even one surface byte
still notify drawing; replacing or removing surfaces can retain a conservative
superset. General diagnostic observers keep every scalar callback by default.
Date diagnostics and bulk-memory notifications retain their original behavior.

Two real initial match states ran 600 shared-engine advances with the actual
native drawing observer and an 8-by-8 surface outside simulation data. This
isolates irrelevant write-notification cost; it excludes UI drawing and complete
rounds. Each case used ten alternating pairs, two warmups and five measured
samples per process, with no build or profiler. All ten pairs were faster on
both clocks in both cases, meeting the declared 9/10 and greater-than-1% rule.

| Scenario | Wall median, baseline / candidate | Paired wall reduction | Paired CPU reduction |
| --- | --- | --- | --- |
| Arsenal | 0.504969 / 0.494202 seconds | 2.08% | 2.12% |
| Cruzeiro | 0.469201 / 0.459789 seconds | 2.02% | 2.06% |

Complete final snapshots, original instruction counts, bridge calls and sampler
counts match. Final snapshot SHA-256 values are
`8d91351c565e85ad26c656693c49936457e952ff5085d66649606fe4818f3356`
and `760a47c644b0d0ab8c7af24c8e9e75d1f08e3abd7d9cfe7022ab3deb0e512eb1`.
The paired replay executable hashes are
`f279c69b4847cd11ae6fccb01fa583f27c63317862949766f4aa4c95be3a1a08`
and `8d36682fa4a9492a3f3cc20c8300493ee81d8e1439621cce818f87978ad5ae63`.
These percentages are separate component measurements and are not additive.

## Checked scalar reads

Host profiles repeatedly place scalar memory reads among the largest costs.
Their ordinary checked load is now forced inline, while optional read tracing
and diagnostic callbacks use one out-of-line helper. Bounds validation still
precedes diagnostics, trace entries still precede callbacks, and the value is
loaded after callbacks so their writes and exceptions retain their effects.
No simulation operation, instruction charge, random draw or state layout changes.

A six-page-only screen missed its declared daily threshold. The independently
measured global implementation covers generated and semantic code. Six
alternating pairs per scenario use two warmups plus ten measured samples for
the short daily operation, one plus three for world normalization, and two plus
four for match intervals. No game app, compiler or profiler ran during timing.

| Captured operation | Wall median, baseline / candidate, seconds | Paired wall reduction | Paired CPU reduction |
| --- | --- | --- | --- |
| Daily staff-link transition, all-league career | 0.044308 / 0.043107 | 2.57% | 2.10% |
| World attribute normalization | 1.811083 / 1.662747 | 8.15% | 8.29% |
| Arsenal shared-engine interval | 0.497138 / 0.483264 | 2.67% | 2.75% |
| Cruzeiro shared-engine interval | 0.462862 / 0.446804 | 3.24% | 3.20% |

All six pairs improve both clocks in every scenario. The daily and world cases
meet the declared greater-than-2% and at-least-5/6 rule; both match cases meet
the declared no-regression rule. Complete final states and original accounting
match. The daily operation retains 15,254,185 original instructions and 114
native calls, with final SHA-256
`8507d05111e1fe67c96d5ed8b0b0295259b103171cffe30c6a65f328601ae7d4`.
The other scenarios retain the final hashes documented above. Daily/world
replay executable hashes are
`9f1bd73843d0d8c20b1ca116c2be221d85b973e9a1925c17a56cf0464e21cb39`
(baseline) and
`f624e7d9529253004b268ebdaeb09afa154dabb45f95e4fd572754e1ef9b48d1`.
The candidate match executable is
`bd29b01632c1000e6910d00a08bc0f9f0d6d1980b0a76631869ce86a58ec73a3`;
its baseline is the preceding observer-filter candidate.

These are bounded subsystem measurements, not a complete day, round or loading
benchmark. A separate local instruction-counter experiment was slower and was
discarded. Exact visit counting in the real daily operation identifies two
8-byte-record heapsorts, including more than 90,000 comparator calls. The first
sort contains 4,246 records and makes no native OS calls. Its complete standalone
replay matches captured final state and accounting. Native sorting is the next
concentrated performance target; that optimization is not implemented here.

## Validation and delivery status

Eleven focused CTests pass: record conversion and profile fallback, match speed,
productive/background/shared-engine composition, nonlocal transfer, import
descriptors and register-context production/profile behavior. Source audits
pass five new contract tests, five match-speed tests, six presentation tests,
seven compiler-arithmetic tests, and the local-temporary suite (five passing,
one optional private-export check skipped; 2,080 differential fixtures pass).
Complete generation retains all 1,953,334 original instructions, with zero
missing static or pending targets.

After adding the observer filter, all 15 focused CTests pass, including memory
bounds, drawing, arithmetic production/profile and the preceding component
checks. The filter has 256 scalar byte-interest oracle cases, high-address and
fault-before-callback checks, general/date/bulk observer preservation, and real
surface expansion, overlap and removal checks.

The checked-read candidate passes 19 focused CTests after its complete rebuild,
adding original resources, filesystem, input-queue and validation-pointer checks
to the preceding production/profile, memory and drawing coverage. New read
checks exercise all four scalar widths, trace-before-callback ordering,
callback mutation of the returned bytes, callback exceptions and invalid reads
that must fault before either diagnostic is reached.

The observer-filter candidate loaded the 26-country career, saved and reopened
a new copy.
A fresh watched match from the immutable pre-match career completed as Cruzeiro
3-1 Uberlandia. All other fixtures returned results. Save, normal exit, reopen,
calendar and player-result verification passed through the original UI. Turbo
was visibly selected by default. This manual match is functional evidence only;
its events differ from an earlier run and provide no matched speed comparison.
The checked-read candidate then loaded that 26-country copy and saved a new
copy. It completed a fresh watched Cruzeiro 2-1 Uberlandia match, with Alex
scoring twice and Ze Raimundo once. Other fixtures returned results. A new
post-match save was written, the app exited normally, and its calendar and
player goal counts were verified after reopening. Turbo was visibly selected
by default. These UI observations are functional evidence, not matched timing.
Reopening the newest all-league copy, creating another full all-league career,
and completing an all-league day with the checked-read candidate are
**not retested**.

At the preceding checked-read delivery, the first three native optimizations
were installed together. That source executable SHA-256 is
`23d498864565b77db1b127d29b7ad8b8b64c0de2152d564f87ad45c0cc4cda53`;
the isolated tested executable is
`bdd3ec5a14d5e40860063d75aec1e3520444612fe34bfb8ba5f2db2b0e4254c9`;
the installed signed executable is
`975aa1ffc09b6a3cd2ef504ea4ec70fe4353e0db465580f6ecc629c92f49e5f4`.
All native Mach-O section bytes, addresses, sizes and flags match. The installed
main menu and normal quit were verified with the expected running binary hash.
Only the installed executable changed;
all 846 protected files matched their immediately preceding hashes. One personal
save had already changed before candidate preparation relative to an older
historical baseline; the newer file was preserved.

The broader performance goal remains active. Near-instant loading is not
achieved. Private programs, snapshots, saved careers, profiles, transcripts and
build artifacts remain under `.local/`.

## Native sorting closure

The 8-byte heapsort at `0x10768660` now has a generated scalar-register frame
covering its generic descriptor tail and both pure pair comparators. This
removes repeated dispatcher crossings while retaining the original algorithm,
comparison order, duplicate handling, arithmetic overflow, memory order and
instruction accounting. The four original contracts contain 159 instructions;
their complete fingerprints are checked before output files are changed.
No original instruction bodies are committed.

Hooks cover the actual entry label, including intra-page calls whose published
PC still belongs to the previous instruction. Unrecognized descriptor targets
publish the complete frame and resume ordinary dispatch. Finite budgets,
original-PC profiling, active observers and original-body policies retain the
ordinary path. Passive native drawing remains eligible. Exception unwinding
publishes partial writes and exact architectural residues, and existing
descriptor/comparator outcome diagnostics remain balanced. A process-local
attempt counter provides coverage evidence without changing saved games.

Six alternating pairs against the preceding checked-read build produced these
median paired reductions, with all six pairs faster on both clocks:

| Complete captured operation | Wall reduction | Process CPU reduction |
| --- | ---: | ---: |
| Actual 4,246-record heapsort | 58.12% | 58.19% |
| Entire staff-link transition containing the sort | 14.76% | 14.76% |

Each sort process used two warmups plus 20 measured executions; each daily
process used two plus ten. Only one process ran during timing, with no game,
compiler or profiler active. Both complete final snapshots and all original
instruction/import counts match. The sort executes 3,675,371 instructions and
zero native OS calls; its entire daily parent executes 15,254,185 instructions
and 114 native OS calls. The previous world normalization and both 600-advance
shared-engine scenarios also preserve complete state and accounting, with no
material measured regression under the declared one-percent rule. These are
component-operation measurements, not an entire all-league day or creation time.

The final build passes 14 focused CTests. The new native checks compare 2,810
complete states, including 2,194 matching faults and every one of 1,610
instruction-counter overflow prefixes across both comparators. Cases include
ties, sentinel values, extreme signed values, unaligned records, stack/array/
descriptor aliases, partial faults, dynamic descriptor side exits, policy and
profile fallbacks, and real drawing surfaces. Ten Python audits pass across
the sort and shared scalar-register lowering. Complete generation still has
1,953,334 instructions, zero pending targets, and 963 semantic owners; only
one existing generated page and the new private sort unit change.

The isolated application reopened the newest 26-country career and logged a
real native 4,030-record sort returning to `0x10768318`. A bounded Continue
processed background fixtures and reached date formatting for 24 December
night. A separate three-second host sample shows the shared match engine and
actor-action path dominating that simulation phase. The original UI was still
processing when this owned diagnostic was stopped; a complete interactive day
and whole-advance timing are **not retested**. The starting private saves remain
unchanged. This identifies the next performance target without attributing the
entire remaining cost to loading or sorting.

The sorting candidate also completed a fresh watched match from the unchanged
private pre-match career. Turbo was visibly checked by default. Cruzeiro won
3-0, with Oseas scoring at 12, Leonardo at 17 and Alex at 68; other afternoon
fixtures returned results. A new private save was written, the app exited
normally, and reopening confirmed the calendar score and one goal for each
scorer. This is functional evidence, not a matched live timing comparison.

The sorting optimization is installed. Source executable SHA-256:
`8e08062e71fb2409708329ceda3e0d34c018451023dbba6c6f8eab652b5ea020`;
tested isolated executable:
`8ad470263abc16971f42a0a2ed66e5d4b45ed5d21217e19eb3a62f56a6a5d803`;
installed signed executable:
`34987dc9fbfe59a36517449773b55166f01f15aca0e51e96f06ab727eb95fcf5`.
All native section bytes, addresses, sizes and flags match. Strict signature
verification passes, and only the installed executable changed. All 846
protected files retained their immediately preceding hashes. The broader
near-instant loading and all-league processing objective remains active.

## Read-only spatial query execution

The subsequent background-match sample identified repeated position/role
queries and their continuation scopes as another cost in the shared engine.
Scalar reads notify the separate read observer; the primary memory observer
receives writes. Pure field reads therefore no longer construct a native
continuation scope solely because a primary observer is present. The position
children of role-table queries can also use their existing direct adapter with
that write observer attached. Their repeated reads and diagnostics still run.

Active read observers retain their continuation scopes. Stores retain scopes
for either observer channel; original-body/profile policies, boundary
observers and insufficient instruction headroom retain ordinary dispatch.
No match detail, scheduling rule, random draw or saved-game layout changes.

Ten alternating pairs against the installed sorting revision, with two warmups
and five measured executions per process, give these median paired reductions:

| Identical 600-advance shared-engine interval | Wall reduction | Process CPU reduction |
| --- | ---: | ---: |
| Arsenal | 7.10% | 7.09% |
| Cruzeiro | 6.38% | 6.38% |

All ten pairs per scenario improve on both clocks. A real native drawing
observer is attached in both versions. Complete final snapshots, original
instruction/import/sampler counts, and all 23 spatial entry call/return arrays
match exactly. Timing runs one process at a time, without a compiler, profiler
or game process. These intervals exercise the engine shared by watched and
background matches; they do not establish whole-round or league-loading time.

Seven focused CTests and seven source audits pass. Production coverage includes
454 original/native cases, 23 discriminating negative controls, 64 budget stops,
primary write observers, read traces and child fault prefixes. Two complete
paired matches, with and without native drawing, finish after 798 advances
with identical complete state and per-tick accounting. Each executes 3,675,990
position queries, 1,098,781 role queries and 987,076 actor-role queries. The
existing 196 nonlocal spatial comparisons preserve all abandoned continuations
and 112 restore/handler faults across read and write observer channels. Shared
engine production, original-profile and nonlocal-transfer checks also pass.

The isolated application loaded the unchanged private pre-match save. Turbo was
visibly selected by default. The original results and player-ratings screens
confirm Cruzeiro 3-1 Uberlandia on 10 February, with two goals for Maicon and one
for Rincon; the other afternoon fixtures also returned results. The original
game wrote a new private save before exiting normally. Reopening that save and
all-league loading are **not retested** in this read-only follow-up. This is
functional evidence, not a matched live timing comparison.

This query optimization is installed. Source executable SHA-256:
`ce427a22d896c54144d15c0a79a4a4ef5154a57a2e7cccaf70aed3dedcec67a9`;
tested isolated executable:
`ec5f1bb0fdaf36c7eb7a59b3c6c703624004ff6d6f2350612356ac35a176e16b`;
installed signed executable:
`2f4230fa0e0450c13fa3db4b80572b68b2e555bb5b8e60977f8bad828c226cf2`.
All native sections match byte-for-byte, including addresses, sizes and flags.
Strict signature verification passes; only the executable changed. The actual
installed process displayed the original start menu and exited normally. All
846 protected files retain their immediately preceding hashes. The remaining
priority is the complete post-match/Continue wait and league initialization;
the measured engine gain does not meet the near-instant objective.
