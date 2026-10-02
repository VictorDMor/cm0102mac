# Semantic Mac date difference

This increment adds the C++23 replacement for `0x10718130`, preserving the
previous addition/subtraction replacements. Direct equivalence, bounded real
replays, negative controls, application Save/Load and ARM64 installation have
passed. The historical calendar report
remains a separate milestone.
No independent comparison, date conversion, competition/player subsystem,
optimization or UI feature is replaced by this increment.

## Original boundary and provenance

The private Mac export has 111 instructions from `0x10718130` through the return
at `0x107182ec` (exclusive end `0x107182f0`) and 218 direct `bl` callers.
No current direct caller shares the routine's page. The only nested call is the
leap helper `0x10718440`. The export identifies PowerPC big-endian 32-bit,
Ghidra 12.0 and the supplied executable SHA-256
`91692f95df17cf851e7c1d6dbdecd0bd0a5809ce1937133e209aee859a6b516b`.
`calendar_audit.py` pins assembly, P-code, addresses and flow records with
SHA-256 `caf9175e8cf1dcf26a7cdd61e644ab9f38576ef2516f3208d1938e36b42fb30b`.
Every included fragment must match the complete routine. Missing entries,
truncated bodies, changed operations and changed leap helpers fail generation.
The original private input index and segments are not public dependencies.

r3 points to the first five-byte date: signed 16-bit day of year, unsigned
16-bit year, then one leap byte. r4 contains the second date's day in its high
half and year in its low half. r5 contains the second leap byte in its high
byte and three padding bytes. These are **eight bytes by value**, not a second
pointer. The original spills r4/r5 at entry stack +28/+32 and copies all eight
bytes to frame +56/+60. Padding survives both copies although it does not
participate in date arithmetic. The result is a signed 32-bit day count in r3.

The embedded ordering uses unsigned years, then signed days within equal years.
For equal years the result is first.day minus second.day, including negative or
invalid days. Equal dates return zero; one-day differences return +1/-1 in the
expected order. Leap bytes do not select lengths. Invalid days are not normalized
and the returned result is not narrowed to 16 bits.

For first.year greater than second.year, the routine adds Gregorian lengths for
years first.year-1 down through second.year, then adds the day difference.
For first.year less than second.year, it **subtracts lengths for years
first.year-1 through second.year-2**, then adds the day difference. All leap
helper arguments narrow to unsigned 16 bits, including year zero minus one.
Consequently difference is not generally antisymmetric: January 1 2000 minus
January 1 2001 is -365, while the reverse is +366. This observed ascending-year
quirk is preserved. Gregorian arithmetic is an independent oracle for descending
valid dates, same-year signed day differences, and ascending valid intervals
whose displaced endpoint year lengths agree. It is not the oracle outside that
domain. At most 65,535 monotonic year steps occur for supplied unsigned years;
all supported date values terminate and the accumulator fits signed 32 bits.

## Memory and ABI effects

With the original TOC `0x108bba90`, the scratch slot is `0x108b7ad0` and its
initialization byte is `0x108b84d0`. Zero initializes day 40/year 1972/leap 1
before reading the first date; any nonzero byte suppresses initialization.
An alias of uninitialized scratch therefore sees the initialization effects.

Same-year calls leave initialized scratch untouched. Different-year calls write
the first year, then reload/write its day, then reload/write its leap byte.
These accesses must remain ordered for partial overlaps. The loop changes only
scratch.year, retaining its copied day/leap even when inconsistent. Finally the
routine reloads the first day and the spilled second day; aliases can alter the
first day's result. The adapter retains this sequence without bulk-copying the
first operand. The entire by-value operand and padding are retained on the stack.

The 80-byte frame preserves its back chain, caller linkage at stack +8,
saved r28-r31 at -16/-12/-8/-4, local by-value copies at -24/-20, and argument
spills at +28/+32. All bytes written, including their order and values, are
compared directly. r1/r2 and nonvolatile registers are restored; r0 ends as the
restored link, LR and PC become the caller return, r3 holds the result and r6
retains scratch. r4 becomes the signed second day for same-year comparison;
cross-year calls retain the original leap helper's multiply-high clobber. r5
contains the first year for same-year calls, or the last helper argument after
year traversal. Ghidra's call-context TOC register at offset 2440 is updated only
when a helper call occurs. CR0 ends as equality plus XER.SO. Same-year signed
XOR shifting changes XER.CA; cross-year helper shifts retain their original carry
effects. Other architectural register bytes and all generated unique storage
remain unchanged. `call_original` restoration is tested separately and is not
used to conceal the direct ABI clobbers.

