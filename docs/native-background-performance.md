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

## Current production hotspot diagnosis (2026-11-03)

Reconstruction preserves the original algorithms; handwritten C++23 ownership
does not establish that a subsystem has been optimized. A fresh isolated copy
of the current ARM64 application advanced the reference career from January
11 to 18, 2002. A host-only 2 ms sample recorded 7,139 main-thread observations:
2,104 in the screen-manager branch (including event waits, news and autosave),
and 5,035 in the remaining processing branch. These are sampled stack shares,
not per-routine timers or percentages for an individual calendar day.

| Processing path | Inclusive samples | Share of processing samples |
| --- | ---: | ---: |
| MATCH_ENG, including MATCH_PL and other descendants | 3,024 | 60.1% |
| SHORTLIST_MANAGER, including its contract dependencies | 1,231 | 24.4% |
| MATCH_PL, already inside MATCH_ENG above | 1,244 | 24.7% |
| CONTRACT_MANAGER, mostly inside SHORTLIST_MANAGER above | 425 | 8.4% |

MATCH_ENG and SHORTLIST_MANAGER did not overlap in these stacks; the other
rows must not be added to them. The largest named daily paths were
`scan_candidates` (0x1035e9f0, 15.9%), `refresh_club_shortlist` (0x10357cb0,
13.2%), `share_out_budget` (0x10359020, 12.0%), `distribute_staff_interest`
(0x10363ca0, 5.8%) and `process_daily_contracts` (0x10079050, 4.3%). Their
inclusive times overlap. Candidate scanning walks category lists and performs
eligibility/valuation work for each shortlist row; interest distribution walks
clubs in game order until five accept. Reusing invariant inputs and replacing
repeated lookups are candidates, not measured improvements. Any indexing or
cache must preserve candidate order, ties, mutations and RNG consumption.

A separate host-only sample of the existing 677 match snapshot retained 3,871
simulation observations, excluding 446 driver/copy/comparison observations.
`dispatch_action` (0x102775e0) occupied 58.0% inclusively, `choose_primary_action`
(0x10278220) 32.1%, the role-query family (0x10289560/0x102895a0/0x102895e0)
23.5%, and `best_pass_receiver` (0x101e8950) 22.2%. These paths overlap.
Exclusive top-of-stack groups were dispatch/scope/adapter routing 21.0%, memory
methods 14.3%, guest frame entry/exit 13.2%, generated pages 7.8%, exception
bookkeeping 4.8%, and floating-point helpers 4.2%. Inlined work can instead be
charged to its caller; a generated page is not an original source module.

The first spatial/role optimizations are delivered in `9cb0bba`, `9e62dfa` and
`3182749`: typed internal calls and bounded role/distance reuse preserve the
exported ABI, live state, diagnostics, player order, floating point and RNG.
Further work must profile the accepted code before choosing another shortcut.
Repeated candidate/contract lookups in SHORTLIST_MANAGER remain a separate
measured-cost target; changing RNG or simulation detail is not an equivalent win.

Six unprofiled repetitions of 600 advances each had median times of 0.173226 s
(snapshot 677) and 0.155947 s (2262), with repeated final-state agreement.
These are intervals, not complete matches or new original/C++ comparisons.
Five runs each of existing daily-transition and interest-pass/0 captures
matched their recorded end states, with medians 0.022502 s and 0.016161 s;
each capture is one operation, not a whole day's total cost. The sampled live
Continue request settled in 14.40 s including UI/autosave; it is not a clean
timing or comparable to the historical 26-country week below.

Limits: no matched Windows benchmark, separate complete no-fixture-day timing,
or optimized candidate was produced. The old season-day replay failed native
input validation at GetPort (r19); the fresh 26-country run stopped on an
invalid cursor hotspot and supplies no completed timing. Its cursor selector
was 0xff and SetCursor received a code address; the selector's origin remains
unresolved. The reference-career run above completed. A preliminary profile
with simultaneous guest SIGPROF sampling was discarded for percentage claims;
guest PCs can also remain stale across direct C++ calls. The host-only samples
and weighted stack parsers are private under
`.local/performance/2026-11-03-hotspots/`. Temporary app/runtime/save copies and
the probe executable were removed; only about 11 MB of evidence remains.
No production code, installed application or original save was changed.

## Drawing-aware spatial calls (2026-11-05)

Fresh host-only samples of production `7a23947`, using the existing 677 and
2262 inputs, retain 2,181 and 2,210 simulation observations. Role queries occupy
13.1% and 13.3% inclusively. Each profile runs 100 repeated 600-advance intervals
with native drawing observation, fixed external clock inputs and repeated full
state agreement. These bounded snapshots do not measure a complete game week.

The spatial role/distance wrappers now use their existing direct typed callees
when a native drawing observer is disjoint from the retained stack frames.
Queries contain no drawing boundaries. General observers, stack-overlapping
surfaces, finite budgets, probes and profiling retain dispatch. Guest writes,
frames, fault effects and diagnostic routes remain intact.

Five alternating Release pairs per input, with one warmup and two measured
repetitions per process, reduce median paired wall time by 2.70% and 2.35%
(process CPU 2.75% and 2.39%). Every pair wins on both clocks; wall-time median
absolute deviations are 1.19 and 0.11 percentage points. Both complete 121 MiB
end snapshots are byte-identical to their baselines, including instruction and
import accounting. Focused checks add 22 returned drawing-caller comparisons
and six separately classified injected faults, preserving aliases and observed
stack writes. Private profiles, pairs and compact state-comparison receipts
are under `.local/performance/2026-11-05-current-hotspots/`.

## Checked frame restores (2026-11-05)

A host profile of the January 11–18 replay after the checked word-store change
placed guest frame restoration at 94 of 2,725 main-thread samples. A contiguous
saved GPR area now copies directly into the big-endian register file when the
existing map index certifies the full span and read diagnostics are inactive.
The scalar fallback preserves callback order and the register updates already
completed at an observed fault.

Five alternating Release pairs on the same replay reduced median wall time by
0.91% and process CPU time by 0.85%, with median absolute deviations of 0.24 and
0.15 percentage points. Every pair won on both clocks. Separate untimed runs
matched the recorded state and ordered RNG. The result is for this replay.
Private profile and pair evidence is in `.local/rewrites/runtime-followup/`.

## Installed 95-entry MATCH_PL baseline (2026-11-03)

