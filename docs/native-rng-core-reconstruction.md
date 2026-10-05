# Semantic Mac RNG core reconstruction

Status: delivered in native ARM64 production on October 4, 2026. The three RNG
core entries are semantic C++23, with direct, integrated and real persistence
acceptance below. The Date delivery and its explicitly waived remaining weekly
campaign retain their existing status.

## Scope and verified starting point

The checkout and a fresh origin/main query identified
`daba5f30b6895cc5ecba5d03c40cb337868f5cda` on October 4, 2026. The eight
untracked reconstruction prompts are preserved and excluded from delivery.
The initial personal inventory contains 809 Game / 5 Runtime / 44 Runs files.
Content, resource forks, FinderInfo, all xattrs, modes, mtime and flags were
inventoried. A new private installed reference at
`.local/rng-core-reconstruction/reference/installed.app` matches the installed
bundle's complete inventory. The mutable build bundle is explicitly excluded
from historical preservation; the new installed reference preserves its
production executable before any rebuild. The initial protection pass verifies
1,491 historical file identities after that exclusion, including the Date saves,
original inputs, fixtures, candidates, previous references and pending prompts.

Exactly three entries join the preserved 34 Date replacements:

| Entry | Exclusive end | Instructions | Indexed direct calls | Responsibility |
| --- | --- | --- | --- | --- |
| 0x1077b770 | 0x1077b790 | 8 | 3,728 | Mac CRT generator step |
| 0x1077b790 | 0x1077b798 | 2 | 7 | Mac CRT seed store |
| 0x107474a0 | 0x107475ac | 67 | 4 | Game seed and pool initialization |

Their fingerprints are respectively
`947d4acb79bb75eb056f1720faf3b72a658c702e3390df399e637ed8b3b54760`,
`b5ea8440c2bbee7a391956377ff65b3816799c5b73fe78852d08bb6671eb0cb8`, and
`b189ae6daae4a6fc7170607de6a91f982df7bba630bf562ad1792497003203db`.
The read-only index audit found no indexed external incoming edge into their
interiors. Indirect transitions and callbacks are checked separately below;
static calls are not runtime evidence. Complete proprietary records remain private.

## Original contract and production integration

The state slot is relative to the live r2 TOC, at -0x34c4. The step wraps unsigned
32-bit multiplication by 0x41c64e6d and addition of 0x3039, then returns bits
16 through 30. r0 retains the next full state and r4 retains the incoming state.
Seed writes r3 without changing its value or other architectural registers.
Neither leaf changes flags, stack, LR, CTR or context TOC; both return to LR.
The generated instruction-local temporaries do not modify State.unique.

Initialization uses an 80-byte frame and interleaves the saves of r31/r30/r29/r28
with cursor/base/end slot loads. Zero seed executes original time service
0x1077f900 with r3=0. Seed, original formatting/logging service 0x1077b720 and two
draws retain their direct ABI calls, exact LR and context TOC. No callback
marshalling restores their clobbers away. Cursor installation occurs between the
two draws; a cursor alias of CRT state therefore changes the second draw.
The salt pointer is loaded late, after the cursor reload. End is the inclusive
last unsigned16 big-endian pool sample, base+101998. Signed constant-division
intermediates, carry residue, unsigned cursor bounds, ordered writes, padding,
backchain and saved-register reloads remain observable. Wrapping pool addresses
and aliases make the bounds check significant even though normal draws cannot
select an index above 32767. Index 50999 remains valid for consumers.

Destination hooks cover generated pages 0x10747 and 0x1077b. The sampler
0x107124e0, inline consumers, selectors, scheduler and match engine remain
generated. The Mac core does not use Windows ReCM's different VC6 constants,
32-bit little-endian pool, Windows time services, or independent static C++ state.
Date's ascending-year difference quirk remains unchanged: January 1, 2000 minus
January 1, 2001 is -365; the inverse is +366.

All RNG diagnostic fields are grouped at the end of State. Counts are separate
for each real original/semantic entry. Optional ordered boundary digests and a
maximum of eight app samples per entry provide bounded evidence; they do not
replace full direct comparisons or the existing strict season transcript.
No per-draw register/memory files or new RNG stream format are introduced.
An additional private validation storage observer aggregates accesses by RNG field
and original PC to identify Save/Load accesses. It is enabled only with
`CM_AOT_RNG_STORAGE_AUDIT` in an isolated validation process; production has no
read observer. Its data does not enter State or a game save.

