# Native season reference and bounded regression comparison

Status: **real season, annual persistence, bounded comparisons and ARM64
installation verified**.
The active priority supersedes performance optimization. Existing performance
experiments are preserved as prior work; no new optimization, gameplay feature,
rule, graphics change, or semantic subsystem reconstruction belongs to this increment.

## Reference identity and private artifacts

The starting revision is `b9589e295e093cf501d0b21b02131943f4bd9379` on `main`.
The working tree already contained runtime, compiler, UI, tests, and performance
changes. A private patch and source-file hash inventory were retained under
`.local/season-reference` before the season work. No existing work was discarded.

The immutable Git LFS save `saves/Mac-Teste-Validado-Pos-Jogo.sav` has SHA-256
`82c504f46ce54a723137dbf42c005d458a4d7b18a9abab4c9f92627aa94f08fb`.
It remains unchanged. All other saves, runtime copies, snapshots, native-service
transcripts, logs, original program files, binaries, and pictures stay private.
They are not added to Git or uploaded.

`tools/prepare_validation.py` prepares a distinct ARM64 app and copies its runtime.
For capture it also records the source revision, working diff hash, source-file
hashes, original-program metadata hash, executable hash, and initial runtime file
hashes in a private manifest. Ad-hoc signing can change the copied binary hash;
the source hash and final signed hash are recorded separately. Manifests and
snapshots are local evidence, not distributable test assets.

The first source candidate executable is ARM64 only, SHA-256
`f98057f0040294e04a7b556a23148ed281f2c1bafd806637e71c00c2750bfc25`.
The matching bounded comparison executable has SHA-256
`f89f86d36864cecb09da5dcfb768cb6fea4788116bc1a2d8dc20496d46ffba36`.
These identify the first captured reference candidate. The final candidate below
adds the missing World Cup dispatch destinations while preserving its prior
instruction records; both captured fixtures also pass against that candidate.

## Observed application execution

The previous validation application was open at Cruzeiro on 11 January 2002.
Attempting Command-A in the original Save name editor stopped at the missing
`CarbonLib::MenuKey` bridge. The diagnostic run was preserved. The new bridge
uses the current menu bar, enabled menu/items, case-insensitive MacRoman keyboard
equivalents, and the original menu ID and one-based item number. Input checks
cover matching, disabled/missing equivalents, and positions after menu edits.

The isolated `season-reference-01` candidate reopened the reference copy at
11 January 2002, ran the original holiday/Continue flow, and reached 8 February
2002 morning. The UI displayed the 2002 Mineirao and national league scheduling
news. The original Save routine wrote `Season-08Feb2002.sav` in that private
runtime. Exit was normal according to the application log. This is an
intermediate checkpoint. The later annual checks are documented separately below.

`season-day-01` loaded this save through the original Load interface, restoring
8 February morning, Victor Moraes at Cruzeiro, the scheduling news and the
televised Uberlandia fixture on 9 February. Its one-day capture completed at
9 February morning. `tools/check_season_reference.py` executed two equivalent
replays successfully, required rejection of a one-bit initial RNG perturbation,
and verified SHA-256 identity of all four fixture files before/after validation.
The interval contained 2,068,960,818 original instructions, 873,591 native calls,
and 1,899,859 observed original RNG state writes. All mapped final bytes and the
named date/phase/RNG fields agreed. The perturbation reported its first changed
execution/native-call order at `0x10bf7bbc` and exited with status 1.

The full-season execution subsequently reached 31 March 2002 and
waited at the normal pre-match interface. Continuing exposed the original
two-foreign-player squad limit. Manual selections put Clodoaldo into midfield
and Ricardinho into Rincon's right-midfield position; Marcos Vinicius and Rincon
were omitted. The game assigned Sorin as captain and accepted five substitutes.
This ordinary management decision is part of the scenario, not an engine patch.
High CPU use while this interface waited was not evidence of calendar progress.

The manually managed match ended Uberlandia 2–3 Cruzeiro after 92 minutes;
Leonardo, Alex and Clodoaldo scored in minutes 19, 26 and 36, and Salomao scored
in minutes 77 and 88. No substitution or forced result was used. Normal
post-match navigation returned to the club and holiday progression continued.