The Release ARM64 executable from the installed app ran in an isolated
validation bundle. The reference career completed January 11–18 in 13.99 s
from Continue to the settled UI in one run without a debugger or sampler.
Twelve complete morning-to-morning intervals later in January, each with a
match-engine construction, took a median 5.06 s (3.48–7.21 s) with debugger
markers. One interval that crossed an interactive stop was excluded. Host
sampling overlapped part of this run, so these per-day times are diagnostic.
A separate England-only July preseason run also reached match construction on
every complete observed day. Neither scenario supplied a full match-free day.

In 4,479 processing samples from the reference career, inclusive shares were
48.9% SHORTLIST_MANAGER, 38.0% MATCH_ENG and 6.2% MATCH_PL inside MATCH_ENG.
Within 2,778 samples outside the MATCH_ENG branch, `scan_candidates` occupied
59.3%, `refresh_club_shortlist` 51.9%, `share_out_budget` 44.6% and
`process_daily_contracts` 8.4%. The paths overlap. This describes the work
outside the match engine during those days. Five end-state-checked runs of
each isolated operation had medians of 28.62 ms for daily transition and
17.22 ms for the interest pass.

Six unprofiled repetitions per detailed-match snapshot had median times of
166.55 ms (677) and 153.82 ms (2262) for 600 advances. The sampled 677 interval
put 58.1% of simulation stacks in `dispatch_action`, 33.3% in
`choose_primary_action`, 24.3% in `role_from`, 22.3% in
`best_pass_receiver` and 12.6% in `move_player` (inclusive). Ball action
accounted for 6.1% and the reconstructed shot-event root for 0.4%.
Dispatch, guest-memory accesses and guest frame setup/teardown together
accounted for 48.4% of exclusive samples. Prioritize the role/receiver call
chain for detailed matches and candidate scanning for the daily workload.
No cross-platform or before/after speed comparison was measured.

## Warm lineup setup and familiarity experiments (2026-11-04)

The cold isolated factory spent most samples reading commentary resources.
Priming the original resource initializer issued 370 native calls on the first
pass and zero on the second. Seven controlled warm Arsenal–Manchester United
setups returned both starting elevens in 18.56 ms median with 13,580,385
original instructions and 838 native calls. Real career profiles showed no
sustained commentary reads during setup. Suitability pages accounted for 302
of 610 setup samples in the reference career and 51 of 128 in the England-only
preseason. The scorer ran at least 9,851 times per warm Arsenal setup, as counted
at its unconditional rating call. The earlier 2,015 count saw entry dispatches.

The guarded familiarity and rating candidate passed 3,506 exact comparisons
against generated code, including low instruction budgets, guest faults and
dispatch stops. Its paired median wall gains were -0.03% on the staff scan
(nine pairs), -0.42% on daily transition and +0.96% on the interest pass (seven
pairs each). The earlier series without the cross-page stop guard gave +0.92%,
+0.49% and +0.09%, respectively. The familiarity prefix alone gained about 0.4%
on staff scan in two series with comparable variation. Two same-process warm setup comparisons
gave about 0.6% median gains within variation over eleven pairs per fixture.
The variants were discarded. None of these measurements times a full Continue.

The generated condition refresh at `0x10187a40` held 16 of 238 samples in the
warm Arsenal profile. Direct instrumentation counted 21,098 calls per
Arsenal–Manchester United setup and 19,586 per Cruzeiro–Manchester United setup.
Every call in three repetitions per fixture returned at the same-date test
without reaching the condition update or RNG draws. All repetitions in each
fixture matched game-observable state, instructions and native call counts.
The counts include calls entered within a generated page. Evidence and probe
sources are in `.local/performance/2026-11-04-suitability/` and
`.local/performance/2026-11-04-valuation/`.

The condition refresh now copies its original 19-register spill and reload
after certifying the mapped span. The two generated instructions fall back to
scalar execution for fragmented or wrapping spans, relevant observers, and
debug attribute locks. The refresh body still decides when to update records
and draw random numbers. The differential covered 116 original/candidate cases
with full guest memory, registers, ordered effects and instruction counts.

| Warm lineup setup | Baseline median | Candidate median | Median paired wall gain | Process CPU gain |
| --- | ---: | ---: | ---: | ---: |
| Arsenal–Manchester United | 19.20 ms | 16.87 ms | 11.45% | 11.43% |
| Cruzeiro–Manchester United | 17.82 ms | 16.14 ms | 9.62% | 9.61% |

Both setups won all eleven measured pairs. Within each process, pairs restored
the same private checkpoint, alternated order and matched final state and work
counts. With a native drawing observer registered on a guest surface disjoint
from the stack, both setups again won eleven of eleven pairs. The wall gains
were 8.10% and 8.46% and the CPU gains were 8.09% and 8.46%. The spill checks
the observer's write interval before copying, so a disjoint surface remains
eligible and an overlapping surface retains its word-by-word notifications.

Eleven same-process daily-transition pairs gave a 0.95% wall gain with
1.40 percentage points median absolute deviation, after an earlier series
showed a 0.74% loss within variation. The background fixture's 0.88% loss was
within 2.27 percentage points of variation. Both controls reached their
recorded end states. Evidence: `.local/performance/2026-11-04-register-frame/`.

Four additional spill and reload pairs in generated suitability, player
valuation and staff assessment use the same lowering. All eight added sites
matched scalar execution in 248 comparisons with mapped and partial frames,
observed accesses, full guest state and instruction counts. In eleven
alternating pairs against the condition-frame build, wall time improved by
7.86% and 7.80% in warm setup, 1.18% in the staff scan and 1.38% in the interest
pass. The recruitment fixtures each won nine of eleven pairs. Daily transition
was 0.71% slower within 2.98 percentage points median absolute deviation.
All captured operations reached their recorded end states. Evidence:
`.local/performance/2026-11-04-valuation-frames/`.

With native drawing observation active, the added valuation pairs alone
improved warm setup by 7.19% and 6.73% in nine of nine pairs per fixture.
An independent paired comparison applied all five frame pairs against the
pre-frame baseline. Both routes used the same process, warm checkpoint, native
drawing observer and display fixture. Final state and work counts matched.

| Warm lineup setup, all five frame pairs | Baseline median | Candidate median | Median paired wall gain | Process CPU gain |
| --- | ---: | ---: | ---: | ---: |
| Arsenal–Manchester United | 18.09 ms | 15.26 ms | 15.66% | 15.65% |
| Cruzeiro–Manchester United | 16.62 ms | 13.97 ms | 16.18% | 16.17% |

Each fixture won all eleven alternating pairs. The ARM64 production build and
the focused memory, actor attributes, match engine, daily transition and
contract valuation tests pass. The complete Continue comparison is pending.

## Detailed match simulation with drawing observation (2026-11-04)