## Contract and suite validation

The generator pins all three bodies and rejects changed or incomplete fragments.
It emits 37 semantic replacements, retaining all 34 Date entries. The focused
harness compares complete register/temporary files, PC, allocator/import state,
all mapped bytes and interleaved read/write/map/clear effects. It includes minimal
leaf mappings, consecutive sequences, unsigned wrap, TOC relocation, pool/slot/
stack aliases, composed selections, generated consumers, indirect transitions,
callbacks and own-boundary negative controls. Seed-zero tests use controlled
GetDateTime effects in the existing private transcript encoding; they must never
read the wall clock. The consistent ARM64 build passes 2,022 full comparisons:
1,820 direct rand,
28 seed, 137 initializer (including seven controlled clocks), 30 generated
consumer cases, three callbacks, three indirect transitions and one compact
evidence comparison. All three own-boundary negatives reject; reset and
nonserialization checks pass. All 40 CTest groups pass in 135.04 seconds.
Python discovery passes 37 tests
with four optional groups skipped; all four optional groups also pass their
explicit index-backed runs. These are component/suite results, separate from the
integrated and UI evidence below.

The final object audit verifies 1,976 State-dependent linked objects with no
stale project dependency. One historical `cm-aot-probe.dir/aot_probe.cpp.o` is
absent from every current link command and is explicitly excluded. The live
probe host object is rebuilt. State header SHA-256 is
`05a3f3cd892f3b18e5839512dc0dc1830994bd532b803bc9b56890dd0f9bc82d`.
The first clean build needed a dependency refresh after the direct clock EOF
check was added; its unresolved symbol was not treated as a passing build.
The subsequent complete build and direct checks passed.

Generated-temporary UBSAN passes 199,742 differential executions across 2,050
actual instruction patterns and 46 operation kinds. Focused RNG UBSAN passes
all 2,022 comparisons, own negatives, callbacks, indirect transitions, controlled
clocks, snapshot exclusion and malformed transcript controls with empty stderr.
Its instrumented units are runtime, RNG adapter/harness, season transcript,
callbacks and generated pages 0x10747, 0x1077b, 0x10712, 0x1077f and 0x10780.
This does not claim whole-program/bridge instrumentation or ASan.

## Integrated immutable day comparison

The new explicit modes retain all 34 semantic Date entries. `--rng-baseline`
selects the original RNG core; `--rng-modern` selects the three semantic bodies.
Default all-original and historical Date comparison modes retain original RNG.
Conflicting Date/RNG selections and candidate oracle recording are rejected.
The existing strict CMDAY001 transcript and snapshots are reused without a new
Date capture. Each run rechecks the executable and all four fixture identities.

| Mode | Equivalent runs | rand own calls/run | seed own calls/run | initializer own calls/run |
| --- | ---: | ---: | ---: | ---: |
| Date semantic / RNG original | 2 | 209,662 original | 4,201 original | 0 |
| Date semantic / RNG semantic | 2 | 209,662 semantic | 4,201 semantic | 0 |

Opposite-mode calls are zero in both modes. Every repeated report within a mode
is identical. The ordered boundary digest is `8c6cf6e031fecd47` in all four runs.
Registers, unique storage, PC, memory, allocator, import/callback order and RNG
consumption retain the existing strict comparisons, including final-state and
physical transcript EOF/checksum. Initial RNG perturbation fails at the first
native input register divergence; it is not accepted as an equivalent run.
Initialization has **zero replay coverage** and is established separately by
direct contracts and actual startup, never inferred from aggregate draws.

Final state: year 2002, ordinal 39, phase 0; salt `0x6fc9`, cursor `0x108eb25e`,
inclusive end `0x108fbfae`, 41,620,536 native calls. Translated instruction counts
are 13,093,777,329 original versus 13,092,305,494 semantic. These accounting
counts are not a speed claim. Wall times are 67.70/68.53 seconds original and
69.04/68.47 seconds semantic; the perturbation rejects after 58.56 seconds.
Private result: `.local/rng-core-reconstruction/day-final-01.json`.
The fixed replay executable SHA-256 is
`545f513af8112ee45b5905c6ebf1259a0b9f0731bd58e56ce6c72d2eb395f2f1`.
All fixture identities are preserved, including imports SHA-256
`8b544a71b26e718e40fe3eef337ef5ca82b9392167f96e73dc7c042a663d8415`.
No ordering/week campaign was reopened. Date's waived weekly controls remain
unverified and waived; this report does not convert that waiver into a PASS.