On 26 May 2002 afternoon, the original Save interface wrote
`Season-26May2002.sav` (SHA-256
`61ba8388674ddfdea2c05c5d570dba39c17d7c6dac737766dc56441928a24e35`).
The game exited normally. The fresh `season-week-01` ARM64 candidate loaded it
through the original Load interface. Before continuing, visual inspection
confirmed the same date, Victor Moraes at Cruzeiro, the pending afternoon
Mineirao fixtures, Cruzeiro's 58 points from 21 games (19 wins, one draw, one
loss; 66 goals for and 20 against), and the observed squad entries. Sorin's
profile still showed age 26, condition 54%, form 7–8–7–9–7, and 13 official
appearances with two goals and two assists. His full-time contract remained
R$32,500 per week, expiring on 24 December 2004, key-player status and no clauses.
Brazil was the only selected league; the original settings showed real players
and attribute definition enabled. This is a verified intermediate Save/reload,
before the annual transition; the later 2003 checkpoint completes that comparison.

Command-A in the new Save editor no longer stopped at an unimplemented import;
it inserted `a` when no enabled Select All menu equivalent was present. The
filename was cleared with Backspace and entered normally. This observation
verifies the missing bridge fix, not Select All editing behavior.

The `season-week-01` capture completed seven full days, from 27 May morning
(day-of-year 146) to 3 June morning (153), intended to include the final scheduled
state-league regular round. It contains 13,953,710,068 original instructions, 776,398
native calls, and 29,298,568 observed original RNG state writes. The automatic
driver passed two equivalent executions with identical reports and full final
state agreement. The one-bit initial RNG perturbation failed with status 1 at
native input register byte 99, original PC `0x10bf7bbc`. All four fixture hashes
were unchanged before/after validation (`week-repeat-check.json`). The initial
snapshot SHA-256 is
`22a9ae2c05da7c96ee4309d5b39f5bdff162adca8062b72bf4de91ef83f03089`,
the expected snapshot is
`7da3014d2f157a68d046d1715c213b44a5f73fa8897c382d5bcb833917647cb0`,
and the transcript is
`5795198f3fd4675fd67224b32a2d9d51b415b406c3bc6eefa9357db30ad7ee8e`.
The separate visual checks below distinguish the regular round from the final.
During that execution, the computer-use provider reported that macOS
was locked and automatic unlock failed. The user was asked to unlock manually.
The game process remained running; application logs continued changing, but
those logs alone do not establish a later date or competition completion.
No build was installed and no commit/push was made while the real season and
annual persistence checks were pending.

When computer access returned, the original retained application image showed
14 July 2002 afternoon and Argentina versus Spain in the World Cup final. The
application had stopped at uncompiled address `0x101b8548`, with LR `0x101b84a4`,
in run `app.SPubaJ`. The log and retained image establish that point of failure;
they do not establish completion of the final or a normal exit. The diagnostic
alert was dismissed and that stopped instance closed before rebuilding.

The indirect table used by original dispatch `0x101b84a8` has global pointer
`0x108b6688`, table `0x108cc9fc`, and seven entries. Two destinations,
`0x101b8548` and `0x101b84cc`, were absent from automatic disassembly. Read-only
offline discovery of all seven destinations expanded the private export from
1,953,048 to 1,953,157 instructions. Every prior instruction record and every
original segment remained byte-identical; the complete regenerated output
changed only `page_101b8.cpp`. Prior derived inputs were preserved privately.
`tools/build_aot.sh` now includes these roots for fresh exports. No original
program byte, table entry, game rule or RNG operation was patched.

`original_competition_dispatch` executes the original signed-byte dispatch for
all seven valid outcomes and four out-of-range values. It requires each indirect
destination to be compiled, stopping at its first instruction using the existing
budget mechanism. Before promoting the expanded export, this check failed with
the exact observed uncompiled address. It checks dispatch coverage; it does not
replace live completion of the final or validate each outcome body's semantics.