The earlier role and distance fast paths declined whenever a write observer
was attached. The normal drawing observer usually covers pixels outside their
guest stack frames. The paths now check the actual stack span before deciding
whether they can avoid dispatch and redundant reloads. The conservative span
covers both the role wrapper and its nested lookup. Overlapping writes, wrapped
spans and read diagnostics keep their existing routing.

The movement routine also uses checked copies for its 19 saved GPRs. It reloads
the current guest slots, including any changes made by calls during movement.
Its isolated median wall gains were 0.50% in 677 and 0.71% in 2262, with seven
and ten wins in eleven pairs. The 677 estimate was within paired variation.

| 600 advances with drawing observer | Previous guard | Frame-span guard | Median paired wall gain | Process CPU gain |
| --- | ---: | ---: | ---: | ---: |
| Arsenal (677) | 150.10 ms | 121.14 ms | 19.22% | 19.22% |
| Cruzeiro (2262) | 139.07 ms | 112.60 ms | 19.05% | 19.11% |

Both variants include the movement change. Each series won eleven of eleven
alternating pairs in one process and matched complete guest state, instruction
counts and native inputs. The fixture uses the native drawing observer on a
guest surface outside the stack. Focused checks cover both observer endpoints
and wrapped frames. Both full captures also match with a diagnostic observer.
Evidence: `.local/performance/2026-11-04-handwritten-frames/`.

A separate bulk-frame candidate for action dispatch, primary selection and
pass reception matched both complete replay snapshots. Its integrated build
was 0.35% and 0.27% slower over nine alternating pairs per match despite an
initial 1.26% gain in the 677 prototype. The candidate was discarded.

The four interest passes exercised 467, 436, 444 and 420 dispatched contract
negotiations. The handwritten contract frame copy and the variant that bound
existing GPR values without reassigning them matched the recorded end states.
Each gained about 1.9% in the first series of the smallest pass. The repeated
series found losses of 1.94% and 0.32%, respectively, and the larger passes
stayed within variation. Both variants were discarded. The larger interest
pass `/1` dispatches finance classification 1,787 times, against 27 in `/0`.
Seven checked instrumented `/1` replays attribute 1,110 calls per pass to
Shortlist, 668 to Contracts and nine to the club caller. The classifier took
about 0.235 ms per pass including the child and probe overhead, around 0.4%
of the operation. This leaves the larger recruitment callers as the next
profiling target.

## Pass reception and recruitment caller profile (2026-11-04)

Pass-receiver scoring now calls the verified MATCH_PL position and strategy
queries directly. The strategy query still writes its original frame. The
dispatcher remains active for overlapping stack observation and diagnostic
routes. Each test runs 600 detailed advances with the native drawing observer
attached. The process CPU and wall gains come from nine alternating pairs of
separately linked binaries. Both fixtures won eight of nine pairs.

| Capture | Baseline median wall | Candidate median wall | Paired wall gain | Paired CPU gain |
| --- | ---: | ---: | ---: | ---: |
| Arsenal (677) | 121.90 ms | 121.13 ms | 0.66% | 0.66% |
| Cruzeiro (2262) | 114.31 ms | 113.28 ms | 0.99% | 0.99% |

Complete guest snapshots matched byte for byte. Guest instruction and import
counts agreed in every pair. The targeted receiver checker matched 9,699
returns, and the five MATCH_PL queries matched all 322 focused comparisons.
The production replay also agrees with and without spatial diagnostics. A
separate commentary frame-copy candidate changed wall time by −0.27% and
−0.04% in eleven pairs per capture and was discarded.

In the recruitment operations, the guest sampler attributed 76.7% and 84.0%
of samples to interest registration or its callees. Samples use guest PC and
return addresses, which can remain stale inside handwritten routines. The
instrumented counts and inclusive times below come from five replays per
capture with matching recorded end states. Nested entries overlap and elapsed
times include the probes.

| Entry | Interest pass /1 | Interest pass /3 |
| --- | ---: | ---: |
| Club interest registrations | 7,554 | 3,303 |
| Candidate contribution assessments | 3,469 | 1,316 |
| Human club-control queries | 39,920 / 1.69 ms | 12,076 / 0.71 ms |
| Dispatched protected-player checks | 5,357 / 0.61 ms | 2,585 / 0.29 ms |
| Dispatched release-term queries | 872 / 0.04 ms | 840 / 0.03 ms |

In eleven alternating pairs per replay, the shortlist scan that checked the
first eligible row twice gave −0.39% and +1.25% median wall gains. The
single-read scan gave +0.88% and −0.45%. Bulk spilling the plain club-control
frame gave +0.40% and +0.26%, within paired variation. The three experiments
were discarded. Evidence is under
`.local/performance/2026-11-04-residual-profile/`.

## Candidate contribution and contract valuation profile (2026-11-04)

Seven checked replays per capture timed the phases of candidate contribution.
The estimate was the largest phase on `/1`. On `/3`, the estimate and judge
selection had similar cost. A separate probe timed dispatched dependencies by
return address in five checked replays per capture. Timings include probes and
overlap when one measured routine calls another.

| Mean inclusive time per replay | Interest pass /1 | Interest pass /3 |
| --- | ---: | ---: |
| Candidate contribution | 16.39 ms | 4.21 ms |
| Generated estimate within contribution | 9.46 ms | 1.63 ms |
| Judge selection within contribution | 3.20 ms | 1.51 ms |
| Dispatched potential calculation from the estimate | 1.39 ms | 0.12 ms |
| Negotiated contract valuation | 2.79 ms | 2.12 ms |
| Club funds query from interest registration | 1.74 ms | 1.42 ms |

The dispatch probe found 16,422 and 4,679 staff rating calls. On `/1`, 11,056
came from judge selection and 5,366 came from the potential calculation.
The rating cache experiment covered the judge-selection calls. The staff
rating accounted for 1.93 ms and 0.61 ms inclusive in the dispatch probe.

Each experiment below ran in eleven alternating pairs per capture and matched
the recorded operation state, instruction and import counts.

| Experiment | Paired wall gain on /1 | Paired wall gain on /3 |
| --- | ---: | ---: |
| Skip empty judge slots in groups and defer the reputation read | +0.18% | +0.64% |
| Direct typed staff rating in judge selection | −0.21% | −0.55% |
| Cache staff rating arithmetic with guest-state replay | +0.49% | −0.24% |
| Copy the club funds query's register frame in bulk | +0.08% | −0.08% |
| Reuse the repeated subtraction in negotiated price adjustment | −0.23% | −0.49% |
| Reuse the first plain club-control query's stack image | −1.22% | −0.38% |

