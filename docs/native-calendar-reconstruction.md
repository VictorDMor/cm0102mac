# Semantic calendar reconstruction

This increment replaces Mac date addition (`0x10717890`) and subtraction
(`0x10717b20`) with a C++23 arithmetic module and a legacy state adapter.
The final direct checks, bounded real simulation replays and complete suites have
passed. Normal month/year advancement and Save/exit/reopen/Load on both sides of
the annual boundary were observed in the ARM64 application. The validated native
application is installed with personal data and captured Mac metadata preserved.
The prior completed season remains historical evidence in
[native-season-reference.md](native-season-reference.md).

## Contract and compatibility domain

A Mac date is exactly five big-endian bytes: signed 16-bit zero-based day at
0, unsigned 16-bit year at 2, and one leap byte at 4. Normal game dates use
nonnegative days within their year. The caller places output, input and offset
in r3/r4/r5. The offset's low 16 bits select a signed direction. High argument
bits do not change the stored day result. These are void routines; the residual
r3 is an observable clobber, not a promised date pointer return.

The original first narrows the day sum/difference to 16 bits, then normalizes
only in the selected direction. Addition with a positive offset that wraps the
day negative leaves it negative. Subtraction that wraps positive does not undo
that wrapping. Invalid input days are not generally normalized to valid dates.
Years wrap modulo 65536. Gregorian leap rules apply to the unsigned stored year,
including year zero, century exceptions and the divisible-by-400 exception.
The input leap byte is recomputed rather than trusted on nonnegative paths.
There is no new validation, exception or automatic Gregorian correction.

Negative offsets initialize and copy the direction's scratch, then delegate to
the opposite operation with a positive magnitude. Both scratch records and
flags therefore have effects. `-32768` is exceptional: negation narrows back to
itself and the originals recursively delegate without a date result. Only this
nonterminating argument retains the original implementation and its bounded
stack/budget failure. It is counted separately and must not occur in accepted
real fixtures. The normal tested application path uses the modern module.

Scratch pointers are at `0x108b7ae4` and `0x108b7adc`; lazy flags are at
`0x108b84d5` and `0x108b84d3`. A zero flag writes day 40/year 1972/leap 1
before reading the input; any nonzero flag suppresses initialization. Therefore
an input aliasing uninitialized scratch observes those initialization writes.
Ordered field copies preserve separate, in-place and partially overlapping
input/output effects; preloading the full date or replacing copies with memcpy
would change these cases. The current-date pointer `0x108b3f10` and phase pointer
`0x108b40a4` are used by the daily boundary, not by these supplied-date routines.
The calendar functions themselves make no native imports or RNG calls.

The adapter preserves the original 80-byte frame, saved registers, linkage,
negative-delegation local record and nested frame residue. All architectural
register bytes, including CR0, XER shift carry, LR and Ghidra's call-context TOC
at register offset 2440, are compared in direct checks. The leap helper's r4
multiply-high clobber and r5 year clobber are reproduced. Generated unique
storage is unchanged by the instruction-local original translation. Callback
checks separately verify `call_original` restoration; that restoration is not
used as evidence of direct routine clobbers.

The characterized valid domain includes normal dates with signed offsets whose
initial narrowed result agrees with mathematical arithmetic. Compatibility
checks also observe wrapped days, invalid days, year bounds, inconsistent leap
bytes, nonbinary initialization flags and shared scratch. Arbitrary unmapped
pointers or aliasing with the routine's own linkage, TOC/global pointer storage
are not a supported date contract and are not claimed exhaustively equivalent.

## Architecture and dispatch

`native/calendar.hpp` contains date fields, leap/year length, signed day narrowing,
one-year movement and addition/subtraction semantics. It has no game-memory,
ABI, native-service or random-number dependency. `calendar_adapter.cpp` performs
ordered legacy reads/writes, scratch initialization and ABI residue preservation.
It does not execute the originals for terminating calendar arithmetic.

The generator hooks both routine **entry labels**, before original instructions.
Same-page direct gotos, inter-page dispatch, indirect calls and native callbacks
therefore reach the same replacement. Internal original instruction entries
remain available for reference diagnostics. A private `State::calendar_original`
selects original execution; the app defaults to modern with no new UI control.
`tools/calendar_audit.py` pins all 193 routine instructions and the 35-instruction
leap helper by assembly, P-code and control-flow hashes; changes fail generation.
Private exports are from the supplied Mac PEF and are not clean-clone dependencies.

