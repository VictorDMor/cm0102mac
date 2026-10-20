# Complete C++23 port: progress and acceptance boundaries

The active objective is to finish the modern C++23 port and the game's native
runtimes. A completed subsystem advances this objective; it does not close it.
The production ARM64 application already runs the statically recompiled Mac
engine. Most of that engine still consists of generated routines rather than
named semantic C++23 operations. The native application and a completely
reconstructed engine are separate claims.

The match simulation already executes as statically recompiled C++23, including
complete-match validation recorded in the [ball-action report](native-ball-action-reconstruction.md).
Its semantic reconstruction is still incomplete. The match loop's nearest-candidate
predicate `0x101e31a0` and a coherent target/control query block now have semantic
replacements, as do the shared match/general samplers and the event construction/
transition/history family, actor strategy flag queries and periodic balance/counter
maintenance, target-facing eligibility and corridor accumulation, and complete actor action routing/
primary and secondary selection. Other simulation helpers
remain generated. Running a complete match does not establish that every simulation
routine has a semantic replacement.

## Current evidence

The latest accepted installed application has **963 unique semantic owners**.
The [actor decision policy block](native-actor-policy-reconstruction.md)
is verified and installed with **963 owners**, preserving the exact
958-owner prefix. Five complete operations cover 640 original instructions and
pass 2,740 component comparisons, with 469 ordinary sites and 171 additional
diagnostic-only sites. Actual-context70/profile15 checks and 600 paired advances
pass, with 31,680 productive own calls and 421 complete natural sites. Three
focused CTests pass 21,241 comparisons, and twelve-unit focused UBSAN passes.
**Installed visual acceptance passed on 2026-10-20**: startup, Game Options,
cancellation to the menu and normal quit were observed after the user unlocked
the Mac. The process exited and the installed identity remained valid. Save/Load
is not retested, performance not benchmarked, and the full objective remains
unfinished. Cycle storage cleanup is retained.

The [actor action routing and selection block](native-actor-action-reconstruction.md)
installs **958 semantic owners** after the exact 953-owner prefix. Five complete
operations contain 12,243 original instructions, including the whole primary
selector. The permanent component check passes 13,729 comparisons and covers all
own sites: 6,554 ordinary prepared sites and 5,689 additional diagnostic-only
sites. The actual matrix passes 70 cases and fifteen profile-original controls.
A paired sequence of 600 consecutive actual advances preserves complete final
state, each-advance architectural/import/accounting state and ordered memory
SHA-256, with 23,499 productive own calls and 7,624 natural own sites. Three
focused CTests pass 30,978 comparisons; thirteen-unit focused UBSAN, source/manifest
guards and installed opening/options/cancel/normal exit checks pass. The prior
953 owners, reference app, debugger, 846 protected files and eighteen private
prompts are preserved. Save/Load is **not retested**; performance **not benchmarked**;
PDF updates remain paused. Whole-engine semantic reconstruction and complete
match-event coverage remain unfinished; the full objective stays active.

The [target-facing, eligibility and corridor block](native-target-acceptance-reconstruction.md)
installs **953 semantic owners** after the exact 949-owner prefix. Four complete
operations contain 758 original instructions and pass 12,477 component comparisons:
701 ordinary prepared sites plus 57 checkpoint-only sites in four complete diagnostic
cases. The actual matrix passes 87 comparisons and twelve profile-original controls,
covering 655 complete own sites. All roots have natural indexed BL callers and
44,968 productive invocations in 600 advances. Complete native parent/child and
actual coordinate/area/side/distance/reseed composition, plus two generated response
returns, are observed; no actual generic sampler calls are claimed. Thirty prepared
actual alternatives remain separate from natural provenance. Three focused CTests
pass 29,261 comparisons; source/manifest guards, 29-unit composed UBSAN and installed
opening/options/cancel/normal exit checks pass. Prior owners and helper bodies,
846 protected files, 18 private prompts, prior 949-owner app and installed debugger are preserved.
Save/Load is **not retested**; performance **not benchmarked**; PDF updates remain
paused at edition 39. Whole-engine semantic reconstruction and complete event coverage
remain unfinished; the full objective stays active.
Installed ARM64 SHA-256: `449cfd4d7ba12489f23bfe9373a568bfcc5a3bdbe7ad4097ae799452d69dae7d`.

