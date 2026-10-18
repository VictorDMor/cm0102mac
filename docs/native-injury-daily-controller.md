# Daily injury controller

## Scope and status

The complete original operation at `0x10185410..0x101864a0` has a named C++23
implementation. It orchestrates daily injury maintenance, candidate selection,
physical attribute effects, injury registration and existing progression/club
services. It is distinct from the previously accepted daily progression helper
at `0x1018b2b0`.

The accepted manifest and installed game contain 735 unique entries and preserve
the exact prior 734-entry prefix. Runtime, focused sanitizer, installation and
visual startup acceptance are complete. The 12 dependencies separately prepared in the
previous fitness/maintenance exercise were already accepted under
`actor_attributes_adapter`; their redundant preparation code has been removed.
No existing entry changes owner. Generation now rejects duplicate ownership
before page emission.

## Preserved original behavior

The controller discards its pending list and decrements both original injury-day
counters. It retains the original day-180 refresh predicate, weekday-code 5/6
sampling exclusion, signed sample-size arithmetic and smaller severity-selected
fraction. It draws from the shared pool, preserving cursor advancement, reseeds,
wrapped arithmetic, floating conversions and recursive sampling dependencies.

Candidate checks preserve profile/type/availability rules, existing injury
exclusion, physical trait and history gates, repeated-injury selection, the rare
category-zero retry, and classification arguments. The 49-entry computed dispatch
table preserves all original physical attribute paths and unknown-destination
continuations. Null-profile and underage retry loops retain the original budget
behavior rather than gaining invented termination rules.

Every original memory access, volatile/nonvolatile register residue, temporary,
flag, floating status, stack effect, dependency argument, ordered write and
instruction count remains observable. Floating operations use
`-ffp-contract=off`. Existing selection, registration, physical-state refresh,
progression and club-event implementations are reused.

## Source and component evidence

The source contains 1,060 indexed instructions across two pages, with no missing
body sites or external entrances into its interior. Full instruction/flow
fingerprints, the category triples, floating constants and computed dispatch
table are pinned. Independent original references retain every original label
and disable all semantic hooks.

The focused Release comparison passes **178 cases**: 87 complete agreements,
89 matching interrupted/faulted outcomes and two rejected negatives. It reaches
all **1,060 controller sites** plus 340 sites in accepted dependencies. Eleven
comparisons use explicit structural controls, including wide draw paths; these
are component evidence, not natural gameplay observations. Budget cuts, sampled
missing bytes and dependency interruptions remain proportional to the selected
boundary fixtures.

Seven source/scope/hook tests pass. They reject altered instructions/metadata,
omitted sites, external interior edges, changed tables/pointers, missing hooks,
and duplicate ownership. Fifty-six focused historical manifest checks pass;
these check existing prefixes and owners without running historical campaigns.

Full generation retains 1,953,334 indexed instructions across 1,922 pages and
1,923 C++ files, with no missing or pending targets. Only `page_10185.cpp` changes;
the 734-entry accepted prefix is byte-for-byte identical in canonical manifest
form. The body continuation on `page_10186.cpp` is unchanged.

```sh
python3 -m unittest discover -s tools -p test_injury_daily_audit.py -v
cmake --build BUILD --target cm-injury-daily-checks cm-injury-daily-production-checks cm-native-prototype -j 3
ctest --test-dir BUILD -R '^injury_daily_component_equivalence$' --output-on-failure
```

## Actual runtime and sanitizer evidence

Prepared actual-world comparisons retain all 153,796 staff records and 1,200
injury slots from the original database-backed world. The isolated match factory
omits the management availability service; preparation reproduces its original
16-byte allocation, complete constructor at `0x100031b0`, and publication sequence
from `0x1034530c..0x1034532c`. That preparation executes 4,984 original instructions.
No dependency is replaced with a stub in these runtime checks.

Release and UBSAN each pass 11 paired comparisons: eight returning outcomes
(including one rejected negative) and three measured budget interruptions, plus
one separate matching unmapped-pointer fault. They exercise direct, generated,
CFM and both original caller instructions (`0x10119250`, `0x10119810`), as well as
global/per-entry original controls. Dedicated modern-call, return, fault and
budget counters are checked, with no own-body residue or dispatch escape. Full
state, ordered effects, import ABI, private output files and instruction counts
agree. Two further generated/CFM profile comparisons select the original body and
leave the dedicated modern counter at zero.

Prepared runtime reaches 520 of the 1,060 controller sites. This is actual-data
prepared coverage, not traversal of the complete outer management parents or
all natural career scenarios. Component comparisons provide full body coverage.
There is no repeated full-match campaign. UBSAN instruments the new semantic
units, adapter, diagnostics and focused check harnesses; generated pages and
unchanged dependencies remain consistent Release builds. This is not a
whole-engine sanitizer claim.

## Installed delivery

The arm64 application at `~/Applications/CM 01-02 Native Prototype.app` matches
the accepted candidate executable SHA256
`cb7b9f09347d10745112cd45bb87e7b5051ed5c1f9a1b767d088a30c84223f3a`
and passes strict code-signature verification. The installed launch visibly
reaches **Preparar Jogo**, shows the original quit confirmation and exits normally.
This startup smoke does not claim natural UI traversal of the daily controller.
Its production behavior is supported separately by the actual-world checks.

Installation preserves 846 protected runtime files, including the retained
careers, all 18 pre-existing feature prompts, and the installed Match Debugger
executable. The preceding accepted application remains a private reference.
Original inputs, snapshots, recipes, source/installation identities and detailed
receipts remain in `.local/injury-daily`; no original game code or assets are
published.

Save/Load: **not retested**. Performance: **not benchmarked**. This bounded
controller delivery is not a complete-game reconstruction claim; the full goal
remains active.