The function makes no imports or RNG calls. Entire mapped memory comparison
includes RNG fields and stack, rather than comparing only named date fields.
Supported alias cases cover the first date separate from scratch, scratch itself,
and offsets -2/-1/+1/+2/+3 around scratch. Aliasing with global pointer storage,
TOC, initialization byte, linkage/argument spills or unmapped pointers is not
claimed exhaustively equivalent. No new normalization or fallback is introduced.

## Architecture and dispatch

`calendar.hpp` supplies memory-independent ordering, signed year contributions
and difference arithmetic. `calendar_adapter.cpp` preserves the legacy accesses,
initialization, frame and observable register effects. The production path uses
semantic components for every difference call; it never delegates to the
original difference routine. Addition/subtraction and their existing exceptional
SHRT_MIN behavior remain intact.

The compiler intercepts the routine's entry label. Inter-page direct calls,
indirect `bctr` dispatch and callbacks are exercised by the direct test binary.
Same-page call emission plus the intercepted destination label are checked in
the generator test; the current export has no direct same-page difference caller.
The previous entry hooks still cover both shift operations. The production app
has no selector or new UI. Private diagnostics distinguish:

| Replay mode | Addition / subtraction | Difference |
| --- | --- | --- |
| default | original | original |
| `--calendar-baseline` | delivered semantic modules | original |
| `--modern` | delivered semantic modules | semantic module |

All three use the same executable and other subsystems. Diagnostic selectors and
call counters are not serialized game-machine state. `modern-difference` counts
actual semantic entry dispatches; the prior add/subtract counters cannot establish
this replacement's use. A validation app logs its first 32 actual difference
calls with first/second year/day, all four r5 bytes, result and return PC.

## Automatic comparison boundary

The original-versus-original replay and its RNG negative control remain strict.
Semantic replay still permits exactly the previous instruction-counter difference;
no mask of registers, temporaries, PC, memory, allocator, imports/callbacks or RNG
is added. Each semantic entry counts one actual dispatch step. The final recorded
morning and transcript EOF terminate normally; the original final instruction
budget and host timeout remain finite bounds. The candidate is never recaptured
as its own oracle.

A new private edge stream is recorded using entirely original execution against
the immutable fixtures. Difference edges include all raw arguments, the first
input record, result, scratch/flag, observable output registers (including r6 and
context TOC), condition/carry bytes and 116 bytes spanning frame/linkage/spills.
The existing shift edge records remain checked in order. Modern and delivered
baseline runs consume the same stream. An isolated private difference perturbation
adds one to r3 on the first modern difference call; it must fail at that first
return boundary. Existing shift-day and RNG controls remain enabled. Perturbations
require a private modern edge reader and are cleared when it closes; the app
cannot activate them. Original fixture and edge hashes are checked before/after.

## Reproduction

All evidence, exports, streams, binaries and saves stay under ignored `.local`.
With the verified original export present:

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

Instrument the adapter and both original calendar pages explicitly; remaining
platform bridges are linked from the normal build, not claimed instrumented:

```sh
clang++ -std=c++23 -arch arm64 -O1 -g -ffp-contract=off \
  -fsanitize=undefined -fno-sanitize-recover=all -I native \
  native/date_difference_checks.cpp native/calendar_adapter.cpp \
  .local/recompiled/page_10717.cpp .local/recompiled/page_10718.cpp \
  .local/aot-build/libcm_aot.a .local/aot-build/libcm_ui.a .local/aot-build/libcm_audio.a \
  -framework AppKit -framework Carbon -framework AVFAudio -framework Foundation \
  -lz -framework CoreFoundation -framework CoreServices -framework ApplicationServices \
  -o .local/date-difference-reconstruction/difference-ubsan
.local/date-difference-reconstruction/difference-ubsan .local/recomp-input
```

A full build after the finalized State header recompiles every dependent object.
No overlapping header edit or interrupted stale-layout object is accepted as a
final candidate. Live validation uses one ARM64 process and a Runtime copied by
`prepare_validation.py` from the prior private calendar checkpoint. Save and Load
are ordinary game actions; snapshots/replay do not substitute for persistence.

## Final automatic evidence

The final 6,892-case direct comparison passed normally and under UBSAN, preserving
complete registers, temporaries, PC, ordered write sequences, every mapped byte,
allocator, padding, scratch, flags and absence of imports/RNG. Coverage includes
thirteen representative years, short and 0/65535 spans, signed/invalid days,
leap/century boundaries, inverted/equal operands and seven scratch alias patterns.
494 cases also passed the independent mathematical/Gregorian domain oracle.
Callback, real inter-page `bl` and real indirect `bctr` checks passed separately.
The unchanged 16,380 shift comparisons also passed under UBSAN; both existing
1,232-case original/modern shift oracle groups passed in the complete suite.