The preceding installed milestone had **949 unique semantic owners**. The
[periodic actor balance/counter block](native-actor-effects-reconstruction.md)
preserves the exact 946-owner prefix and adds three complete operations containing
1,569 original instructions. Its 18,373 component comparisons cover 1,364 ordinary
prepared sites and 205 further checkpoint-only sites, with nineteen actually mutated
checkpoint completions. The actual matrix passes 66 comparisons and nine original-
profile controls, with 488 complete own sites and returning score/role/reseed
composition. All roots have natural indexed BL callers, with 13,134, 37 and 863
productive calls in 600 advances. Twenty-five prepared actual alternatives remain
separate from natural evidence. No actual generic-sampler caller composition is
claimed. Three focused CTests pass 23,861 comparisons; nine source guards, 92 manifest
methods and composed UBSAN pass. Only two generated root pages change. Installed
ARM64 SHA-256: `ee7d193ea79d148aa0aafa09b076f4f03ef6a386ba65e87fa925c5f424acc8e8`.
Opening, original Portuguese options/cancel and normal exit are verified. All 846
protected files, eighteen private prompts, prior 946-owner app and installed debugger
are preserved. Save/Load is **not retested**; performance **not benchmarked**; PDF
updates remain paused at edition 39. The full reconstruction goal remains active.

The preceding installed milestone had **946 unique semantic owners**. The
[actor strategy flag query block](native-actor-flags-reconstruction.md) preserves
the exact 939-owner prefix and adds seven complete operations containing 185
original instructions. The existing shared reader and mode policies retain their
ownership. Its 16,368 component comparisons and 108 actual-context comparisons
cover all 185 own sites without source-checkpoint mutation. All seven roots have
indexed original BL callers, with 37,210 productive calls in 600 advances. Ten
prepared actual-context alternatives remain separate from natural caller evidence.
Twenty-one profile-original controls and 30 complete actual shared-reader returns
pass. Two focused CTests pass 20,510 comparisons; nine source guards, 91 manifest
methods and composed UBSAN pass. Only one generated page changes. Installed ARM64
SHA-256: `aea9750591a4d9bddecabcc39840165cc64bd23a55b51a5d60c2828467838939`.
Opening, original Portuguese options/cancel and normal exit are verified. All 846
protected files, eighteen private prompts, prior 939-owner app and installed debugger
are preserved. Save/Load is **not retested**; performance **not benchmarked**; PDF
updates remain paused at edition 39. The full reconstruction goal remains active.

The preceding installed milestone had **939 unique semantic owners**. The
[match event construction, transition and history block](native-match-event-reconstruction.md)
preserves the exact 930-owner prefix and adds nine complete operations containing
2,917 original instructions. Its 20,786 component comparisons cover 2,628 ordinary
sites and 289 further checkpoint-only sites, with 22 explicitly mutated-checkpoint
completions. Actual-context validation passes 131 comparisons and 27 original-profile
controls, with 890 own sites and 22 completed owned children. Seven entries occur
naturally in 600 advances; address and jitter have prepared original-BL coverage.
A complete prepared emitter composes side/qualifier/jitter/transition/classification
and real reseeding; two complete transition inputs compose the participant helpers.
This does not prove all match events. Three focused CTests pass 26,274 comparisons;
ten source guards, ninety manifest methods and composed UBSAN pass. Four generated
pages change. Installed ARM64 SHA-256:
`5109e2148643fe75106039d1c03f0ba58dfcb570565e9dadb8bcfad57dd09f18`.
Opening, original Portuguese options/cancel and normal exit are verified. All 846
protected files, eighteen private prompts, prior 930-owner app and installed debugger
are preserved. Save/Load is **not retested**; performance **not benchmarked**; PDF
updates remain paused at edition 39. The full reconstruction goal remains active.