These variants were discarded. Grouping eight arithmetic instructions in
staff rating also showed an initial gain, but the unguarded group changed
the NaN payload in a synthetic comparison. The finite-input variant matched
7,000 complete comparisons of registers, scratch, instruction budgets,
return PC and host exceptions. Nine alternating pairs of separately linked
binaries gave +0.45% and −0.18% wall gains, within paired variation. The
arithmetic variant was discarded.

A checked block census inside the generated estimate found 3,273 visits to
`0x10217c58` on `/1` and 298 on `/3`. This block reads ten signed attribute
bytes, spills four intermediate words and sums the half-values. It accounted
for 225,837 of the 2,434,535 instructions charged to measured estimate-body
blocks on `/1`, the largest single block in that census. These are instruction
counts. In the separate instrumented comparison, the guarded native sum
reduced mean block time from 50.3 ns to 20.0 ns on `/1`, saving about 0.10 ms
per replay. All visits used the guarded route. Whole-replay wall time
increased by 1.98% on `/1` and 0.24% on `/3` in eleven alternating pairs.
The replay end states agreed and the candidate was discarded. The phase and
block censuses are under `.local/performance/2026-11-04-contribution/`.

## Pass-receiver phase costs and geometry leaf (2026-11-04)

Seven matched captures with a native drawing observer timed the receiver and
its phases. The probes also counted and timed the original commentary and
staff relationship routines at their actual entry and return labels. These
inclusive times overlap where a measured routine calls another and include
probe overhead.

| Mean inclusive time over 600 advances | 677 | 2262 |
| --- | ---: | ---: |
| Pass receiver | 25.86 ms | 21.44 ms |
| Receiver reachability | 9.55 ms | 8.27 ms |
| Receiver exposure | 4.77 ms | 4.04 ms |
| Primary action selection | 39.60 ms | 33.16 ms |
| Movement | 19.07 ms | 19.97 ms |
| Commentary | 1.23 ms | 1.02 ms |
| Both staff relationship queries | 0.048 ms | 0.066 ms |

Direct staff relationship queries from pass reception changed median wall
time by −0.20% and −0.36% in eleven pairs. That candidate was discarded.
The typed MATCH_PL geometry routines now perform the absolute-value leaf
directly, including its TOC/link effects, two instruction budget boundaries
and carry. The leaf itself accesses no guest memory.

| Capture | Baseline median wall | Candidate median wall | Paired wall gain | Paired CPU gain |
| --- | ---: | ---: | ---: | ---: |
| 677 | 120.98 ms | 116.52 ms | 3.55% | 3.49% |
| 2262 | 113.10 ms | 109.56 ms | 3.41% | 3.41% |

The separately linked binaries won all nine alternating pairs in each
capture and produced byte-identical final snapshots. The same-process
comparison also matched the instruction, import and clock counts. The
absolute-value differential matched 7,168 complete register and budget
cases, including the signed minimum, both budget stops and ordinary
returns. The production build matched 648 geometry comparisons and both
600-advance replays with and without spatial diagnostics. The production,
audit and match-replay CTests all passed. Evidence is under
`.local/performance/2026-11-04-estimate-block/`.

## Negotiated valuation stages and module routing (2026-11-04)

The Contracts probes separate the valuation of a player already held by
the shortlist row from that of the incoming candidate. Each replay reached
436 negotiations on `/1` and 420 on `/3`. Seven checked replays per capture
gave the following mean inclusive times. Child timings overlap the parent
stage and include instrumentation.

| Phase per replay | Interest pass /1 | Interest pass /3 |
| --- | ---: | ---: |
| Preparation | 0.93 ms | 0.87 ms |
| Base quote within preparation | 0.50 ms | 0.48 ms |
| Relationships and potential | 0.47 ms | 0.21 ms |
| League relationship query | 0.40 ms | 0.15 ms |
| First league-table lookup within the relationship query | 0.30 ms | 0.08 ms |
| Regional shortlist row lookup | 0.08 ms | 0.08 ms |

On `/1`, the deeper league comparison ran 29 times per replay for Contracts
and 34 times for Shortlist. The first table lookup can sort the records
before searching. The three indirect predicates tested later in the branch
together accounted for under 0.01 ms per Contracts replay. The potential
calculation's two arithmetic tails ran 5,808 times on `/1` and 802 on `/3`.

| Experiment | Paired wall gain on /1 | Paired wall gain on /3 |
| --- | ---: | ---: |
| Direct club classification, separately linked | −0.33% | +0.23% |
| Native potential tail, in-process | +0.45% | −0.10% |
| Swap league records through two temporary buffers, in-process | −1.65% | +0.32% |
| Swap league records two words at a time, separately linked | −1.26% | −0.77% |

All candidates matched the recorded operation states, instruction counts and
import counts. The direct classification and two-word swap prototypes gained
0.72% and 1.17% on `/1` in their in-process series. The gains did not repeat
in the separately linked series and the four candidates were discarded.

The module controls below compare the current default against the same
binary running the selected module's original bodies, one module at a time. They
use seven alternating pairs per capture, retaining the other modules'
default routes. Both routes match the recorded operation state and native
import count.

| Default C++ median wall gain over original | Interest pass /1 | Interest pass /3 | Daily transition |
| --- | ---: | ---: | ---: |
| Contracts | 0.81% (6/7 wins) | 0.93% (7/7) | 1.44% (4/7) |
| Shortlist Manager | 5.07% (7/7) | 2.86% (7/7) | 0.46% (4/7) |

The daily-transition results varied by pair.

The match controls used the same binary and a native drawing observer on a
surface disjoint from the guest stack. They timed 600 advances per run and
alternated the original control and default route. The default was faster
in all seven pairs for each control and capture. Every run matched
the live operation state, RNG, clock and import counts.

| Default C++ median paired wall gain over control | 677 | 2262 |
| --- | ---: | ---: |
| Match Engine forced original | 18.79% | 18.01% |
| Match PL forced original | 9.36% | 7.35% |
| Match Engine and Match PL forced original | 20.07% | 19.46% |

The controls overlap because the modules call each other. The route
comparisons and experiment logs are under
`.local/performance/2026-11-04-valuation-stages/`.

## Complete 26-country Continue diagnosis

A separate private diagnostic on revision `7430110` now completes one original
Continue from 21 December 2001 morning to 28 December morning. The final manager
news screen was visually verified responsive. No further Continue input was
sent while it processed. This supersedes the earlier incomplete observations
as a completed functional weekly advance; it is still not a matched speedup
comparison or a new-career creation measurement.

From delivered confirmation/Continue input to the final presented frame:

| Complete operation | Wall seconds | Process CPU seconds | Original instructions |
| --- | ---: | ---: | ---: |
| Load the unchanged 26-country saved career | 8.385 | 8.259 | 5,934,629,856 |
| One Continue, seven calendar days | 497.314 | 495.694 | 230,833,069,683 |

The instrumented shared engine records 1,723 completed intervals, comprising
2,513 advances and 166.629 wall / 165.726 CPU seconds. These include finalization
of already-calculated low-detail fixtures; the count is not a claim of 1,723
unique detailed simulations. Recorded completed engine work accounts for about
one third of the complete wait. The remaining work must be measured and improved
alongside football simulation. In a separate three-second sample during daily
world processing, recruitment distribution/club-interest registration accounts
for 934 of 2,319 main-thread samples; generated utility/descriptor paths are also
substantial. Three other samples during match processing remain dominated by
the shared engine and actor actions. These local samples describe their phases,
not whole-week CPU percentages.

No compiler or other game ran during this diagnosis. It adds clocks and sparse
completion logs, retains the private validation app's ordered RNG-boundary
digest, and includes four three-second host samples. These elapsed times are
diagnostic observations rather than clean benchmark acceptance. The
private diagnostic executable SHA-256 is
`a96a2ffc679f4825cb6c82ab5d24bd87631fedfdc1e7f068107912ab2e444d77`;
its instrumentation is not installed. A new private weekly result was written
on normal quit; reopening is **not retested**. The initial save and all 846
protected files remain unchanged. Near-instant loading and progression remain
unachieved. The next priority is the complete daily world/recruitment/utility
work, using this completed scenario to judge useful improvements.

## Daily world cost by function

A second private diagnostic measures actual recruitment, policy, link and
heapsort entries rather than inferring their identity from a generated page.
The compiler merges instruction suffixes within a page, so a sampled host
address can represent several original continuations. Page membership alone
does not distinguish sorting from other work.

The unchanged 26-country career again advances from 21 to 28 December morning,
returning to the original news screen. This run disables only the private
ordered RNG-boundary digest; it retains the original RNG and input handling.
Its observed Continue takes 481.334 wall / 481.815 process CPU seconds and
227,666,002,891 original instructions. Real clock inputs, transfer outcomes and
match workloads differ from the first diagnosis, so the elapsed difference is
**not** an accepted speedup. No compiler or host profiler ran during this run.

The additional timers report these process-wide totals, including load and
save. Parent times include nested children and must not be added together:

| Entry | Calls | Inclusive wall seconds |
| --- | ---: | ---: |
| Distribute staff interest | 2,423 | 76.305 |
| Register club interest, nested in distribution | 13,517,821 | 71.515 |
| Transition staff links | 3,199 | 62.076 |
| Check candidate country policy | 46,479,823 | 13.833 |
| Evaluate row contract gate | 15,191,016 | 17.712 |
| All heapsorts | 9,606 | 23.023 |

The existing two native pair-comparator sorts account for approximately 12.020
seconds of that sort total. Another eight-byte comparator (`0x10309a00`) accounts
for 10.580 seconds across 75 sorts and 7,560,945 rows. Other sort configurations
are substantially smaller in this run. The repeated recruitment/link work is
the larger next target; this measurement does not justify attributing all
remaining delay to the sort routine.

The diagnostic executable is private and not installed, SHA-256
`eb2e24a4cf1f773ad9f0ca026395dcc6903af45311a73749d4c0cf553045a5e8`.
It wrote a new private weekly result and exited normally. Reopening is **not
retested**. The starting career and all 846 protected files remain unchanged.

A separate compiler experiment forced inlining of 83 existing guest-operation
methods across 240 production translation units. Four alternating pairs retain
identical complete final states and accounting in the daily transition, world
normalization and both shared-engine intervals. It fails the declared gain
threshold: the daily transition is 1.12% slower on wall time and 1.11% slower on
process CPU; the other scenarios show no useful gain. This experiment is
rejected and is not installed. Broader inlining alone does not solve the
repeated world-operation cost.

A private native-endian register-cache experiment also retains complete states
and accounting. Its corrected variant permits dependency dispatch with canonical
register publication and reload. It fails the declared control-scenario gain
threshold: daily transition is 0.48% slower on wall time, world normalization is
unchanged, and one match interval is 2.21% slower while the other is 0.90% faster. Broad
deployment is rejected. These controls do not capture the dominant complete
recruitment-distribution pass, so they cannot establish its speedup.

## Accumulated bounded-operation gains

A direct comparison uses the frozen production AOT/UI archives from `4bc51cd`
and the currently accepted archives at `582532a` (performance source `7430110`).
The reference already contains the first native-default-Turbo delivery; gains
already present in that delivery are outside this comparison. This measures
accumulated improvements directly rather than adding earlier percentages.

Both versions run the same harness and immutable complete captured inputs.
Six alternating pairs per scenario use one process at a time, with warmups,
separate wall/process CPU clocks, and no game, build or profiler. Every pair
preserves complete final state and original instruction/import accounting; the
match cases also preserve clock, sampler and all 23 spatial call/return counts.
All six pairs are faster on both clocks in every scenario.

| Complete bounded operation | Median paired wall-time reduction | Process CPU reduction |
| --- | ---: | ---: |
| Captured staff-link daily transition | 18.42% | 17.63% |
| World attribute normalization | 15.33% | 15.60% |
| Arsenal shared-engine interval, 600 advances | 11.37% | 11.66% |
| Cruzeiro shared-engine interval, 600 advances | 10.63% | 10.37% |

An unrelated Blender process was active during timing and was left untouched.
These are approximate bounded-operation estimates, supported by alternating
order and consistent process CPU gains. They do not establish a whole-game,
whole-match, league-loading or complete-week percentage, nor a percentage of
completion toward near-instant all-league play. The observed weekly diagnostic
still takes approximately eight minutes. The target remains unachieved.

Private evidence is retained in
`.local/native-background-performance/cumulative-progress-01/`: exact archive
and executable hashes, compiler arguments, individual pairs, full snapshots and
summary. No new production executable is installed for this comparison.

## Native side-tendency arithmetic

`update_side_tendencies` retains ratings and floating-point status in native
values between audited legacy-call boundaries. The same formulas use immediate
register access under state-inspecting diagnostics; only no observer, the final
native drawing observer and the exact active passive replay observer qualify
for deferred publication. Re-audit the guard if a child gains callbacks or can
install observers. Preserve strict floating-point operations, conversion
exceptions and RNG draw order when extending this pattern to another chain.

## Shortlist link index insertion