The final CTest suite passed 31/31 groups in 75.82 seconds. Python discovery
passed 25 tests (four optional groups skipped); all four optional groups were
explicitly executed against the private index and passed: control cursor (5),
match presentation (6), unsigned division (7), and local temporaries (5).
The audit proved 1,953,157 instructions / 6,437,244 P-code operations local.
Generated instruction checks passed 199,742 differential executions across
2,050 patterns under UBSAN, including 23,247 matching memory failures. This
increment does not claim an AddressSanitizer pass or instrumented platform
bridges. The earlier ASan initialization limitation remains historical evidence.

A full rebuild followed the finalized State header, then a small dependency
refresh included the final adapter and test changes. All 1,964 **linked** objects
depending on State were newer than that header. One historical, unlinked
`cm-aot-probe.dir/aot_probe.cpp.o` was excluded after checking current link
commands; the actual probe host object was rebuilt. No stale-layout object is
linked into the delivered app.

Each immutable fixture passed two strict original runs plus the existing RNG
control, one entirely original edge recording, two modern edge-checked runs,
one delivered-calendar baseline run, and modern RNG/shift/difference controls.
Only translated instruction accounting differed. Full state/transcript checks
remained enabled. Sixteen identities covering the original index/segments,
fixtures/streams, prior checkpoints and LFS save matched before and after.

| Fixture | Modern add / subtract / difference | Final date/phase | Entire original / delivered baseline / candidate instructions | Native calls |
| --- | --- | --- | --- | --- |
| One day | 24,534 / 322,828 / 62,897 | 2002 day 39, morning | 13,143,064,675 / 13,105,615,004 / 13,097,920,791 | 41,620,536 |
| Seven days | 99,279 / 893,137 / 367,792 | 2002 day 153, morning | 159,105,908,509 / 158,999,925,969 / 158,952,761,001 | 238,517,060 |

The difference defect failed at edge 0 in the day fixture (return byte 36,
expected 49/actual 50) and edge 14 in the week fixture (expected 25/actual 26).
The existing shift defect failed at its first relevant shift, edge 1/day and
edge 0/week, with the same original day mismatches (38/39 and 145/146).
The RNG control retained the first differing native-input register at
`0x10bf7bbc`. No exceptional original shift occurred; difference has no fallback.
The immutable original edge-stream hashes are
`cd3eb269c6e3a6f95edf54b31483eae67e1dbb2da126009a2c385561a6c2f5a1` (day) and
`35cb1d573eac87495e723edbf0242129be6cbe289acf754e3aa926690c61db93` (week).
They were unchanged across all consumers. Instruction totals are not a matched
wall-time benchmark; no speed improvement is claimed.

## Final real application evidence

`prepare_validation.py` copied the prior private calendar Runtime into
`.local/performance/date-difference-live-01`. Only one game process ran at a time;
the installed Game files were read only. Existing checkpoints remained unchanged.
The original UI loaded `Calendar-Single-19Jul2002`, displaying 19 July 2002 night,
Victor Moraes / Cruzeiro, the July 31 Palmeiras fixture and August 3 Fluminense,
August 7 Atletico Paranaense and August 10 Ponte Preta fixtures.

Rincon's profile showed birth 14 August 1966, age 35, 92 caps / 19 goals,
condition 100%, 17 official appearances / 2 goals / 1 assist / average 6.94.
His full-time contract displayed R$29,500/week, start 30 January 2002, expiry
2 December 2003, important-first-team status, R$1,100 per-assist bonus and the
R$0 manager-job release clause. Normal Save to `DateDiff-19Jul2002`, normal
exit, reopen and Load restored the date and reexamined profile/contract fields.
Run `app.QFbzTT` exited normally with 797 modern additions / 591 subtractions and
zero difference calls. Opening those panels alone did not exercise difference.

The original holiday UI advanced normally; 31 July night was observed with
Palmeiras 2-1 Cruzeiro. Return from holiday was confirmed on 3 August afternoon.
At the paused checkpoint, the club calendar retained the Palmeiras away 1-2
result and pending Fluminense, Atletico Paranaense, Ponte Preta, Internacional,
Cerro Porteno and Vitoria fixtures. Rincon remained age 35 / 92 caps / 19 goals,
condition 100%, with 18 official appearances, 2 goals, 1 assist, average 6.94 and
form 6-7-6-7-7. His examined contract fields were unchanged. These are observed
career results, not a recaptured full-career original comparison or evidence
that the reconstructed operation alone caused player/fixture changes.