The preceding installed milestone had **930 unique semantic owners**. The
[actor encounter response and score block](native-actor-encounter-reconstruction.md)
retains the exact 926-owner prefix and adds four complete operations containing
1,809 original instructions. Its component passes 716 comparisons, including six
explicit branch-condition fixture completions: 1,669 ordinary sites and 140 further
checkpoint-only sites. Seventy-one actual-service comparisons and twelve original-profile
controls pass, with 1,210 actual sites and complete prepared peer/team/score/response
composition. The actual reseeder and shared sampler also execute. Three roots have
13,280, 141 and 45 natural calls in 600 advances; team scaling is captured through a
prepared complete peer and original BL. This does not establish complete match-event
coverage. Three focused CTests pass 7,124 comparisons; ten source guards, 89 manifest
methods and composed component/runtime UBSAN pass. Three generated pages change.
Installed ARM64 SHA-256: `2690424fd32bbf313b41aea906841e64eb46798c2c43936bfd851dd4b655b49f`.
Opening, original Portuguese options/cancel and normal exit are verified. All 846
protected files, eighteen private prompts, prior 926-owner app and installed debugger
are preserved. Save/Load is **not retested**; performance **not benchmarked**; PDF
updates remain paused at edition 39. The full reconstruction goal remains active.

The preceding installed milestone had **926 unique semantic owners**. The
[selection context/mode and score block](native-match-selection-support-reconstruction.md)
retains the exact 923-owner prefix and adds three complete operations containing
400 original instructions. Its component passes 1,636 comparisons and all 400 own
sites without source mutations. Forty-eight actual-service comparisons and nine
original-profile controls pass, with 373 actual sites, 55 returning owned score
children and 839 selected external-helper returns. The score helper has 222 natural
calls in 600 advances; two selectors have prepared original-BL coverage. Both leaf
formulas and the mode policy's two offsets have actual evidence; complete event
parents remain unproven. Three focused CTests pass 6,061 comparisons; ten source guards, 88 manifest
methods and composed component/runtime UBSAN pass. Two generated pages change.
Installed ARM64 SHA-256: `ae4fc14c55252c23eaee57116500a532a4bb179823e12e483d03b6d8dae92ce7`.
Opening, original Portuguese options/cancel and normal exit are verified. All 846
protected files, eighteen private prompts, prior 923-owner app and installed debugger
are preserved. Save/Load is **not retested**; performance **not benchmarked**; PDF
updates remain paused at edition 39. The full reconstruction goal remains active.

The preceding installed milestone had **923 unique semantic owners**. The
[team-selection and preset-target block](native-match-selection-reconstruction.md)
preserves the exact 919-owner prefix and adds four complete operations containing
815 original instructions. Its component passes 2,369 comparisons and all 815 own
sites without source mutations. Fifty-nine actual-service comparisons and twelve
original-profile controls pass, with 603 complete actual sites and 1,018 selected
helper returns. The preset lookup has 519 natural calls in 600 advances; three
selectors have explicit original-BL coverage with real match input. This does not
claim their complete event parents. Three focused CTests pass 8,567 comparisons;
ten source guards, 87 manifest methods and composed component/runtime UBSAN pass.
Only two generated pages change. Installed ARM64 SHA-256:
`32d24b72e12394df9c5aaead383b4f1f3e581faa27bb50bac29fa03975cabf5d`.
Opening, original Portuguese options/cancel and normal exit are verified. All 846
protected files, eighteen private prompts, prior 919-owner app and installed debugger
are preserved. Save/Load is **not retested**; performance **not benchmarked**; PDF
updates remain paused at edition 39. The full reconstruction goal remains active.

The preceding installed milestone had **919 unique semantic owners**. The
[match target/control reconstruction](native-match-target-reconstruction.md)
preserves the exact 911-owner prefix and adds eight complete operations containing
785 original instructions. The component passes 4,142 original comparisons and
covers all 785 own sites, including four explicit source-mutation cases. The
actual-service matrix passes 114 comparisons and 24 original-profile controls,
with 385 sites in complete productive comparisons. Six roots occur in twenty
natural match ticks; two strategy queries are captured through prepared complete
parent calls. These are separate evidence boundaries. Two focused CTests pass
11,598 comparisons; ten source guards, 86 historical manifest methods and composed
component/runtime UBSAN pass. Five generated pages change. Installed ARM64 SHA-256:
`36b1ab0167a0a6cd4ecaa6f35883f7197a44eaa41fc6ef13c7a66e4375e9ee18`.
Prepare Game, original Portuguese options/cancel and normal exit are verified.
All 846 protected files, eighteen private prompts, prior 911-owner app and installed
debugger are preserved. Save/Load is **not retested**; performance **not benchmarked**;
PDF updates remain paused at edition 39. Full match-engine and whole-game semantic
reconstruction remain unfinished; the full goal stays active.