After expansion, all 28 CTest groups passed (`ctest-world-cup.log`, 59.75
seconds), including all 11 dispatch cases. The expanded source app is ARM64,
SHA-256 `45a47c237d470a98fbdbc4535ce4331a150b04179411af315a009adad8a7497b`;
its replay executable has SHA-256
`a8be5478749b73142042a90ef5827fb59f733db2d9ee66dc37ac3520b7d3ff27`.
The isolated `season-reference-02` app loaded the same 26 May afternoon save
through the original interface, displaying the pending Mineirao fixtures, and
continued through 29 May night. Both original snapshot fixtures passed again
with this expanded executable: two equivalent runs, deliberate RNG divergence
rejection, and unchanged fixture hashes (`day-repeat-expanded.json` and
`week-repeat-expanded.json`). Their captured reference identity remains distinct.

At 5 June night, the state competition's regular-phase table showed Cruzeiro first with
61 points from 22 games: 20 wins, one draw, one loss, 68 goals for and 20 against.
Atletico Mineiro had 59 points. Mamore (14 points) and URT (12) displayed the
original red relegation markers. The final regular round on 2 June included Ipatinga
0–2 Cruzeiro. The winners history still began with 2001, so the 2002 historical
title assignment was not yet established at this checkpoint. The later final
legs and next-year participation establish the applicable follow-up below.

The original Save routine wrote `Season-05Jun2002.sav`, SHA-256
`9699cc541826006befd33fb5b57f8a9b52405af6e0990d131463ae2e5ebfa7ba`.
Exit was normal in the log after 120,890,077,932 original instructions. This
retains the completed state-league table before continuing past the World Cup
failure. The same app reopened through the original Load interface and restored
5 June night, Victor Moraes at Cruzeiro, all examined regular-phase table totals, and the
Mamore/URT relegation markers. It then resumed the ordinary holiday flow. This
confirms persistence before continuing into the following calendar year.

The corrected real application passed 14 July and reached 19 July night. Its
World Cup final screen showed Brazil 0–3 Spain; the winners history recorded
2002 Spain, runner-up Brazil, third Germany, host Japan. This verifies completion
and historical title assignment for that competition. The participants differ
from the stopped earlier run (Argentina versus Spain), so the live run is not
a matched-input comparison of that final. The controlled comparisons above are
the separate evidence for equivalent executions. A new normal UI save,
`Season-19Jul2002.sav`, preserves the post-final checkpoint before the national
league continuation (SHA-256
`45227c96b853d0fc50d825bc67aa24c2b5aa440ec32495bf664b5cfe45749879`).
The real application subsequently displayed 7 August national-league fixtures,
21 August Mercosul results, 4 September morning and 18 September night. The full
club season was still in progress at that check. macOS then locked again;
manual unlocking was requested. Later annual evidence comes from the restored
UI and normal Save/reload, rather than CPU activity while access was unavailable.

## Completed season and annual persistence

When UI access returned, the same original holiday run displayed 26 March 2003
night. Returning from holiday stopped it on 29 March 2003 afternoon, with
current Mineirao fixtures. This brackets the annual traversal; the exact
midnight/year-change screen was not directly observed while macOS was locked.
The original engine, calendar and annual processing were not bypassed or patched.

The competition histories in the real application recorded these 2002 results:

| Competition | Winner | Runner-up |
| --- | --- | --- |
| Mineirao | Atletico Mineiro | Cruzeiro |
| National first division | Vasco | Corinthians |
| National second division | Criciuma | America (RN) |
| Copa do Brasil | Gremio | Atletico Mineiro |
| Libertadores | Olimpia | Vasco |
| Mercosul | Corinthians | Nacional (URU) |
| World Cup | Spain | Brazil |

Cruzeiro's 61 points from 22 games on 5 June describe the **regular phase**,
not the final title. The archived 2002 club calendar showed the two Mineirao
final legs: 9 June away at Atletico Mineiro, 1–1; 16 June at home, 0–3. It also
retained the national fixtures through 1 December, a home 3–3 against Juventude,
and the completed Libertadores, Copa do Brasil and Mercosul ties. Historical
winner records verify title assignment; the finals were not all watched live.

Both annual checkpoints were loaded to compare national participants. The 2002
first division had 28 clubs. In 2003 America (RN), Criciuma, Joinville and XV de
Piracicaba appear in that division; Botafogo (SP), Gama, Parana and Recife have
left it and were each found in the 2003 second division. This verifies the
observed promotion/relegation membership changes, without claiming that every
lower-division playoff or relegation calculation was independently inspected.
The earlier Mineirao table marked Mamore and URT `R`; neither appears in its
2003 edition, where Esportiva and Ipiranga (MG) appear instead. Lower state-league
promotion matches were not inspected.