The production add-link owner appends to both indexes as before. For mapped,
disjoint indexes with a strictly ordered prefix and unique nonnegative IDs, it
inserts the new pair into that order and runs qsort on the two smallest pairs.
That last step reproduces the full heapsort's final registers, saved stack
bytes and descriptor linkage. The full sort remains for other inputs, routine
checks, finite instruction budgets and observers that can see the writes.

Seven alternating Release pairs per capture use the same replay harness and
archives with only the production link insertion object replaced. Each side
runs the same four iterations per process and discards the first timing. All
pairs pass the recorded final-state and input checks and make the same native
imports. The original guest instruction counts decrease as expected.

| Complete captured operation | Baseline wall | Candidate wall | Median paired wall reduction | Process CPU reduction |
| --- | ---: | ---: | ---: | ---: |
| Interest pass 1 | 52.72 ms | 46.77 ms | 10.57% | 10.57% |
| Interest pass 3 | 30.43 ms | 16.19 ms | 46.74% | 46.74% |
| Daily transition | 19.92 ms | 17.02 ms | 14.04% | 14.04% |

Every pair is faster on both clocks. A separate differential check compares
every mapped byte, register and generated temporary after the full and
two-row qsort paths. It passes the captured 4,246-pair staff index and 40
synthetic cases covering both comparators and insertions at the beginning,
middle and end. Private commands, logs and hashes are under
`.local/performance/2026-11-04-continue-replay/`.

Seven more pairs per capture compare the rebuilt default route directly with
`CM_SHORTLIST_MANAGER_ORIGINAL=1` in the same executable. The median wall
reductions are 15.31%, 49.19% and 17.39% respectively, and the process CPU
reductions are 15.31%, 49.17% and 17.38%. Both clocks improve in all pairs.

The older full-day capture still stops at an input mismatch before the
boundary. The replay reaches `CarbonLib::GetPort` with the expected return
address but reads r19 as 1 instead of 3. The saved return address confirms the
event pump in the generated match batch at `0x101cc084`. Forcing the original
MATCH_ENG, MATCH_PL or MAIN_PROGRAM independently leaves the same mismatch.
This failure predates the insertion change. The table measures the bounded
operations only.

The ARM64 app builds and verifies with a strict ad hoc signature. Six of the
seven focused CTests pass. The remaining corruption control stops in the
game-load transcript at the same allocator mismatch before and after the
change, before it reaches its deliberate comparator corruption. The
daily-transition replay detects a deliberate write corruption in add-link
at its first checked call. The installer stops at its process check.

## Relation comparator fusion

The shared eight-byte heapsort now fuses the audited PLAYER_STATS relation
comparator at `0x10309a00` with its descriptor call. It keeps the original
heap traversal, comparator calls and swap order. This matters for the captured
100,790-row table, which contains two equal keys with different payloads.
The original dispatcher still handles diagnostic and unsupported targets.
The two staff-club comparators retain their own fused loop.

The benchmark sets up a sort call with complete real-career snapshots. It
uses the captured relation table unchanged, then moves one row to the end to
model an append. A second snapshot supplies a reverse-order stress input.
Each measurement compares the full resulting memory, registers and generated
temporaries with a separate unfused run. Seven alternating pairs per input
exclude the first of four timings in each process.

| Relation-sort input | Baseline wall | Fused wall | Median paired wall reduction | Process CPU reduction |
| --- | ---: | ---: | ---: | ---: |
| Captured order, 100,790 rows | 99.79 ms | 49.13 ms | 50.76% | 50.76% |
| Appended row, 100,790 rows | 98.96 ms | 49.36 ms | 50.09% | 50.08% |
| Reverse order, 100,783 rows | 93.62 ms | 46.38 ms | 50.46% | 50.43% |

Both clocks improve in all 21 pairs. The ordinary C++ comparator skips
original instruction accounting, so the new route charges more instructions
than that baseline. Against the generated comparator it charges the same
instructions. Neither variant issues native imports during the sort.
The earlier weekly diagnosis observed 75 calls with this comparator and
10.580 seconds in their sort paths. The paired benchmark measures the sort
with controlled entry state. A matched weekly Continue remains unavailable.

The symmetric-link controls preserve the recorded operation state and
instruction/import counts in seven pairs each. Median paired wall time moves
by +1.24% (about 0.02 ms) on the existing staff-club sort, +0.07% and −0.32%
on interest passes 1 and 3, and +0.94% on daily transition. Logs, replay
sources and executable hashes are under
`.local/performance/2026-11-04-relation-sort/`.

Five generator audits and six focused CTests pass, including 4,459 sort
differentials with faults and budget cuts. The regenerated ARM64 app passes
strict signature verification. Installation stops at the process check.
The old full-day replay still diverges at GetPort with original English
competition rules selected.

## Spatial query routing per scan

Five MATCH_ENG spatial routines now check eligibility for direct MATCH_PL
role and distance calls once per scan. The check follows the parent's
potentially observed writes, including the line-limit publication between
teams. It covers the two nested role frames. The queries still execute in
their original order with live guest reads and full frame writes.

| Captured operation | Baseline wall | Candidate wall | Paired wall reduction | Process CPU reduction |
| --- | ---: | ---: | ---: | ---: |
| 600 detailed advances, 677 | 120.20 ms | 115.69 ms | 3.88% | 3.89% |
| 600 detailed advances, 2262 | 112.79 ms | 108.43 ms | 3.83% | 3.82% |
| Background fixture | 4.14 ms | 4.05 ms | 2.00% | 2.27% |

Nine separate-binary pairs per detailed fixture keep native drawing
observation attached. All 18 pairs improve on both clocks and retain
byte-identical final snapshots and instruction/import counts. The background
fixture uses the original wrapper epilogue to normalize its recorded end
state and wins six of seven pairs on both clocks.

A write observer that enables the dispatcher during the parent's stack
writes or the line-limit publication sees all 74 subsequent role and distance
dispatches across four cases. Moving the check before the parent's writes
loses calls in the negative control. A separate variant that removes
temporary register updates from the role leaf gives +0.19% and −0.50%
incremental wall gains and is discarded.

The new in-process signal sampler overrepresents two generated pages in
these inputs. Direct boundary timing assigns 1.15 and 1.01 ms per replay to
commentary, of which about 0.10 ms is the weighted row scan. The sampled
negative relationship call takes 0.04 and 0.06 ms. Those direct timings
governed prioritization. Evidence is under
`.local/performance/2026-11-04-match-current/`.

The five focused CTests and 255 live original comparisons across the five
changed entries pass. The ARM64 app builds and passes strict signature
verification. Installation stops at the process check.

## Contract club resolver frame