The preceding installed milestone had **911 unique semantic owners**. The
[shared sampler reconstruction](native-range-sampling-reconstruction.md) retains
the exact 909-owner prefix and adds the sampler used by match actors and ball
actions plus its related general helper: 1,103 original instructions. The block passes 4,772 original comparisons;
42 actual-service and six original-profile controls pass, with 289 complete actual
sites. A natural match tick invokes the match sampler; the general helper has
explicit real-context coverage. Two focused CTests, ten source/hook guards, 85
manifest methods and composed component/runtime UBSAN pass. Only two generated
root pages change. Installed ARM64 SHA-256: `955e121c728d0a52b8884b651c36ac2cab3e1ce99a784c25641167fcd32712d4`.
Original menu/options/cancel and normal exit are verified. All 846 protected files,
eighteen private prompts, prior 909-owner app and installed debugger are preserved.
Save/Load is **not retested**; performance **not benchmarked**; PDF updates remain
paused at edition 39.
The complete match engine and whole-game semantic reconstruction remain unfinished.

The preceding installed milestone had **909 unique semantic owners**. The
[catalog lifecycle integration](native-localization-catalog-integration.md) preserves
the exact 905-owner prefix and adds four complete 734-instruction operations.
Raw bodies, all three localization references, shared helper and platform behavior
remain unchanged. Fifty-four actual-service comparisons, 21 original-profile controls,
three component CTests, thirty source/data/hook guards and 84 manifest methods pass.
The actual matrix observes 521 own sites and returning constructor/destructor/sort
children; the three components retain 1,754 comparisons and all 1,905 own sites.
Composed component/runtime UBSAN matches Release with eight instrumented units and
21 pinned headers. Full generation changes one page and retains no missing/pending
targets. Installed ARM64 SHA-256: `76ddfe835a7d599586c2176c3d378d223479c3fb5dcf6a55ca598ebb11378224`.
Prepare Game, original Portuguese options/cancel and normal exit are verified.
All 846 protected files, eighteen private prompts, the prior 905-owner app and the
independently pinned installed debugger are preserved. Save/Load is **not retested**;
performance **not benchmarked**; PDF updates remain paused at edition 39.
The full goal stays active.

The preceding installed milestone had **905 unique semantic owners**. The
[localization integration](native-localization-integration.md) preserves the exact
901-owner prefix and adds four whole operations: record initialization,
current-language reading, cached/file lookup and complete phrase formatting.
Together they contain **1,171 original instructions**. Accepted raw bodies,
independent references and the common semantic-call helper remain byte-identical;
native-platform behavior is unchanged.

The actual-service matrix passes **49 paired comparisons**: 34 completions
including five deliberate-corruption controls, eleven matching budget stops and
four required-pointer faults. It observes **721 own source sites** in complete
productive comparisons, with at least three productive returns per root. A separate
17-return profiling-original matrix passes. Productive completions finish three
lookup, five formatted-output and one substitution child, excluding preparation.
The original initializer loads the real 9,807-row, ten-language catalog before
observation; cache and file paths use actual strings, allocation and native forks.
This is not actual-service coverage of every original branch or language.

The two components retain **1,137 comparisons** and all **1,171 own source sites**
in complete starting-root comparisons. Twenty source/data/hook guards and 82
focused historical manifest methods pass. Both independent original reference
outputs remain byte-identical. Three focused UBSAN runs match Release stdout with
empty stderr; twelve units are instrumented and eighteen headers pinned. This
is not whole-game sanitizer coverage.

Full generation retains 1,953,334 indexed instructions and 1,922 pages with no
missing/pending targets. Only two generated pages change. The installed
strict-signed ARM64 executable has SHA-256
`8cf3dcb465b2e6cbf6508688da1fa9813423aa3376b1770e3a8cfed76929c553`.
Prepare Game and the original Portuguese options screen render, Back or Cancel
returns to Prepare Game, and original confirmation exits normally. All **846
protected files**, eighteen unrelated prompts and the prior 901-owner application
are preserved. The installed Match Debugger is outside this installation scope.