The 2003 Mineirao was already underway: Cruzeiro was second with 9 games,
7 wins, 2 draws, no losses, 25 goals for, 7 against and 23 points; Villa Nova
had 24 points. The new club calendar contained completed 2003 fixtures and
future fixtures: 22 March away at Guarani (MG), 4–1; 30 March away at Uberlandia;
5 April America (MG), 9 April Esportiva, 12 April Ipatinga, 19 April Rio Branco
(MG), and 23 April Villa Nova (MG). The national table was reset to zero games
and points, and Mercosul scheduled its next draw for 15 June 2003. This verifies
a generated following season, rather than only a displayed year number.

Player evidence separates observed state changes from their cause:

| Field | 5 June 2002 checkpoint | 29 March 2003 checkpoint |
| --- | --- | --- |
| Rincon, birth 14 August 1966 | age 35; 92 internationals | age 36; 95 internationals |
| Rincon current official games | 14 appearances, 2 goals, 1 assist, average 7.00 | 7 appearances, 2 goals, 4 assists, average 8.00 |
| Rincon annual history | current 2002 season | archived 2002: 36 starts plus 2 substitute appearances, 6 goals, 3 assists, average 6.95; separate 2003 row |
| Rincon condition | 100% | 80% |
| Rincon contract | full-time, R$29,500/week; 30 January 2002 to 2 December 2003 | same examined fields |

His contract also retained important-first-team status, the original R$1,100
bonus field, and the manager-job release clause of R$0. His birthday/age and
annual statistics were exercised. Attribute and condition differences occurred
over months of training and matches; they are not attributed solely to the annual
routine. Retirement, youth generation, every injury/contract expiry branch and
all annual player attributes are unverified.

Normal holiday management also exercised transfers and contract-related exits.
The 2002 outgoing history recorded Sorin to Atletico Madrid on 20 June for
R$21.5 million. His new profile showed R$60,000/week, expiry 24 June 2007, and
the archived Cruzeiro 2002 row (15 appearances, 2 goals, 2 assists, average 7.47).
The outgoing list showed `Bos` entries for Ricardinho to Sporting on 15 December,
Alex to Juventus on 20 December, and Mancuso to Guarani on 25 December. The UI
records those exits; individual negotiations and exact contract-expiry screens
were not watched. The 2003 incoming list included Flavio, Paulo Madureira,
Edervan, Sidclei, Reidner and a free Rogerio. No transfer outcome was forced.

The original Save interface wrote `Season-29Mar2003.sav`, SHA-256
`3d25ec85bfb2d278e42e736734e24568d7d87917c0bd89026f8403093584112d`.
Run `app.lfzvK2` ended with the original application's normal-exit request after
1,343,795,407,730 original instructions; its process was confirmed absent.
The same ARM64 candidate reopened, loaded that save and restored the examined
date/phase, Victor Moraes at Cruzeiro, the Mineirao table and title, national
title, 2003 club results and future fixtures, Rincon profile/contract/annual
history, and 2002 outgoing-transfer entries. Run `app.jkcgLX` then exited normally.
The original 5 June checkpoint was loaded again, restoring its date/club and
Rincon's earlier age/statistics/contract; run `app.EUkuv3` exited normally. A
further normal 2003 load allowed the division and cup-history checks above,
followed by normal exit in `app.0R7wAM`. Only one game process was used throughout
these checks. Both private saves retained their recorded SHA-256 hashes.

The pre-transition Save/exit/reopen was already performed on 5 June before
continuing the same career into 2003. That checkpoint is several months before
the calendar-year boundary, not a last-day-of-December checkpoint. The interval
was not captured as a full-career replay; annual visual/persistence evidence
and the separate exact one-day/seven-day comparisons have different scopes.

A temporary duplicate of the prior app was reopened during UI binding recovery
and then exited normally before further simulation. Subsequent reference work
uses one game process. No performance comparison is inferred from this activity.

## Bounded capture contract

