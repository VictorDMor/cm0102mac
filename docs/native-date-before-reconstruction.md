# Semantic Mac date-before predicate

This increment addresses only independent entry `0x103317f0`, answering whether
its second date is strictly earlier than its first. Addition, subtraction,
difference and the independent later predicate remain delivered. Other date
conversions, ternary caller comparators, age arithmetic and engine subsystems
remain outside this scope. The ascending-year difference quirk remains deferred
as recorded in [the difference report](native-date-difference-reconstruction.md#deferred-improvement--2026-10-02).

The semantic entry is delivered and installed on ARM64. Direct and bounded
original comparisons passed, including an isolated negative control. Real
contract sorting exercised the new entry, and normal Save/exit/reopen/Load
preserved the inspected state before and after a bounded advance. These are
separate evidence types, not proof of a full career or the remaining engine.

## Contract and private provenance

The original supplied Mac export has 26 instructions over the exclusive interval
`[0x103317f0,0x10331858)`. The fingerprint using the existing calendar audit fields
is `e93ed9feee78922e44a69beafbc099f515429b887d7ead794132f8bb59a1f18b`.
Changed and incomplete included bodies fail generation. The private index hash
is `2baf0e4af36613ba9ec3c7ceacdc8f663f24f65bc679d01c2c0e2d7e43d84fec`;
program metadata hash is
`50dda1a1470924b6ae5d7195ad44d042c18fe0cda254bdb6570e5ec176a962d0`.
The original PEF identity remains
`91692f95df17cf851e7c1d6dbdecd0bd0a5809ce1937133e209aee859a6b516b`.
Exports, snapshots, transcripts, builds and writable game data remain private.

The inspected ABI takes the first date through r3 and the second **by value**
through r4/r5. Its five big-endian bytes contain signed16 day, unsigned16 year,
and one leap flag; the remaining three r5 bytes are padding. r4 is not a pointer.
The Boolean result is 32-bit 0/1. Years have priority; equal years compare signed
days. Equality is false in both independent predicates. Leap flags, padding and
month lengths do not participate in comparison and no normalization is added.

Two complete ordered spills write r4 to old r1+28 and r5 to old r1+32 before any
date read. The second year is loaded before the first. Those year loads repeat
on the second-year >= first-year branch. Equal years load the second day first,
then the first day. Overlap of the first record with spills therefore affects
the effective input. The implementation preserves ordered reads and both full
spills. No frame allocation, scratch, initialization, allocator/import or RNG
operation appears in the inspected body; direct differential checks also
confirmed these absences within the mapped test domain.

For unequal years, r0 retains first year, r4 second year, and r5 its input tail.
For equal years, r0 is the Boolean, r4 the arithmetic shift by one of the
sign-extended days' xor, and **r5 the first signed day**. This differs from the
later predicate's second-day r5 clobber. CR0 reflects the unsigned year comparison
and XER.SO; the equal-year day arithmetic does not update CR0. XER.CA changes
only on that arithmetic shift, for negative xor with a discarded low one bit.
PC returns to LR. Other registers, context TOC, LR/CTR, temporaries and stack
pointer remained equal to direct original execution in the accepted checks.

Mapped date/spill records, including overlaps from old r1+24 through old r1+35,
are the intended direct-check domain. Unmapped records, wrapping address-space
accesses and externally mutating memory observers are outside the proven domain;
no broader memory-failure equivalence is claimed.

## Architecture and dispatch

`calendar.hpp::is_before` reverses the established pure year/day order. The
calendar adapter shares the ordering boundary while explicitly selecting its
distinct repeated-year branch and r5 clobber. Arithmetic for clobbers is separate
from the semantic return; normal production executes semantic C++ with no
original fallback for either independent predicate.

The generator hooks the destination label on page `0x10331`. Same-page caller
`0x103317b8`, inter-page caller `0x10332c48`, indirect entry and callback marshalling
reach the same hook. Both callers first consult the later predicate and reach
the earlier predicate on its false branch before their existing tie-breaker.
The comparator bodies and tie-breaker are not replaced.

`State::date_before_calls` counts actual modern calls independently. Private
validation logs identify entry, effective operands after spills, packed input
tail, result and return PC for the first 32 calls. No new game UI option exists.
Original capture still selects all calendar entries through `calendar_original`.
Diagnostic flags/counters are not serialized into snapshots or normal saves.

| Replay mode | Shifts | Difference | Later | Earlier |
| --- | --- | --- | --- | --- |
| Default | original | original | original | original |
| `--calendar-baseline` | semantic | original | original | original |
| `--date-ordering-baseline` | semantic | semantic | original | original |
| `--date-before-baseline` | semantic | semantic | semantic | original |
| `--modern` | semantic | semantic | semantic | semantic |

Historical option meanings remain unchanged. Historical unversioned edge
streams keep their four-entry encoding and are consumed through the existing
`--calendar-record`/`--calendar-check` options. New `--calendar-record-v2` and
`--calendar-check-v2` streams start with `CMCAL002` and additionally include
before-entry boundaries. New streams are saved separately, never substituted
for historical streams or compared as though their formats were identical.

## Verification protocol

The shared direct harness is compiled separately for each independent predicate.
Before checks compare all architectural register bytes, temporaries, PC, exact
ordered spill writes, entire mapped memory, allocation tables and native counts.
The tuple oracle uses effective first fields after spills. A separate Gregorian
oracle covers confirmed valid dates. Pure checks cover every signed16 day against
zero and both signed extremes. The direct instruction budget is 100.

Actual caller checks use controlled synthetic inputs and unmasked original/modern
state comparison. They separately follow the later-to-earlier branch for an
earlier second date and equality, and check the later return for a later date.
These are caller execution checks, not real application observations. Structural
generator checks reject altered/missing bodies and ensure the same-page goto
reaches the semantic hook. Callback restoration remains separate from direct
ABI evidence.

Immutable day/week and 20–21 July fixtures retain strict original-against-original
checks. Baselines and two candidates allow only existing translated instruction
accounting to differ. State, PC, registers, temporaries, every mapped byte,
allocator, imports/callbacks and ordered RNG remain strict. Final recorded
morning/transcript EOF and the original finite budget bound execution.

The private before negative control flips only its first modern result. The
checker identifies the first original before edge and requires rejection at
that exact return boundary. RNG, shifts, difference and later controls remain.
Perturbations are available only in private edge validation and inactive in
normal delivery.

```sh
python3 tools/compile_recomp.py .local/recomp-input .local/recompiled \
  --root 0x10773710 --all-analyzed
cmake -S native -B .local/aot-build -DCMAKE_BUILD_TYPE=Release \
  -DCM_GENERATED_DIR="$PWD/.local/recompiled" -DCM_ORACLE_DIR="$PWD/.local/recomp-input"
cmake --build .local/aot-build -j 4
ctest --test-dir .local/aot-build --output-on-failure
python3 -m unittest discover -s tools -p 'test_*.py'
python3 tools/check_calendar_season.py .local/performance/season-day-01/SeasonFixture
python3 tools/check_calendar_season.py .local/performance/season-week-01/SeasonFixture
python3 tools/check_calendar_season.py .local/performance/date-ordering-capture-02/SeasonFixture
python3 tools/test_control_cursor_audit.py .local/recomp-input/instructions.sqlite
python3 tools/test_match_presentation_audit.py .local/recomp-input/instructions.sqlite
python3 tools/test_unsigned_division_audit.py .local/recomp-input/instructions.sqlite
python3 tools/test_local_temporaries.py .local/recomp-input/instructions.sqlite
python3 tools/test_generated_temporaries.py .local/recomp-input/instructions.sqlite --sanitize
```

`--edges-directory` selects a new private directory to retain a v2 original
stream. A current-reference ARM64 installed app was copied with resource forks
and extended attributes before replacement to
`.local/date-before-reconstruction/reference/installed.app`. Its executable
matches source and installed hash
`b61765e722c6e5f7624550ac15524fcdb623f933e9306c3329644310a887f545`.
This is the current reference, distinct from older backups and signed diagnostics.
Private preservation inventories and checkpoint/fixture hashes were captured
before work and rechecked before and after installation.

## Accepted automatic evidence — 2026-10-02

The regenerated program contains 1,953,157 instructions across 1,922 units,
with no missing or pending targets. The final full build and refresh passed.
All 1,966 linked State-dependent objects use the final header; none is stale
(22 other linked objects do not depend on State).

Both independent predicate harnesses passed 5,646 direct original/modern cases,
156 Gregorian oracle cases and 196,608 pure signed-day checks each. Coverage
includes unsigned year extremes, signed invalid day extremes, month/year and
1900/2000/2100 boundaries, inconsistent flags, all padding variants, initial
condition/carry states, effective aliased records, callbacks and both real
comparators' later-to-earlier/equality branches. The full CTest suite passed
33/33 groups. Python discovery passed 27 tests with four optional groups skipped;
those groups were then run explicitly: control cursor 5, presentation 6,
unsigned division 7, and local temporaries 5 tests, all passing.

The temporary audit examined 1,953,157 instructions and 6,437,244 P-code
operations. Generated UBSAN passed 199,742 differential executions over 2,050
patterns, including 23,247 matching memory failures. Focused UBSAN instrumented
the adapter, both predicate harnesses and generated pages 10331/10332/10717/10718;
before/after, difference (6,892 direct / 494 oracle) and shifts (16,380 plus
bounded SHRT_MIN failures) passed. Platform libraries were linked normally;
this is not whole-program sanitizer coverage or an ASan claim.

For repeatable focused UBSAN, compile each harness with `clang++ -std=c++23
-arch arm64 -O1 -g -ffp-contract=off -fsanitize=undefined
-fno-sanitize-recover=all -Inative`; use `-DCM_CHECK_BEFORE=1` for the shared
ordering harness's before variant. Compile the adapter and the four generated
pages with the same flags, then link these objects before `libcm_aot.a`,
`libcm_ui.a` and `libcm_audio.a` from `.local/aot-build`, with `-lz` and the
AppKit, Carbon, AVFAudio, Foundation, CoreFoundation, CoreServices and
ApplicationServices frameworks. The private objects and logs are retained under
`.local/date-before-reconstruction/ubsan/`.

Each immutable fixture passed two strict entirely original executions, the
original RNG negative, all three historical/delivered baselines and two identical
five-operation candidate reports. Modern call counts per candidate were:

| Fixture | Add | Subtract | Difference | Later | Earlier |
| --- | ---: | ---: | ---: | ---: | ---: |
| One day | 24,534 | 322,828 | 62,897 | 0 | 0 |
| Seven days | 99,279 | 893,137 | 367,792 | 0 | 0 |
| 20–21 July ordering | 13,336 | 51,567 | 25,546 | 38,051 | 12,911 |

Exceptional shift calls were zero. Day/week have no independent-predicate
coverage; their unexercised negatives were omitted explicitly. The ordering
fixture exercised both predicates, so no additional original capture was needed.
RNG/shift/difference/later/before negatives all failed as expected there. Its
first original before edge is 26; the isolated inverted modern return failed
exactly as `calendar edge 26 at 0x103317f0 field=before return byte=36
expected=1 actual=0`. Final date, phase, RNG and native calls remained strict:
2002 day 201 morning, seed 0x2c17, cursor 0x108eb0bc, 167,634,020 native calls.

The ordering fixture's original instruction total was 132,028,129,381; the
four-operation baseline was 132,019,456,540, and each five-operation candidate
was 132,019,214,257. This accounting difference is allowed at the established
comparison boundary, not a timing benchmark. No new state mask was introduced.

All 12 fixture files retain their initial hashes, including the four ordering
hashes in the handoff. Historical streams/reports were preserved. Separate v2
original stream SHA-256 identities are:

| Fixture | CMCAL002 SHA-256 |
| --- | --- |
| One day | `bdce201d4fc83a5c7e8eb9144421726f8bbbe76a1e12f1beaf424f756f5fd083` |
| Seven days | `8ff9f77b64c342d5074136d350850cdf53b10eaf8b1987d9632a226e46974b12` |
| Ordering | `a7849b48626ea92da01ed062417522323477196cd3a3a78c857357076f7b8954` |

These private streams include the additional entry and are not hash-equivalent
to historical unversioned streams. Reports/streams remain under
`.local/date-before-reconstruction/` and are excluded from Git.

## Real application and persistence evidence

The signed isolated candidate used its own copied Runtime/Runs and one ARM64
game instance at a time. It loaded the preserved DateOrder-10Aug2002 checkpoint.
On 10 August afternoon, manager → Player & Staff Search → Search Player →
View Contract → Contract Expiry displayed 2,063 players. Accepted run
`app.FA8EdX` logged 38,422 modern later and **13,153 modern before** calls, with
16 additions, 143 subtractions, zero differences and zero exceptional calls.
All 32 before samples identified entry 103317f0 and return 10332c4c; independent
tuple and Gregorian checks agreed. For example, second 18 December 2004 before
first 16 December 2005 returned 1 (tail 01000014); equal 29 June 2004 returned 0.
Raw effective fields and padding remain in private logs. No normal original
fallback was used. These observations do not replay the whole UI against original.

Calendar, Luisão's profile and contract were checked, then a new private
DateBefore-10Aug2002 save was written. Normal exit, process absence, exact
candidate reopen and Load restored 10 August afternoon and the inspected data.
The original checkpoint was not overwritten. The candidate then advanced through
normal holiday processing until the observed 27 August night; Return from holiday
stopped it. The actual interval was 17 calendar days, not a planned one-day step
or a season. No dates, saves, RNG or memory were edited to force it.

The resulting calendar showed Ponte Preta 10 August 2:1, Internacional 17 August
2:1, Cerro Porteño 20 August 2:1, Vitória 24 August 1:2 and Goiás 31 August pending.
Luisão remained born 13 February 1981, age 21, one international; he was injured,
with official appearances 26 (4), one goal, two assists and average 7.20.
His full-time contract retained BRL 8,750/week, start N/D, bound Yes, expiry
24 December 2004, rotation status and no bonuses/clauses. A new
DateBefore-27Aug2002 save, normal exit, reopen and Load preserved all these
observations. The reload run also exited normally. No own before calls occurred
in the advance/reload runs; the sorting run provides the real-entry evidence.

| Accepted run | Add | Subtract | Difference | Later | Earlier |
| --- | ---: | ---: | ---: | ---: | ---: |
| Sorting/first save | 16 | 143 | 0 | 38,422 | 13,153 |
| Reload/advance/final save | 302,977 | 4,060,079 | 1,006,652 | 0 | 0 |
| Final reload/inspection | 13,349 | 25,890 | 28,517 | 0 | 0 |

All accepted runs logged normal exit and zero exceptional original calls.
Private new save SHA-256 identities are `dfaa610c7f7d2b53006d7b9ceb53509d939033be3573ec1e6b56c6f7c892596d`
(10 August) and `2056240fdc1e6ea179734d8e5e75d11c41475b36e4ee4495d91f338904a8c084`
(27 August). Earlier calendar/difference/ordering checkpoints remain unchanged.

An excluded first attempt used the native macOS File → Save menu and stopped
at the existing `Native menu selections require original MenuSelect dispatch`
limitation in platform.cpp. It is retained privately as a failed route, not an
accepted persistence run. Accepted saves used the original in-game Game Options
→ Save Game route. That unrelated native-menu limitation was not changed.

## Installation and preservation

The validated production build was installed with `python3 tools/install_native.py`.
Source and installed executables are byte-identical ARM64; the isolated candidate
has a separate signature/hash and passed strict code-signature verification.
The production plist contains no private validation/capture settings. Bundled
program metadata and segments match their private inputs.

| Artifact | Executable SHA-256 |
| --- | --- |
| Source and installed | `d2784cfde149a28d8e8c1d067e26ac90cfff482581a36858a0d215a7790194a1` |
| Signed private candidate | `c20788488bb734e5a97590ca75c315a6d5d4f39e75f93fbc65613892ec606511` |
| Preserved current reference backup | `b61765e722c6e5f7624550ac15524fcdb623f933e9306c3329644310a887f545` |

Inventories before work, before installation and after installation agree for
809 personal Game / 5 Runtime / 42 Runs files, including data forks, resource
fork hashes and Finder metadata. The reference backup matched the previous
installed inventory before replacement and remains executable ARM64. All 73
input/segment/checkpoint/reference hashes remain unchanged, including immutable
LFS save `82c504f46ce54a723137dbf42c005d458a4d7b18a9abab4c9f92627aa94f08fb`.
Ignore/LFS rules, prior reports and the preexisting untracked difference prompt
were preserved. This delivery establishes only the independent earlier predicate
within the stated boundaries; no remaining-engine reconstruction, full-career
equivalence or speed gain is claimed.