The [lookup report](native-localization-lookup-reconstruction.md) and
[formatter report](native-localization-format-reconstruction.md) retain their
component contracts and model boundaries. Catalog lifecycle is subsequently integrated above; sorting and the
large substitution child retain separate ownership. PDF updates remain paused;
the last accepted guide is edition 39 and predates this milestone. Save/Load is
**not retested**; performance is **not benchmarked**. The whole-game goal remains active.

The preceding [preference-session and screen integration](native-preferences-ui-integration.md)
installed 901 owners after the exact 897-owner prefix, with four complete operations,
2,933 source instructions, 60 actual-service comparisons and three focused UBSAN
runs. Its report retains the detailed installed and preservation acceptance.

The preceding [currency configuration and preference lifecycle integration](native-preferences-integration.md)
installed 897 owners after the exact 884-owner prefix. Its thirteen operations
retain 241 actual-service comparisons, 138 original-profile controls, 4,441
component comparisons and all 2,278 own component sites. Complete initialize/apply
callers, recursive linked application and the actual 303-byte preference file
layout were accepted independently. The preceding
[contract display and money-presentation integration](native-money-presentation-integration.md)
installed 884 owners. Their reports retain their individual acceptance boundaries.

The records below document preceding installed milestones.

The [automatic proposal builder integration](native-transfer-synthesis-integration.md)
adds the complete 3,551-instruction operation after the exact 831-entry prefix,
reaching **832 installed entries**. Three component CTests pass 21,489 comparisons;
the new root retains 7,780 comparisons and complete original-site coverage.
Release and focused UBSAN each pass 56 actual-service comparisons, with 580
source sites, plus the adjacent 182-comparison proposal runtime. Fifty-eight
original profile controls, 37 source/data/hook guards and 63 historical manifest
methods pass. All three focused sanitizer runs have empty stderr; runtime stdout
matches Release. The semantic bodies and common call helper remain unchanged.

The strict-signed arm64 executable has SHA-256
`c56c18120eace83d759ed76036c18d1b916be55f27669366593cf43857bb03c6`.
Prepare Game, original quit confirmation and normal exit pass. All 846 protected
files, the prior 831-entry application, eighteen unrelated prompts and Match
Debugger are preserved. Save/Load is **not retested**; performance is
**not benchmarked**. The cumulative Portuguese guide is updated, and the full
reconstruction goal remains active.

The [combined transfer-offer record and valuation integration](native-transfer-proposal-integration.md)
adds sixteen complete operations and 1,994 original instructions after the exact
accepted 815-entry prefix, reaching **831 installed entries**. Eight component
CTests pass 46,000 comparisons; the new blocks retain 13,709 component
comparisons with every original source site required. Both Release and focused
UBSAN pass 182 actual-service comparisons across all sixteen roots and 1,038
sites. Fifty-four original profile controls, eighty source/data/hook guards and
63 historical manifest methods pass. Six adjacent runtime seams pass on both
builds. All nine focused sanitizer runs have empty stderr; runtime and seam
stdout match Release exactly. The full reconstruction goal remains active.

The strict-signed arm64 executable has SHA-256
`fbe0ac56c93c33bc923947e9711c642e475dc92a35eb32c64be1c35b2fe3e27d`.
Prepare Game rendering, original quit confirmation and normal exit pass.
All 846 protected files, the prior 815-entry application, eighteen unrelated
prompts and installed Match Debugger are preserved. Save/Load is **not retested**;
performance is **not benchmarked**. The cumulative Portuguese PDF is updated.

The [transfer-offer query/policy integration](native-transfer-offer-integration.md)
adds nineteen complete operations and 1,111 original instructions after the exact
accepted 796-entry prefix, reaching **815 installed entries**. Release and focused
UBSAN each pass 10,429 component comparisons with complete source coverage and
248 actual-service comparisons covering all nineteen roots and 941 sites.
Fifty-six source guards, 62 historical manifest methods, 96 profile controls,
six component CTests and five adjacent runtime seams pass. All seven focused
sanitizer runs have empty stderr; runtime/seam output matches Release exactly.