The hook at original address `0x101195e8` is the recurring phase-processing loop
of `0x10119530`. Capture begins at a morning visit and ends at a later morning
visit after the requested 1 through 7 observed calendar days. Original game
phases are morning/afternoon/night, encoded 0/1/2. Repeated visits on the same
morning do not count as a complete day. The original loop executes all phases,
fixtures, player processing, and calendar updates within the selected interval.
The hook does not change the game date, selected decisions, RNG, or results.

Date mapping comes from the original loop and its date helpers:

| Field | Global pointer | Record offset | Width |
| --- | --- | --- | --- |
| Zero-based day of year | `0x108b3f10` | 0 | 2 |
| Calendar year | `0x108b3f10` | 2 | 2 |
| Leap-year flag | `0x108b3f10` | 4 | 1 |
| Current processing phase | `0x108b40a4` | 0 | 1 |
| RNG additive seed | `0x108b3f00` | 0 | 4 |
| RNG sample cursor | `0x108b3f0c` | 0 | 4 |
| RNG buffer end | `0x108b3f08` | 0 | 4 |

Initial and expected snapshots preserve mapped bytes, machine registers,
generated temporaries, allocator state, instruction/native-call counts, and PC.
The snapshots use the pre-existing private runtime snapshot encoding. They are
not `.sav` files and cannot replace normal UI Save/reload verification.

A compressed, versioned transcript records each native import's exact PC,
register inputs, and execution order. Memory mapping/write/clear effects and
native output state are recorded while inside that import. This includes the
observed clock responses, input EventRecords, file-read contents, and service
results, without predicting which external input can affect the simulation.
Replay runs without `NativePlatform` and consumes these recorded external effects.
It neither restores native host objects nor claims an autonomous headless career.

Before an original callback, recording separates native effects from original
execution. Replay invokes `call_original` with the recorded transition vector
and arguments; it executes the compiled original callback, its nested imports,
and stack/ABI restoration, and checks the callback result. It does not copy a
callback's game-memory changes as if they were native output.

Outside native services the memory observer records writes to the original RNG
seed and cursor. Replay checks their values and order as original code executes.
This distinguishes final RNG state from ordered RNG consumption, including
cursor resets and seed changes. The watched pointer targets are fixed at capture
start for the loaded scenario. Changing the RNG implementation or moving these
fields requires a reviewed fixture/schema update; silent fallback is unsupported.

Comparison checks named date/phase/RNG fields, then PC/counters/allocator,
registers/temporaries, mapped ranges, and every mapped byte. Other game-memory
differences identify the owning allocation and offset when possible. No volatile
bytes are currently excluded. The first native input or RNG mismatch is reported
at its execution boundary. Exact replay is intentionally strict and tied to this
compiled reference's accounting and ABI; it is not proof of complete historical
hardware equivalence or an independent oracle for native bridge implementations.

## Reproduction

Prepare an unused name with a private initial runtime:

```sh
python3 tools/prepare_validation.py season-day-01 \
  --runtime /absolute/private/Runtime --season-reference-days 1
```

Launch and operate the produced app through the original interface. Capture
starts on the next original morning processing boundary. Continue until the
private `SeasonFixture/capture.txt` says `status=complete`. A capture interrupted
before its final boundary is not usable. Save and exit normally before changing
builds. Capture properties are one-shot; use a fresh validation app/directory
for another fixture or a normal validation app for subsequent Save/reload.

```sh
.local/aot-build/cm-season-reference /absolute/private/SeasonFixture
.local/aot-build/cm-season-reference /absolute/private/SeasonFixture
.local/aot-build/cm-season-reference /absolute/private/SeasonFixture --perturb-rng
python3 tools/check_season_reference.py /absolute/private/SeasonFixture
ctest --test-dir .local/aot-build --output-on-failure
python3 -m unittest discover -s tools -p 'test_*.py'
```

The first two commands must pass. The third changes one initial RNG seed bit in
memory only and must fail with a divergence diagnostic and nonzero exit status.
The immutable initial fixture remains unchanged. Replay is bounded by the
expected final instruction count; a changed path cannot run indefinitely.

The `private_season_transcript` component test is explicitly synthetic. It
covers native memory effects and allocator restoration, a real four-instruction
original callback, equivalent executions, changed RNG/input/allocated bytes,
and transcript truncation. Its synthetic date steps do not count as a real day,
a season, or an annual transition. Real captured fixtures must separately pass.