Interest pass 1 reaches 17,107 contract lookups by club. Of those, 10,197
return the primary contract and 6,867 reach the club resolver after the
secondary lookup. The two largest caller sites account for about 1.76 ms
of inclusive club-query time in the instrumented replay.

The generated resolver now uses the existing checked bulk copies at its
19-register spill and reload. The original scalar instructions handle
observed accesses, locks and fragmented mappings. Only the two resolver
pages change. Eighty site comparisons preserve fault prefixes, registers,
mapped bytes, access traces and instruction counts.

| Captured operation | Baseline wall | Candidate wall | Paired wall reduction | Process CPU reduction |
| --- | ---: | ---: | ---: | ---: |
| Interest pass 1 | 45.74 ms | 44.31 ms | 3.09% | 3.08% |
| Interest pass 3 | 16.16 ms | 15.80 ms | 2.08% | 2.15% |
| Daily transition | 16.90 ms | 16.65 ms | 1.50% | 1.51% |

Nine alternating pairs per operation preserve recorded end state and
instruction/import counts. The recruitment inputs win nine pairs each
and daily transition wins six. Interest passes 0 and 2 also pass their
recorded-state checks. The final secondary-index reuse experiment, compared
with frame copies already active, changes wall time by +0.63%, −0.75%
and +0.29% respectively. It is discarded. Its exploratory read-order
assertion also fails with the preceding lookup implementation. Private
sources and logs are under `.local/performance/2026-11-04-contract-query/`.

The older full-day replay reaches 116,253 club-contract queries before
stopping at the GetPort mismatch. The lineup suitability call at
`0x10211f34` accounts for 99,190 of them, including 65,394 primary hits,
210 secondary hits and zero resolver calls. This census covers an
incomplete day. It identifies the lookup prefix as a separate target from
the resolver's frame.

The early-return path publishes all five guest frame stores and reuses
the saved register values when the primary lookup finishes without calling
another routine. Diagnostic paths retain the full frame. Secondary and
resolver calls retain the live frame and reload its saved words.

Nine separately linked alternating pairs replayed the 99,190 recorded calls
against the first lookup's snapshot. The median paired wall gain was 17.62%
(9/9 wins, 1.29 percentage points MAD), with exact return values, instruction
counts and byte-identical final snapshots. The fixed snapshot sends 28,494
calls to the resolver while the live day prefix sends none. In the day replay,
the two builds record identical lookup inputs and results through the same
GetPort mismatch. The focused frame check matches 194 cases, including
overlapping observers, stack aliasing and partial faults. Seven alternating
pairs of warm lineup setup gained 1.10% for Arsenal (7/7) and 0.51% for
Cruzeiro (4/7, within variation). Both setups reach only the no-primary
return at this call site. Interest passes 1 and 3 and daily transition
completed with identical recorded outcomes and gains within variation.
Private evidence is under `.local/performance/2026-11-04-lineup-contract/`.

## Shortlist staff name formatting (2026-11-04)

The prior sampled reference week has 124 of 5,035 processing samples in
`record_name_equals`. On the controlled staff scan, 288 name calls take about
0.45 ms for formatting and 0.01 ms for comparison per replay, including
probes. The full-name `sprintf` accounts for about 0.30 ms and makes 801
BlockMoveData imports. Daily transition and interest pass 1 do not enter
the name comparison.

Nine alternating pairs per variant preserve the recorded end state and
instruction/import counts. Allocation-free BlockMoveData, bulk formatter
frames, direct string leaves and the two formatter changes together give
median staff-scan wall gains of −0.15%, +0.37%, −1.62% and −0.28%. Discard
the four variants. Private measurements are under
`.local/performance/2026-11-04-shortlist-names/`.

The staff scan reaches `0x103f43b0` 28,286 times. Every call takes the
flag-clear return and destroys the empty stack list it just constructed.
Daily transition and interest pass 1 reach this return 3,191 and 606 times.
The candidate scan can publish both frames and the return state directly
when the mapped stack is disjoint from the inputs and diagnostics are off.
The original route handles the other cases.

The complete staff scan improved by 3.29% wall and 3.29% CPU in nine
separately linked alternating pairs, with nine wins on both clocks.
The earlier variant limited to the destructor gained 0.34% within
variation. The daily-transition and interest-pass wall gains of 0.92%
and 0.97% did not repeat above variation. The focused comparison matches
144 generated-code cases and four decline controls for the guarded path.
The Release build passes all eleven selected CTests and leaves a
byte-identical final staff-scan snapshot against the preceding build.

## Staff score arithmetic (2026-11-04)

The controlled staff scan enters the scorer at `0x101ad080` 15,551 times.
The role adjustment takes about 7.3 ms per scan, the final adjustment
4.4 ms and the familiarity/rating call 3.1 ms, including the probes.
The four relationship checks total 1.4 ms with probes and all return
zero on this input. The scorer executes 258,188 integer conversions.

Nine separately linked pairs rejected both changes to `fctiwz`. Packing its
status writes gained 0.53% within staff-scan variation and the direct
conversion variant was 2.45% slower. Packing the generated arithmetic
status suffix was 1.87% slower. All three retained the recorded end states.
Private measurements are under `.local/performance/2026-11-04-staff-score/`.

The generated role block at `0x101af000` can forward 44 scratch loads
from the registers that still hold the saved words. The compiler pins the
call-free block to its exported instructions. The entry guard keeps the
scalar loads for observable stack effects, a wrapped frame or a dispatch
into the middle of the block. Both routes publish every original store.

Nine alternating pairs of the normal Release builds give a 1.69% median
paired wall gain and a 1.53% CPU gain in the controlled staff scan, with
eight wins on both clocks. The final snapshots match byte for byte. The
focused differential matches 125 cases, including changed stack values
from observers, debug locks, faults and budget boundaries. Nine CTests pass.
The recruitment and daily-transition controls have no visits to the block
in the dispatch census. The recruitment slowdown from the overlay build
did not repeat in the normal builds. Neither control shows a consistent
change on both clocks.

## Detailed match actor queue (2026-11-04)

Five instrumented replays per capture time the actor queue at 13.9 ms
per 600 advances in 677 and 13.8 ms in 2262. The queue comparator accounts
for 10.2 and 10.1 ms inclusive of probes, with 78,676 and 77,647 calls.
MATCH_PL intensity scaling takes about 0.6 ms including probes in each
capture despite more than 13,000 calls.

| Comparator variant | 677 paired wall gain | 2262 paired wall gain |
| --- | ---: | ---: |
| Reuse the saved conversion words | +0.72% (5/9) | +0.87% (6/9) |
| Also copy the saved GPR range in bulk | +0.19% (5/9) | +0.92% (6/9) |
| Reuse the restart and pinned-player reads | −1.53% (1/9) | +1.03% (6/9) |