The strict-signed arm64 executable has SHA-256
`5070fffa74fdc72a21a86db85cde4cb606bf25f419ff7927d79d0490dea3878c`.
Prepare Game rendering, original quit confirmation and normal exit pass.
All 846 protected files, the prior application, eighteen unrelated prompts and
the installed Match Debugger are preserved. Save/Load is **not retested**;
performance is **not benchmarked**. The cumulative Portuguese PDF is updated.
The full reconstruction goal remains active.

The preceding [contract review/status integration](native-contract-review-integration.md)
adds thirteen complete operations and 1,447 original instructions after the exact
accepted 783-entry prefix, reaching **796 installed entries**. Release and focused
UBSAN each pass 10,507 component comparisons with every source site required and
519 actual-service comparisons across all thirteen roots. Forty-five contract
guards, 61 historical manifest methods, 104 profile controls, five component
CTests and four adjacent runtime seams pass. All six focused sanitizer runs have
empty stderr; their runtime/seam output matches Release exactly.

The strict-signed arm64 executable has SHA-256
`22320cfc824699fb265d7e27a863b70a7872f3c46e85aa905aef8bd1297ed600`.
Prepare Game rendering, original quit confirmation and normal exit pass.
All 846 protected files, the prior application, eighteen unrelated prompts and
the installed Match Debugger are preserved. Save/Load is **not retested**;
performance is **not benchmarked**. The cumulative Portuguese PDF is updated.
The whole-game goal remains active.

The preceding [contract terms integration](native-contract-terms-integration.md) adds
21 complete operations and 1,203 original instructions, extending the exact
accepted 762-entry prefix to **783 installed entries**. Clause/bonus editing and
eligibility, labels, end descriptions, shared sampling and expiry bands pass
5,242 component comparisons with complete source coverage, and 559 prepared
actual-service comparisons covering all 21 roots. Thirty-three contract guards,
60 historical manifest methods, 112 profile controls and three adjacent seams
pass. The focused 12-unit UBSAN build passes all five runs with empty stderr.

The strict-signed arm64 game has executable SHA-256
`25b596bb1824a4387e0032050cc5cef4d6d2168debec6b8ee67247fb8cabc7cd`.
Prepare Game rendering, original exit confirmation and normal exit pass. All
846 protected files, prior application and installed Match Debugger are
preserved. Save/Load is **not retested**; performance is **not benchmarked**.
The cumulative Portuguese PDF is updated. The whole-game goal remains active.

The preceding [contract lookup and creation batch](native-contract-integration.md) adds
six complete operations and **2,161 original instructions**, extending the
accepted prefix from 756 to **762 unique entries**. Release and focused UBSAN
each pass 6,113 component comparisons covering every site and 120 prepared
actual-world comparisons, including twelve pointer faults and twelve rejected
negatives. Twenty-four profile controls, 21 source/hook guards and 59 historical
manifest methods pass. Exactly four generated root pages change. The existing
AWOL and daily injury runtime seams pass with both contract owners selected
original on their reference side. This is prepared actual-data coverage, not
natural career traversal or completion of all contract services.

The strict-signed arm64 application is installed with executable SHA-256
`40c724fbcdfeaee09067aa1b90c9b6a7de68325fa36cc19dbc2b1c8dce32e246`.
Prepare Game rendering, original exit confirmation and normal exit pass.
All 846 protected files, previous application and installed Match Debugger are
preserved. Save/Load is **not retested**; performance is **not benchmarked**.
The cumulative Portuguese companion guide is updated alongside each cycle.
The whole-game reconstruction goal remains active.

The preceding [AWOL block](native-awol-records.md) adds 21 operations and
6,958 original instructions after the daily injury controller's accepted
735-entry prefix. That generated, built and installed delivery contained
**756 unique entries**. Release and focused UBSAN each pass 1,819 component
comparisons covering every source site, plus 189 prepared actual-world
comparisons and 21 separate pointer faults. Forty-two Release profile controls,
eight source/hook guards and 59 focused historical manifest methods pass.
Actual-world checks retain 153,796 staff and use original management, contract,
list, message, text and private-file services. This prepared coverage is distinct
from natural career-UI traversal; no repeated full-match campaign ran.

At that AWOL milestone, the strict-signed arm64 game had executable SHA-256
`626df9fa62ec13a0ebd5a3c19f96d0b60e996eb40df88872fdb563d8d71b61d2`.
Prepare Game rendering, original quit confirmation and normal exit pass.
All 846 protected files, prior application, eighteen existing feature prompts
and installed Match Debugger are preserved. Save/Load is **not retested** and
performance is **not benchmarked**. The full reconstruction goal remains active.