Final review tightened the app's existing Date oracle guard to require explicit
original RNG as well as original Date. Three incomplete selections reject before
creating a file; an explicitly original empty execution reaches the recording
branch. This is a selection check, not a valid empty oracle capture. The app/probe
was rebuilt and its linked State dependencies rechecked. The replay executable,
core adapter, generated pages and focused UBSAN units did not change; their
identities were reconfirmed rather than repeating the bounded campaign.
The final app UI acceptance uses this guard-corrected executable.

## Real final-candidate coverage and persistence

The unchanged final signed ARM64 candidate is
`.local/performance/rng-core-final-01/CM Validation rng-core-final-01.app`, with
private Runtime/Runs. Its source executable is
`1b84152cc5ddad11d43fa2b3fc7c0293b20d2dcf0a8008ee0750843867a0d9b6`;
ad-hoc signing produces
`69ad1bddbf3bda28aa4c9708cd1ba03d6a6e00bf7c58a184809f23b942f7b0a5`.
The three final processes all use that same signed binary. Each launch is
preceded by an explicit process-absence assertion. Bindings are recreated after
reopening; no accessibility query is made after final exit because that API can
relaunch a closed application.

| Final process | Actual workflow | rand semantic | seed semantic | initialize semantic |
| --- | --- | ---: | ---: | ---: |
| app.QoeGiw | Startup, preferred Original Load, profile/calendar, first Save, normal exit | 656 | 15 | 2 |
| app.Ra9Fg4 | Exact reopen, first Original Load, normal night processing, second Save, normal exit | 117,100 | 2,530 | 2 |
| app.BpwSYk | Exact reopen, second Original Load, field/result verification, normal exit | 122,002 | 2,459 | 2 |

All original-mode own counts are zero. Startup's two initializer calls use the
original clock at callers `0x10118ed4` and `0x103343ec`. Independent arithmetic
approves all 54 retained samples: 24 draw state/results, 24 seed input/returns
and six initializer cursor/salt/return residues. These first-eight samples per
entry are bounded observations, not comprehensive per-call ABI capture.
`live-samples-final.json` retains the values and log identities.

Both complete Save/normal exit/process absence/exact reopen/Original Load cycles
pass on the final candidate. Original Game Options > Save Game is used throughout.
The native File > Save limitation is unchanged. First save and reload show
Wednesday September 4, 2002 night (`4.9.02 NTE`), Victor Moraes/Cruzeiro, rank 16.
Goias on August 31 remains 0:3; Coritiba/Santos/Colo Colo remain pending on
September 6/8/11. Luisao is born February 13, 1981, age 21, one international cap,
condition 85%, form 7-9-8-6-8 and Very Good morale. Official statistics remain
26(4), one goal, two assists and 7.20 average; noncompetitive two games, 7.50.
His full-time contract remains BRL 8,750/week, start N/D, linked, expiring
December 24, 2004, rotation status and no bonuses or clauses.

Limited normal Continue processes the night's matches. The final validation
stops at the same date/night phase, after results, without playing a Cruzeiro
match or forcing dates, RNG or results. The second save preserves that progress:
after the second Load the original game resumes on news, and Alex's original
match news confirms Brazil's 5:1 victory over Uruguay, observed before Save as
Uruguay 1:5 Brazil. The same club fixtures, player/profile and contract fields
are reconfirmed. This is a bounded real progression, not a full-career test.

| Private save in final Runtime/Files/games | Bytes | SHA-256 |
| --- | ---: | --- |
| RNGFinal-04Sep2002.sav | 34,543,289 | 18cca88066fc9e1e3655e80c0e9bf6171eb711600816eac94d5328503f931c4a |
| RNGFinal-04SepResults.sav | 34,872,328 | 44dd1f372cf9f568f63b1f2e5942c5a68f357cb816b1b2c9b58941c32580e4a3 |

Both identities remain unchanged after their Load. The preferred source and its
candidate copy retain SHA-256
`0d0ca840ef270751a2c8537191aacff301c52fb653f34c9bb5171d172fdcf86a`.
The alternative Date save and LFS reference retain their original identities.