## Validation evidence and delivery

Real application evidence proves the completed 2002 season and ongoing 2003
edition, the inspected titles and membership changes, player/transfer updates,
and normal Save/exit/reopen on both sides. Private `annual-observations.json`
records examined fields and normal-exit runs; it is not published. The separate
one-day and seven-day automatic comparisons each pass two equivalent runs,
reject the deliberate RNG perturbation and leave every fixture unchanged.
The final source app and replay executable retain the expanded-candidate hashes
above. A final cached build completed without recompilation; no functional
source changed after the expanded-candidate suites. The final CTest suite passed
28/28 groups. Python discovery passed 20 tests with four optional exports skipped;
all corresponding optional groups were also run explicitly against the expanded
original index without skips, as recorded below.

The validated ARM64 app was installed at
`~/Applications/CM 01-02 Native Prototype.app`. Its executable SHA-256 is
`45a47c237d470a98fbdbc4535ce4331a150b04179411af315a009adad8a7497b`, exactly
matching the source candidate used for the real season. The isolated validation
copy's ad-hoc-signed identity was
`0fbe4f6bd19b397c210fee646a1a0f47adf3efd427bc17ff7c884c1183c5cfec`.
Installation carries no capture/validation runtime properties. All four original
program files were verified byte-for-byte against the private local inputs.
Private before/after inventories matched for all 809 installed Game files and
all 9 Runtime files, including data hashes, resource forks and Finder metadata;
they also matched the inventory from the start of this goal. The previous app
was retained as a private backup. No private checkpoint was copied into the
user's installed runtime or published. The installed bundle was verified on disk;
the season itself ran in the isolated app using the same executable code.
Normal Git delivery uses the commit containing this report. The private delivery
record must retain the local and freshly queried remote branch SHA and require
equality; no private saves, transcripts, original data, logs or builds belong in
that commit. A full career replay and historical hardware equivalence remain
outside the proven scope.

The first source candidate passed all 27 CTest groups (`ctest-final.log`,
54.03 seconds). Python discovery passed 20 tests, with four optional original
export cases skipped; the corresponding private original-instruction audits
were run separately. The generated-temporary sanitizer check exercised 199,742
differential executions across 2,050 original instruction patterns, including
23,247 matching memory faults. These component results do not substitute for
the separate real season and annual persistence checks.

## First semantic reconstruction candidate

The first proposed semantic reconstruction is original calendar/date arithmetic.
The five-byte
(day-of-year, year, leap flag) boundary is small, named, and used by the exercised
daily loop. `0x10717890` advances a supplied date and returns the same layout;
its helper uses a separate original scratch record and leap-year initialization.
The original-routine checks below establish month/year edges, leap years,
signed offsets, scratch side effects, ABI and RNG preservation. The actual
season traversal establishes the boundary's role in calendar progression.
This is a smaller initial contract than player development or competition
rollover, whose broader dependencies and unobserved branches still need mapping.
A modern C++ replacement is the next increment and is not implemented here.

`original_calendar_reference` directly executes both original routines
`0x10717890` (addition) and `0x10717b20` (subtraction). It checks 1,232 combinations
of January, February/March, and December boundaries; years 1900, 1999, 2000,
2001, 2002, 2004 and 2100; positive, negative and zero offsets up to 730 days;
separate and in-place outputs. Expected dates use independent C++ Gregorian
calendar arithmetic. Checks include both original scratch records, lazy flags,
callback register/temporary restoration, and unchanged RNG fields with no native
imports. These are original-routine component checks, not a replacement or proof
of the season milestone. Unchecked signed-16-bit extremes, aliasing with scratch
records and invalid dates require explicit review in the next increment.

After the World Cup export expansion, Python discovery again passed 20 tests
(four optional exports skipped), and the sanitizer differential check again
passed 199,742 executions across 2,050 normalized original patterns selected
from all 1,953,157 instructions. This verifies the generated translation patterns
and preserves the distinction from real season observation.
All four optional audit groups were also executed explicitly against the
expanded private index, without skipped cases: control (5 tests), presentation
(6), division (7), and temporaries (5). The complete index proved instruction
locality for 1,953,157 instructions and 6,437,244 P-code operations. These verify
preserved compiler work rather than introducing further optimization.