The preceding [daily injury controller](native-injury-daily-controller.md)
adds one complete 1,060-instruction operation and rejects duplicate semantic
ownership before generation. Its existing actual-world seam also passes after
AWOL integration.

The earlier [injury controller persistence block](native-injury-controller-persistence.md)
adds two complete operations and **945 original instructions**, extending the
accepted manifest from 732 to **734 entries** with its exact prior prefix intact.
Release and affected-unit UBSAN each pass 34,717 component, 85 actual private-file
and 112 nonlocal comparisons, plus two actual pointer faults. All 945 source
sites, fifteen Release profile controls, eighteen new/preceding bootstrap audits,
56 historical manifest methods and four focused CTests pass.

A paired 798-advance match preserves state, imports and per-tick accounting; both
roots have zero calls in that stepper. Prepared actual-data original BLs cover
loading, writing, legacy date conversion, optional arrays and lists. Two primary
inputs and three variants retain original-written private archive files, bounded
actual rows and explicit file-version context. This prepared coverage is distinct
from complete management UI and user career persistence.

The strict-signed ARM64 game is installed with SHA-256
`1aad17166edc1f9229baa02118fc804ae5822971460d41aa2eb801157fb660ac`.
Prepare Game rendering and normal exit pass. Debugger sources and the installed
debugger are preserved. All 846 protected files, including both user career saves,
and eighteen prior feature prompts retain their hashes. Seven linked checks are
cleaned; eighteen task-owned snapshots receive verified transparent compression.
Save/Load is **not retested**; no performance benchmark or gameplay change is
claimed. The full goal stays active.

The latest [spatial-role optimization](native-spatial-role-performance.md)
reduces measured 600-tick match runtime by median **5.14% and 4.14%** in two
scenarios, faster in all 20 measured pairs. It preserves the complete 23-entry
subsystem and the 682-entry manifest. Release/UBSAN comparisons cover 4,090
component cases, 364 production cases and 196 nonlocal cases; 69 profile controls
also pass. A paired complete 798-advance match preserves full state and accounting.
Only the game application was updated; the installed Match Debugger is unchanged.
Save/Load and the installed management UI were not retested in this cycle.

The [ball-contact block](native-ball-contact-reconstruction.md) is integrated and
installed in both signed ARM64 apps. Its six complete roots cover 2,043 original
instructions: four new entries plus two accepted discipline facades reused with
their existing owners. The **682-entry manifest** retains all previous 678 entries.
Component evidence totals 198,811 comparisons per Release/UBSAN mode. Real caller
checks cover 72 production cases, 18 profile controls and 49 nonlocal transfers.
A paired 789-advance match preserves full final state and per-tick accounting;
the description executes 13,063 times. Installed Debugger completion also passes.

Final grouped 600-tick measurements show median paired runtime reductions of
0.58% and 0.21%. These are small and close to measurement variation; no large
speedup is established. The initial slower candidate was revised before installation.
Save/Load was not retested, and the complete port remains unfinished.

The preceding [ball-action coordinator](native-ball-action-reconstruction.md)
delivery covered all 8,142 original instructions and reached the 678-entry
milestone. Its 518,549 Release/UBSAN comparisons and paired complete match agreed,
including 444 C++23 action calls against 444 original controls.

The preceding [team-line reconstruction](native-team-line-reconstruction.md) added
five complete operations: tactical flags, their line-adjustment query, two
column-limit queries and the opposing row-lane scan. That checkpoint brought the manifest to
**677 semantic entry replacements**, preserving its previous 672-entry prefix. All seventeen direct callees of the movement selector now have semantic
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

The accepted source engine is shared by the game and independent debugger.
Earlier installed-debugger acceptance covers team-line queries, movement choice,
the actor queue/comparator, distance queries and dispatch optimization, including
its worker protocol and real match completion. The installed Debugger remains
at its preserved earlier artifact; only the game receives the attribute/condition
deliveries. The current game executable SHA-256 is
`119af966b7bf8f5e71fd5aa45d9b99cd9b3ef32532c1d6c523782352f45d12db`.
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