An earlier candidate supplied additional startup and progression evidence, but
its second persistence cycle is not accepted: an accessibility observation after
exit automatically reopened the app before process absence was checked. A second
attempted instance failed early with `Unknown original resource handle`, before
RNG initialization or save loading. Both facts are retained in the preliminary
incident receipt; no failed run is a PASS. The final sequence above replaces that
acceptance attempt. On the final second process, the coordinate-control API
returned `noWindowsAvailable`; the existing Validation > Click at position UI
sent unmodified original clicks instead. This recovered the same process without
patching game state. No production UI/input change was made for either issue.

## Effective RNG persistence boundary

The CRT slot `0x108b85cc`, salt `0x10be8288`, end `0x10be8280` and cursor
`0x10be8284` are original static storage, outside allocated world records.
Original Load restores the observed career date, phase, manager/club, fixtures,
player/contract and processed results. It does not restore a suspended machine
snapshot or the previous startup CRT trajectory. Fresh startup uses original
time and two initializations; original callers may subsequently seed again from
their inputs. The caller at `0x10430f9c`, for example, calculates its seed from
record/local values before the seed call rather than reading back the CRT slot.

Across all final processes, the optional storage inventory observes every write
(including copy-in and clear) to those four fields. CRT writes occur only at the
rand/seed hooks. End has exactly two writes per process, both in initialization.
Besides initialization, all 84 observed salt-store PCs follow generated rand
refreshes. The 1,080 distinct cursor-store PCs classify as 993 generated
increments by two and 87 generated wraps followed by rand. No bulk Load write
restores any of these four live fields. Original consumer bodies remain intact.
This explains why requiring the prior CRT/cursor sequence to persist would change
the original behavior. Deterministic replay verifies RNG evolution from its
complete snapshot/transcript independently of this real save/reload evidence.

This is an effective-load characterization, not a complete save-byte-format map:
`Memory.copy_out` reads are not observed by the compact storage inventory, so it
does not establish absence of every possible unused byte copy in a save. It does
establish the observed restore/write behavior. The private static contexts and
`storage-persistence-characterization.json` retain the classification evidence.
Diagnostics themselves are excluded from machine snapshots by direct comparison
and have no game-memory representation to be serialized by the original saver.

## Reproduction

Original segments and the immutable fixture are private prerequisites. These
commands reuse existing original data; they never publish or rewrite it.

```sh
python3 tools/compile_recomp.py .local/recomp-input .local/recompiled --root 0x10773710 --all-analyzed
cmake -S native -B .local/aot-build -DCMAKE_BUILD_TYPE=Release -DCM_GENERATED_DIR="$PWD/.local/recompiled" -DCM_ORACLE_DIR="$PWD/.local/recomp-input" -DCM_DATE_SNAPSHOT="$PWD/.local/performance/season-day-01/SeasonFixture/initial.snapshot"
cmake --build .local/aot-build -j 4
ctest --test-dir .local/aot-build --output-on-failure
.local/aot-build/cm-rng-core-checks .local/recomp-input .local/rng-core-checks-new
python3 -m unittest discover -s tools -p 'test_*.py'
python3 tools/test_control_cursor_audit.py .local/recomp-input/instructions.sqlite
python3 tools/test_match_presentation_audit.py .local/recomp-input/instructions.sqlite
python3 tools/test_unsigned_division_audit.py .local/recomp-input/instructions.sqlite
python3 tools/test_local_temporaries.py .local/recomp-input/instructions.sqlite
python3 tools/test_generated_temporaries.py .local/recomp-input/instructions.sqlite --sanitize
python3 tools/check_rng_core_season.py .local/performance/season-day-01/SeasonFixture --report .local/rng-core-day-new.json
```

The exact focused UBSAN build recipe is retained privately in
`.local/rng-core-reconstruction/ubsan/run.py`; run it with `python3` after rebuilding
the normal libraries. It uses `clang++ -std=c++23 -arch arm64 -O1 -g
-ffp-contract=off -fsanitize=undefined -fno-sanitize-recover=all`, replacing the
listed runtime/adapter/harness/transcript/callback/page objects before linking
the unchanged remaining native libraries. Its compact report and build logs are
retained after generated payload cleanup. `checks-final.json` records the final
suite commands, exit statuses and durations. Cleanup requires rebuilding removed
test executables before another full CTest run.

## Protected installation