Nine alternating pairs per capture and variant reached identical advance,
instruction and import counts. The process CPU results follow wall time.
Discard all three variants because none improves both captures consistently.
Private measurements are under `.local/performance/2026-11-04-match-intensity/`.

The two descriptor call sites in the four-byte sort reach the comparator
78,676 and 77,647 times per capture. The descriptor adapter takes about
1.6 ms per capture including timing probes. Regenerating the baseline page
for the overlay reproduced the production page byte for byte.

| Follow-up variant | 677 paired wall gain | 2262 paired wall gain |
| --- | ---: | ---: |
| Return directly to the sort after the descriptor and comparator | +0.65% (9/11) | +0.38% (6/11) |
| Also specialize the checked descriptor tail | +1.01% (8/11) | +0.74% (8/11) |
| Fold four consecutive arithmetic status updates in the comparator | +0.24% (7/11) | −0.20% (4/11) |

Each series preserves advance, instruction and import counts. The process CPU
estimates follow wall time. Discard these variants because the gains fail to
repeat above paired variation in both captures. Private measurements are
under `.local/performance/2026-11-04-match-callback/`.

## Shortlist row contract gate (2026-11-04)

Five checked replays per capture time the production C++ gate at
`0x1036fbf0` and the country policy at `0x1036f590`, including calls
that skip the generated dispatcher.

| Replay | Gate calls | Gate time | Country policy calls | Country policy time |
| --- | ---: | ---: | ---: | ---: |
| Daily transition | 10 | 0.07 ms | 3,189 | 0.09 ms |
| Interest pass 0 | 4,678 | 2.63 ms | 1,883 | 0.10 ms |
| Interest pass 1 | 29,835 | 15.17 ms | 1,882 | 0.10 ms |
| Interest pass 2 | 5,302 | 2.85 ms | 1,883 | 0.10 ms |
| Interest pass 3 | 7,088 | 3.96 ms | 1,590 | 0.08 ms |

Times are per replay and include the timing probes and nested calls. Every
gate call in these captures reaches the expiry-band branch. Separate nested
timers attribute the following inclusive times to the four dependencies.

| Dependency | Interest pass 1 | Interest pass 3 |
| --- | ---: | ---: |
| Contract start-date window (`0x10380f20`) | 3.38 ms | 0.84 ms |
| Transfer eligibility (`0x103faff0`) | 2.52 ms | 0.62 ms |
| Transfer price (`0x103fac70`) | 3.46 ms | 1.12 ms |
| Contract expiry band (`0x10380710`) | 2.36 ms | 0.56 ms |

Use interest passes 1 and 3 to test changes to these dependencies. The two
captures cover the observed successful branch; branch and ABI controls need
their own comparisons. Private measurements are under
`.local/performance/2026-11-04-shortlist-gates/`.

### Date initialization and contract window frame

The start-date window takes the reference-after-start return in all 29,835
and 7,088 gate calls in passes 1 and 3. The price query returns zero in both
captures. Pass 1 reaches the age limit and pass 3 reaches the later
eligibility rejection. The transfer eligibility query and expiry-band
calculation still run before the gate completes.

The date constructor now runs directly from the transfer price and
eligibility callers and from the semantic contract window. All three paths
reuse the Date owner, including its store order and counters. Original mode,
dispatcher stops and active diagnostics retain the dispatch path. The
window also copies its five saved GPRs when the stack range is mapped and
unobserved. The scalar access order remains for observers and partial faults.

| Integrated Release overlay | Median paired wall gain | Process CPU gain | Wall wins |
| --- | ---: | ---: | ---: |
| Interest pass 1 | 2.61% | 2.96% | 9/11 |
| Interest pass 3 | 1.97% | 2.08% | 8/11 |

The first combined prototype measured 1.84% and 2.76% wall gains over
eleven pairs each. The isolated constructor routing gained 1.32% on pass 1
and stayed within variation on pass 3. The window frame alone stayed within
variation on both. Discard forcing the window inline and copying only the
generated price and window frames.

Both integrated replays match the baseline snapshots byte for byte, retain
instruction and native import counts, and pass with the dispatch probe on.
The normal ARM64 Release build reproduces both exact final snapshots and
passes six focused CTests, including the Date constructor dispatch control.
The build passes strict signature verification. The sandbox installer stopped
at the process check. The installed ARM64 executable subsequently matched the
build UUID `23A435FD-7354-3C80-BD13-AB9852DFD95A` and passed strict signature
verification. Private measurements are under
`.local/performance/2026-11-04-shortlist-gate-frames/`.

### Expiry-band stack access experiment

Promoting the expiry-band routine's registers slowed interest passes 1 and 3
by 2.31% and 3.75% over eleven pairs each. Calling the generated page directly
gave wall gains of 0.92% and 3.52%, within paired variation. Discard both.

A private generated overlay checks the whole stack scratch span once on
entry. It then uses the same big-endian loads and stores for 80 audited
instruction sites, with scalar access retained when the span cannot be
borrowed. This candidate gains 2.57% and 2.66% wall time (2.58% and 2.71%
CPU) in nine alternating pairs per interest pass, with nine and eight
wall wins. Both full final snapshots match byte for byte, with equal
instruction and import counts.

The normal ARM64 Release build gives a 2.74% wall and 2.63% CPU gain on
interest pass 1, with eight and nine wins in nine pairs. Pass 3 and daily
transition stay within variation. Both interest passes reproduce the
baseline snapshots byte for byte. Five focused CTests pass, including 50
production expiry comparisons for branches, aliasing, unaligned stack,
partial faults, interior entry and observer mutations, plus the debug-lock
guard. The signed build UUID is `7EE1D012-2379-3E26-BA6F-D16403076887`.
The sandbox installer stops at the process check. Private measurements are
under `.local/performance/2026-11-04-contract-expiry/`.

### Module candidates for further reconstruction

The original source-name pool identifies transfer price (`0x103fac70`) and
eligibility (`0x103faff0`) as TRANSFER_MANAGER.CPP. Both still execute their
generated bodies in production. Together they account for about 5.98 ms in
interest pass 1 and 1.74 ms in pass 3 in the instrumented profile above.
These inclusive costs predate the Date dispatch optimization and bound the
observed work rather than predicting a gain. Use the captured price and
eligibility calls to measure a proposed handwritten route.

STAFF_CONTRACTS.CPP already has separate C++ owners for contract terms and
review. The expiry band still runs the generated route in production, while
the start-date window has a C++ owner. The stack experiment above improves
the measured expiry path without requiring a whole-module replacement.