Normal exit logs identify mode, top-level modern add/subtract dispatch counts and
exceptional original dispatches. Semantic negative delegation happens inside the
adapter; it is not falsely counted as a new generated entry dispatch. App bundle,
source executable, isolated signed executable and installed executable identities
must be recorded independently.

## Auditable simulation comparison

Original-versus-original replay remains strict: instruction/native counts, PC,
all register bytes, all generated temporaries, allocator tables, mapped ranges,
every mapped byte, native inputs/callbacks and ordered RNG writes are checked.
`tools/check_season_reference.py` and its RNG negative control are retained.

Semantic replay permits exactly one accounting difference: `State::executed`.
One modern entry step counts the actual semantic dispatch; it does not invent
an original instruction total. The replacement skips translated original steps,
while its exact ABI and memory residue lets the existing register/memory checks
remain intact. Instruction totals are host diagnostic accounting, not game data;
clock/input values are supplied by the unchanged transcript. Native-call counts
and order, PCs, entire registers, temporaries, allocator and all memory remain
strict at every existing boundary. No register, PC, memory, import or RNG masks
are introduced. The finite original end count remains an upper bound on actual
modern execution steps; the final recorded morning boundary and transcript EOF
provide normal termination, with a host timeout as an additional bound.

An additional private compressed edge stream is recorded by an original replay
against the immutable fixture. It includes entry/order, effective arguments,
input record, result, both scratch records/flags and relevant direct ABI outputs.
Modern replay consumes it without modifying it. The modern executable cannot
record its own oracle in this mode. This catches a deliberately injected one-day
output defect at its first calendar edge before later simulation effects.
The fault exists only while a private validation edge reader is open, is reset
on close, and has no application configuration or UI switch.

## Reproduction and evidence

With the verified private exports already present, regenerate and configure the
entry hooks before building (a clean public clone cannot run these commands
without the original local data):

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
python3 tools/test_control_cursor_audit.py .local/recomp-input/instructions.sqlite
python3 tools/test_match_presentation_audit.py .local/recomp-input/instructions.sqlite
python3 tools/test_unsigned_division_audit.py .local/recomp-input/instructions.sqlite
python3 tools/test_local_temporaries.py .local/recomp-input/instructions.sqlite
python3 tools/test_generated_temporaries.py .local/recomp-input/instructions.sqlite --sanitize
```

The existing 1,232 original Gregorian/callback cases are retained; the same
1,232 cases also run through the modern integration. A separate 16,380-case
original/modern direct comparison checks complete registers, temporaries, return
PC, exact write footprint and every written byte over 13 years, 10 day patterns,
9 offsets, both directions and 7 alias patterns. It also checks bounded
`SHRT_MIN` failures. The final consistent candidate passed all 16,380 direct
comparisons and both 1,232-case oracle groups. Its full CTest suite passed 30/30 in 58.37 seconds.
Python discovery ran 23 tests with four optional groups skipped; those four groups
were also run explicitly against the private index and passed (control cursor,
match presentation, unsigned division and local temporaries). The generated
temporary audit proved 1,953,157 instructions / 6,437,244 P-code operations.
Generated differential checks completed 199,742 executions over 2,050 patterns
with UndefinedBehaviorSanitizer. The semantic adapter separately passed its
16,380-case direct comparison under UndefinedBehaviorSanitizer.

All exports, builds, saves, copied applications, inventories, transcripts and
logs remain private under ignored `.local` paths. The immutable LFS save is
untouched. No additional private file is selected for version control.

The standalone adapter AddressSanitizer attempt did not reach `main`: local
Apple runtime initialization deadlocked inside ASan shadow setup / dyld shared
cache iteration (private sampled stack retained). This is not a passing check.
UndefinedBehaviorSanitizer passed separately, matching the existing generated
instruction sanitizer workflow. AddressSanitizer remains unverified in this local
runtime.

## Build consistency diagnosis

A preliminary full-build replay exposed stale generated objects from an
interrupted compilation during a `State` layout edit. Four contiguous units
(`page_10207` through `page_1020a`) were absent from the next compilation log;
those in-flight objects had been written after the header timestamp despite
having read its former layout. Dispatch then misread the PC at `0x10207000`.
The isolated UI copy was exited normally without advancing validation. Recovery
recompiled every State-dependent unit with the finalized layout, reran the full
suites and both immutable fixtures, then staged the consistent isolated app.
This build-consistency failure is not counted as calendar equivalence evidence.

The focused UndefinedBehaviorSanitizer check compiles the semantic adapter and
its generated original calendar page with instrumentation, then links the
remaining previously built bridges. It does not claim every platform bridge is
instrumented:

```sh
clang++ -std=c++23 -arch arm64 -O1 -g -ffp-contract=off \
  -fsanitize=undefined -fno-sanitize-recover=all -I native \
  native/calendar_checks.cpp native/calendar_adapter.cpp \
  .local/recompiled/page_10717.cpp .local/aot-build/libcm_aot.a \
  .local/aot-build/libcm_ui.a .local/aot-build/libcm_audio.a \
  -framework AppKit -framework Carbon -framework AVFAudio -framework Foundation \
  -lz -framework CoreFoundation -framework CoreServices -framework ApplicationServices \
  -o .local/calendar-reconstruction/calendar-ubsan-final
