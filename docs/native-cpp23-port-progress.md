# Complete C++23 port: progress and acceptance boundaries

The active objective is to finish the modern C++23 port and the game's native
runtimes. A completed subsystem advances this objective; it does not close it.
The production ARM64 application already runs the statically recompiled Mac
engine. Most of that engine still consists of generated routines rather than
named semantic C++23 operations. The native application and a completely
reconstructed engine are separate claims.

## Current evidence

The [ball-action coordinator](native-ball-action-reconstruction.md) remains a
component-only candidate. Its contiguous reconstructed prefix now covers
**7,073 of 8,142 original instructions**, including all five live computed
tables and the failed-action preparation/messages/fallback. Independent
comparisons cover all prefix source sites in 382,287 cases, with complete
state, ordered effects, dependency ABI, faults and instruction
accounting. The production coordinator remains generated and the accepted
manifest stays at **677**. The unsuccessful-action tail and whole-root production
acceptance remain required.

The latest [team-line reconstruction](native-team-line-reconstruction.md) adds
five complete operations: tactical flags, their line-adjustment query, two
column-limit queries and the opposing row-lane scan. The accepted manifest now
contains **677 semantic entry replacements**, preserving its previous 672-entry
prefix. All seventeen direct callees of the movement selector now have semantic
owners; transitive dependencies and the complete engine remain unfinished.
Independent comparisons cover all 172 original instructions: 10,358 component
cases, 60 production comparisons, 15 profile controls and 42 nonlocal comparisons
pass. Six focused CTests, seven boundary audits, 45 updated manifest checks and
three affected-unit UBSAN checks pass. A 600-tick actual match compares all five
roots selected as original versus C++23, preserving complete final state,
per-tick accounting and import ABI. No speed optimization or timing benchmark
is part of this cycle.

The preceding [movement-choice reconstruction](native-movement-choice-reconstruction.md)
adds four complete operations: neighbouring-cell selection, its adjustment query
and two pitch-region predicates, reaching the 672-entry milestone.
Independent comparisons cover all 749 original instructions; 6,518 component
cases, 48 production comparisons, 12 profile controls and 35 nonlocal comparisons
pass. Five focused CTests, seven boundary audits, 44 updated manifest assertions
and three affected-unit UBSAN runs pass. Forty paired 600-tick intervals retain
complete final-state equality. Timing varies between batches; this block claims
accurate reconstruction, with no consistent speedup established.

The preceding [actor processing-order reconstruction](native-actor-order-reconstruction.md)
adds five complete operations: the active queue, its comparison callback and
three comparison values, reaching the 668-entry milestone. Independent
comparisons cover all 202 original instructions; 3,529 component cases, 60
production comparisons, 15 profile controls and 42 nonlocal comparisons pass.
Five focused CTests, seven boundary audits, 43 updated manifest assertions and
three affected-unit UBSAN runs pass. Twenty paired 600-tick intervals retain
complete final-state equality and show median paired time reductions of **2.57%
and 2.12%**, with the candidate faster in all twenty pairs. These are measured
simulation intervals, not a whole-game speed claim.

The preceding [pitch distance reconstruction](native-pitch-distance-reconstruction.md)
added three complete queries, reaching the 663-entry milestone. Its acceptance
covers 129 original instructions and seven focused CTests. Its twenty paired
600-tick intervals showed small time increases of approximately 0.98% and 0.69%;
that earlier block established no speedup.

The preceding [team spatial reconstruction](native-team-spatial-reconstruction.md)
added nearest actor, minimum distance, nearby weight and both teams' column
limits. Its 48-case production regression passed at the preceding distance
delivery. Its acceptance covers all 1,624 original instructions;
the measured 600-tick intervals established no consistent additional speedup.

The preceding [dispatch runtime optimization](native-dispatch-runtime-performance.md)
removes a repeated environment lookup identified by native CPU sampling.
Twenty paired 600-tick comparisons show approximately **31.6% less simulation
time** for the two measured matchups, with complete final-state equality and
unchanged instruction/import accounting. Eight focused CTests and the installed
debugger protocol/full-time checks pass. This runtime improvement adds no
semantic replacement entries.

The [match range sampler report](native-match-random-reconstruction.md)
records the preceding 656-entry milestone: one complete 110-instruction
operation with 415 indexed call sites, preserving its 655-entry prefix.
A fresh 200-tick match recorded 33,962
calls to this operation. Independent comparisons cover every original source
site, complete state/effects, recursion, faults and nonlocal transfers. Five
focused CTests, seven boundary audits, 40 updated manifest assertions and three
affected-unit UBSAN runs pass.

The game and independent debugger share the accepted implementation, including
team-line queries, movement choice, the actor queue/comparator, distance queries and dispatch
optimization. Both signed ARM64 applications are installed; the
installed worker protocol, real match completion and game menu/normal exit pass.
The game executable SHA-256 is
`7bbd743a555c1a506aac15075248e38f04ded111ad2fe66535f275e3a8fbc40b`.
The [complete movement](native-player-movement-reconstruction.md),
[foul/referee](native-match-discipline-reconstruction.md) and
[debugger lab](native-match-debugger-lab.md) reports retain their own evidence.
Save/Load is **not retested** in this cycle. The preceding sampler-only delivery's
eighteen paired intervals showed original/C++23 state equality but **no consistent
speedup**; its measurements are separate from the runtime gain above.
The semantic entry count is not a completion percentage or a CPU-share measure.