Exactly one `python3 tools/install_native.py` call completed successfully,
from `2026-10-04T21:35:44.301598-03:00` to
`2026-10-04T21:35:45.312248-03:00`. This effective install timestamp identifies
the delivery; an older copied `.app` directory mtime is not executable evidence.
Immediately before and after installation, complete content/forks/FinderInfo/
all-xattr/mode/mtime/flag comparisons pass for personal Game/Runtime/Runs:
809 / 5 / 44 files. All 1,491 protected historical identities and the new prior
installed reference remain unchanged. The final source and signed candidate
inventories also remain identical throughout UI and installation.

| Role | Executable SHA-256 | Full bundle files |
| --- | --- | ---: |
| Final source | 1b84152cc5ddad11d43fa2b3fc7c0293b20d2dcf0a8008ee0750843867a0d9b6 | 2 |
| Signed final candidate | 69ad1bddbf3bda28aa4c9708cd1ba03d6a6e00bf7c58a184809f23b942f7b0a5 | 7 |
| Installed | 1b84152cc5ddad11d43fa2b3fc7c0293b20d2dcf0a8008ee0750843867a0d9b6 | 6 |
| New pre-RNG installed reference | a35a303f441d352d61e0ab0a8e154e06f14f747461310c9e6ac28ce36abaf1e5 | 6 |

Every executable is ARM64. Installed common files match source content and all
metadata; its four bundled original files match the private program inputs.
The production plist equals the source plist and has no validation/capture or
LSEnvironment flags; no CM_AOT diagnostic variable is inherited by the installer.
Old references, candidates, fixtures, both historical Date saves, the immutable
LFS save, personal data and all eight pending handoffs remain preserved. No save,
original program payload, private log or handoff is part of the product commit.

## Reproducible payload cleanup

After acceptance and installation, 58 regenerated files totaling
5,607,697,928 bytes (5.61 GB decimal) were removed. Free space increased from
20,982,489,088 to 26,590,380,032 bytes at that checkpoint. Every deletion was
preceded by a current SHA-256/size match, a goal-time mtime check and exclusion
from the protected-identity list. `cleanup-plan.json` and `cleanup-receipt.json`
retain every exact private path, size, hash and removal result.

The removed CMake test executables, all regenerated in this goal, are:

```text
cm-allocator-checks cm-aot-checks cm-audio-checks
cm-background-checks cm-bridge-checks cm-calendar-checks
cm-calendar-reference-checks cm-callback-checks cm-competition-dispatch-checks
cm-control-cursor-checks cm-date-before-checks cm-date-construction-checks
cm-date-day-of-month-checks cm-date-difference-checks cm-date-month-checks
cm-date-ordering-checks cm-date-presentation-checks cm-date-subsystem_audit-checks
cm-date-weekday-checks cm-day-advance-checks cm-decimal-checks
cm-event-checks cm-floating-point-checks cm-input-checks
cm-match-arithmetic-checks cm-match-presentation-checks cm-match-replay-checks
cm-match-scheduler-checks cm-memory-checks cm-menu-checks
cm-network-checks cm-platform-checks cm-printing-checks
cm-resource-bridge-checks cm-resource-checks cm-season-reference-checks
cm-shutdown-checks cm-sound-checks cm-unsigned-division-checks
```

The other removed payloads are the new CTest outputs
`.local/aot-build/date-construction-private.bin` (86,643,539 bytes) and
`date-presentation-private.bin` (459,350,529 bytes); the goal-private
`rng-focused`, `rng-focused-debug` and its three dSYM files; the focused
`ubsan/rng-core-ubsan` executable and ten objects (`runtime`, `rng-adapter`,
`checks`, `season`, `callbacks`, `page_10747`, `page_1077b`, `page_10712`,
`page_1077f`, `page_10780`); and the zero-byte `guard-original-empty.gz` branch
probe. No immutable season transcript or historical Date archive was removed.
The final production build, both new candidates, all previous candidates and
references, original inputs, saves, compact reports/logs/recipes, the new direct
RNG checker and the replay executable remain available. The entire `.local`
tree was not deleted. Ignore/LFS settings were not changed.

The final post-cleanup preservation pass again matches every personal file and
metadata field, all 1,491 historical identities, and the new installed reference.
All four new preliminary/final saves are unchanged. Full source, signed candidate,
installed and prior-reference inventories match the immediate post-install
checkpoint, including original bundled metadata. The game is closed normally.
The exact product diff is restricted to the RNG implementation, focused harnesses/
audits, private replay selection/EOF support, and these two technical reports.
All eight untracked handoffs remain outside the product commit.