.local/calendar-reconstruction/calendar-ubsan-final .local/recomp-input
```

## Final bounded simulation evidence

Both immutable fixtures passed two strict original runs and the existing RNG
negative control, an original calendar-edge recording, two modern runs, and
modern RNG/calendar negative controls. The fixture and original edge hashes
remained unchanged. No exceptional original dispatch occurred.

| Fixture | Modern add / subtract calls | Final date and phase | Original / modern instruction accounting | Native calls |
| --- | --- | --- | --- | --- |
| One day | 24,534 / 322,828 | 2002 day 39, morning | 13,143,064,675 / 13,105,615,004 | 41,620,536 |
| Seven days | 99,279 / 893,137 | 2002 day 153, morning | 159,105,908,509 / 158,999,925,969 | 238,517,060 |

The intentional one-day defect was rejected at calendar edge 0, subtraction
`0x10717b20`, output day byte 34: expected 38/actual 39 in the day fixture and
expected 145/actual 146 in the week fixture. RNG seed perturbation was rejected
at the first differing native input register at `0x10bf7bbc`; it is not presented
as a direct RNG-write diagnostic. Full mapped state and ordered RNG checks
remained enabled. These are bounded real simulation checks, not full-career or
historical-hardware equivalence.

Private reports are `day-consistent.json`, `week-consistent.json`,
`ctest-consistent.log`, `python-consistent.log`, `calendar-ubsan-final.log` and
`generated-sanitize.log` under `.local/calendar-reconstruction`.

## Final application evidence

The consistent ARM64 candidate uses private writable Runtime/Runs under
`.local/performance/calendar-live-02`, with the original installed Game assets
read only. A single game process was verified for accepted observations. A
preliminary mixed-object copy and an overlapping stale UI-binding session were
excluded; both exited normally before the accepted single-process sequence.

At 19 July 2002 night, normal Save to `Calendar-Single-19Jul2002`, normal exit,
reopen and Load preserved Victor Moraes / Cruzeiro, Freddy Euzebio Rincon (born
14 August 1966, age 35, 92 caps / 19 goals), 17 official appearances / 2 goals /
1 assist / 6.94 average, and his R$29,500 weekly full-time contract from
30 January 2002 through 2 December 2003. The June Atletico Mineiro final results
(1-1 away / 0-3 home), Gremio cup semifinals (0-4 away / 2-0 home), July 31
Palmeiras Mercosul fixture and early August national fixtures were reobserved.
This is UI persistence evidence, not a new automatic comparison of those saves.

The preannual exit log records 1,455 modern add calls and 916 modern subtract
calls, zero exceptional original calls. Its private save SHA-256 is
`5e984802bac83d5eaee9a13b575f068d21a5ef623356ed2662a86cf0ff262cbd`.
The original game holiday function then advanced normally from July to August.
The UI showed 26 July morning and 3 August afternoon, with Fluminense 2-4
Cruzeiro, then 11 August afternoon and 21 August night. August 31 / September 8 and
subsequent October, November and December screens also showed normal advancement.

The prior one/seven-day fixtures end in February/June 2002; they do not
automatically prove July-to-January UI results. Previously completed May/June
results visible in the July checkpoint agree with the historical season report;
new holiday results are observations of this candidate, not a full-career
original comparison.

The same process subsequently showed 11 December 2002 night, 20 December
morning while processing December 24 night, and 3 January 2003 morning. The
late-December menu interruption was unavailable during processing; the exact
December 31 midnight instant was not observed. Normal holiday advancement
continued through January, and return from holiday completed on 5 February 2003
night. This proves one exercised annual calendar passage, not additional annual
transitions inferred solely from changed dates.

At the postannual checkpoint, the UI showed Cruzeiro's 2003 Mineirao fixtures:
February 9 away America (MG), February 16 home Villa Nova (MG), February 19 away
Rio Branco (MG), February 22 home Atletico Mineiro, March 1 away Caldense and
March 9 home Ipatinga. The Mineirao table contained 12 participants with zero
games/points; its retained 2002 history showed Atletico Mineiro winner and Cruzeiro
runner-up, consistent with the June final results already in the source checkpoint.
Rincon was age 36, 95 caps / 19 goals, condition 90%, form 7-6-7-7-6 and very good
morale. His unchanged contract still expired December 2, 2003. His history
contained separate 2003 (zero appearances) and 2002 (league 39 (4), 4 goals,
8 assists, 3 man-of-match, average 6.93) rows. Selected 2002 results remained
visible (Anapolina 4-0, Uberlandia 2-1, Democrata 1-1). These specific observed
competition/player boundaries are not an audit of every rollover subsystem.

Normal Save to `Calendar-Single-05Feb2003`, exit, reopen and Load preserved all
these reobserved fields, fixtures, table/participants and histories. The private
save SHA-256 is `33fbf5bc21bde1543c6f6e257c19b2d318be5aeefcaa9c862b81c1e6976c37d7`.
The advancing run `app.HEpxFq` logged 3,191,397 modern additions / 30,938,681 modern
subtractions with zero exceptional originals. The postannual reload/normal exit
`app.ukzRVT` logged 2,179 / 1,676 with zero exceptional originals. Together with
entry-hook coverage and the bounded replay, these are evidence that the actual
application used the modern module. No save, date, seed or memory was patched.
Private live field observations and delivery inventories are under
`.local/calendar-reconstruction`; they are not new public game fixtures.

## Candidate identity

| Artifact | Executable SHA-256 |
| --- | --- |
| Preserved installed reference | `45a47c237d470a98fbdbc4535ce4331a150b04179411af315a009adad8a7497b` |
| Final source app | `dbe6a1f963bb33a702bbbdc29fce17d2e5f75d822da7d09f7fc66246641ad741` |
| Isolated signed live app | `ac3c584bd029834161aa89c5593e5ecad8a2f65193db476fd4f526490a239037` |
| Installed app (independently verified) | `dbe6a1f963bb33a702bbbdc29fce17d2e5f75d822da7d09f7fc66246641ad741` |
| Final replay executable | `fca6152ef29b1ce07ff9dc7e30dff6b207e046213943ca1c15c78ab5daf6a7fb` |

The installed app at `~/Applications/CM 01-02 Native Prototype.app` is ARM64 and
its executable exactly matches the final source app SHA-256 above. The isolated
copy has the same finalized source but a distinct diagnostic identity/signature.
All four bundled original program resources match the verified private inputs.
The original executable reference is backed up privately at
`.local/calendar-reconstruction/reference/installed.app`.

Installation used the existing `tools/install_native.py`, with private before/after
inventories of all 809 Game, 9 Runtime and 41 Runs files. Content hashes, resource
forks, FinderInfo, all extended-attribute hashes, modes, flags and file modification
times agree exactly. The LFS reference remains
`82c504f46ce54a723137dbf42c005d458a4d7b18a9abab4c9f92627aa94f08fb`.
No ignore/LFS rules changed and no exports, game data, new saves, snapshots,
transcripts, logs or binaries are added to Git. The exact delivery commit and fresh
remote SHA are recorded separately after normal commit/push to `main`.

## Next boundary recommendation

After this increment is fully delivered, date ordering/difference and conversion
between this five-byte record and displayed month/weekday are candidates for the
next contract investigation. They share the now-characterized calendar record
and have smaller observable boundaries than competition rollover or player
processing. This is a recommendation, not an approved implementation priority.