## Historical register-context acceptance

The [register-context report](native-register-context-reconstruction.md) adds all
thirty floating suffix/nonlocal context entries over four shared bodies / 91
unique original instructions, preserving the first 234 replacements exactly.
Generation retains 1,953,334 instructions / 1,922 pages with zero missing/pending
targets. Full Release ARM64 build and checks use the expanded State consistently.

Acceptance includes 2,883 component comparisons, 756 mapping-fault prefixes and
thirty individual negatives; production adds 1,169 comparisons, 598 exact budget
stops, sixty mapping faults, thirty negatives and direct/generated/CFM evidence
for every entry. Eighty nested transfers check abandoned and surviving C++ calls,
recursive/shared frames, copied contexts, peer State and owner expiry. Ten focused
CTests, fifteen affected Python suites / 101 tests with zero skips and fresh
eleven-unit UBSAN pass. All six sanitizer outputs match Release.

At that delivery, the protected installed ARM64 executable SHA-256 was
`05f50e3350ac9244a0796093ddf186e7171f3c5a7e34c954e4d7741e162a42f1`.
Installed menu/chooser/cancel/normal exit records 337 modern captures and one
modern restore/transfer, with zero faults, original selections, escapes or unknown
transfers. The 28 floating suffix UI counters are zero; the harness establishes
individual coverage for them. Save/Load is **not retested** for this block.

The prior arithmetic, exception, allocator, stream, file/path/controller,
fragment, lifecycle and object-storage acceptances remain in their subsystem
reports. Historical manifest totals describe those deliveries. The semantic entry
count is not a percentage of behavior or functions, and the 5,833 analyzed
functions are not a completeness certificate. Whole-port completion remains open.

## Register-context ownership contract

The entire architectural and control-flow block is accepted in production.
Raw guest storage and scoped host routing remain separate. A nonlocal restore
abandons interrupted semantic callers; it does not return through a stale
C++ epilogue. The shared exception continuation regressions also pass. Unknown
or manufactured contexts use a separately counted fallback, while all live
captured-owner transfer acceptance and installed restoration use no fallback.

## Work that still prevents completion

| Area | Current authoritative boundary | Evidence required for completion |
| --- | --- | --- |
| C/C++ runtime | Allocator and text/number blocks are semantic; many stream, file, sorting, lifecycle and numeric routines remain generated | Named implementations, pinned Mac contracts, ABI/effect/fault comparisons, discriminating negatives and production dispatch evidence for each migrated entry |
| Simulation engine | Watched/background callers and shared match advancement are semantic; dependent football routines, management, leagues, competition, database and transfers largely remain statically recompiled | Complete coherent semantic subsystems and original-compatible composition; component checks alone do not establish game behavior |
| Standalone match debugger | Independent ARM64 app, actual-club factory, shared-engine progression and goal/lineup/substitution/card controls are delivered; full commentary and remaining engine dependencies are unfinished | Complete remaining original-compatible commentary/engine coverage and retain isolated state, timeline and intervention acceptance |
| Native platform | `native/platform.cpp` explicitly returns Open Transport unsupported initialization; print-job and less-used service coverage is incomplete | Real native services with original Mac ABI/error/lifetime behavior and exercising their actual product flows |
| Whole-game compatibility | Historical live evidence covers core single-player actions and a post-match save; it is not current whole-release coverage | Current installed-artifact identity plus appropriate management, simulation, competition, persistence and supported multiplayer evidence after affected subsystems migrate |
| Generated engine retirement | Generated bodies remain the productive implementation of unmigrated routines and the diagnostic oracle | All required productive engine paths have semantic owners before generated bodies can be removed from production |

This is an evidence ledger, not a reduced definition of completion. Unlisted
services remain subject to the full objective. Native import string presence,
an entry manifest, a source inventory or a green focused suite cannot prove that
the whole game is complete. Newly exposed dependencies are added to the work
as their actual callers and contracts are characterized.

## Execution discipline

The current priority is accurate conversion of the entire game to C++23.
Choose complete remaining behavior and its dependencies by conversion value;
runtime speed is not the selection or acceptance target. Defer optimization
until after conversion. Retain earlier measurements as historical evidence.

Work proceeds through coherent subsystems with shared final acceptance, while
retaining individual ABI, ordered guest effects, negative controls and normal
execution witnesses. Preserve the original Date leap asymmetry and the productive
allocator progress guard. Preserve user saves, preferences, original payloads,
private handoffs and earlier reference artifacts. Implementation artifacts and
technical reports are in English; discussion remains in Portuguese.

Use focused checks during migration and broaden validation when a changed
dependency or an actual divergence warrants it. Earlier bounded Save/Load waivers
remain `not retested`; they are not passes and do not prevent the broader objective
from requiring persistence evidence when the migrated subsystem affects it.
Install only technically accepted artifacts, with a protected reference and
verified internal executable identity. Commit and push reviewed product changes,
excluding original listings, generated sources, payloads, runtime artifacts and
private prompts. Keep the overall goal active until current evidence proves
the complete requested end state.