The advancing run `app.NRGc3I` logged **500,953 modern differences**, 190,702
modern additions, 1,385,991 modern subtractions and zero exceptional originals.
Its first 32 actual difference samples were independently checked with Python
`datetime` in their valid descending Gregorian domain, all agreeing. Examples:

| First year/day | Second year/day | r5 including leap/padding | Signed result | Caller return PC |
| --- | --- | --- | --- | --- |
| 2002/199 | 2001/354 | `00000000` | 210 | `0x1035aa5c` |
| 2002/199 | 1996/1 | `01000000` | 2390 | `0x1035c500` |
| 2003/359 | 2002/199 | `00000000` | 525 | `0x1035c530` |

Normal Save to `DateDiff-03Aug2002`, exit, reopen and Load restored 3 August
2002 afternoon, Cruzeiro, the examined result/future fixtures, player profile,
statistics and contract. Reload run `app.cSl13Y` then exited normally with
798 additions / 592 subtractions and zero differences. Difference execution is
proven by the advancing run and samples, not inferred from a reload counter.
Neither dates, saves, seeds nor memory were patched. A second annual UI passage
was not attempted; year/century equivalence comes from direct tests.

Both new private saves retained their post-Save hashes through reload:
`cb0700048651d2d23bd6b457dd5f6d2b4cfbefd3587cc7ddc8e932c196d7ae7e`
(`DateDiff-19Jul2002`) and
`256dca40fbb2ab53fc3543bdd7a8fb0c8c6ba1ebcb80e411b3f333642c3b67ae`
(`DateDiff-03Aug2002`). Their files, real-call samples, observations and logs
remain ignored and private.

## Installation and delivery identity

| Artifact | Executable SHA-256 |
| --- | --- |
| Preserved installed reference | `dbe6a1f963bb33a702bbbdc29fce17d2e5f75d822da7d09f7fc66246641ad741` |
| Final source application | `ca725a106fade1354e6e774364543b86dbf1eb9baa930c2ab55e096d72da0e61` |
| Isolated signed validation application | `56eae291b951c560d698447bc9468cb5586e81248fb5832ee208ee96b182eb4f` |
| Independently verified installed application | `ca725a106fade1354e6e774364543b86dbf1eb9baa930c2ab55e096d72da0e61` |
| Final replay executable | `840d400bb34a792710d829fa34dd2817e62bf9dd7220b1972e1472731ffc4ec2` |

The existing installer delivered the ARM64 source application to
`~/Applications/CM 01-02 Native Prototype.app`. It contains no validation or
capture bundle properties. All four original program resources agree byte for
byte with the verified private inputs. The installed binary is identical to
the source used for the signed validation copy; the signature/bundle identity
makes the diagnostic executable's hash distinct. Real gameplay ran in that
isolated copy; the installed bundle was independently verified on disk.

Before/preinstall/after inventories agree for all **809 Game, 5 Runtime and
42 Runs files** currently in the user's installed support directories. The
handoff's older 9 Runtime / 41 Runs inventory is historical, not the current
count. File content, resource forks, FinderInfo, all extended-attribute hashes,
modes, flags and modification times were preserved exactly. No new checkpoint
was copied into installed personal data. The backup below remains executable.
Private reports include `day-final.json`, `week-final.json`, `ctest-final.log`,
`python.log`, `ubsan-final.log`, `shifts-ubsan.log`, `generated-sanitize.log`,
`build-consistency.json`, inventories, identities and `live-observations.json`
under `.local/date-difference-reconstruction`. Normal commit/push records the
local revision and freshly queried remote equality separately; no private
artifacts or ignore/LFS changes belong in that commit.

The previous installed reference was independently reidentified with SHA-256
`dbe6a1f963bb33a702bbbdc29fce17d2e5f75d822da7d09f7fc66246641ad741` and preserved
as an executable private bundle at
`.local/date-difference-reconstruction/reference/installed.app`. It is distinct
from the former signed diagnostic bundle
`ac3c584bd029834161aa89c5593e5ecad8a2f65193db476fd4f526490a239037`.
The immutable LFS save remains
`82c504f46ce54a723137dbf42c005d458a4d7b18a9abab4c9f92627aa94f08fb`.
No full-career replay, historical-hardware equivalence, exact year-change UI
instant, reconstruction of the rest of the engine or speed improvement is claimed.
