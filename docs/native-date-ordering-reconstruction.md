# Semantic Mac date ordering

This increment replaces the independent Mac date-ordering entry `0x10331860`.
It extends the delivered calendar module without changing addition, subtraction,
date difference, or the preserved ascending-year leap quirk. Date conversion,
other inlined comparisons, age-record arithmetic and larger engine subsystems
remain outside this increment. Correcting the leap quirk remains deferred by the
user and requires a separate decision and validation.
The adjacent opposite-order predicate `0x103317f0` also remains generated;
this delivery does not claim reconstruction of every date comparison.

## Original contract and provenance

The supplied private Mac export contains 26 instructions from `0x10331860`
through `0x103318c4` inclusive. Its three direct callers are `0x10331794`
(same generated page), `0x10332c24` and `0x10717f50` (other pages).
`calendar_audit.py` requires the complete interval `[0x10331860,0x103318c8)`
and fingerprint
`5a74c4775a352a92e713d648900ade2eae03254c96bb84200d5b2378f24b29d2`.
Changed or incomplete included fragments fail generation. The original PEF
identity remains `91692f95df17cf851e7c1d6dbdecd0bd0a5809ce1937133e209aee859a6b516b`;
the private Ghidra 12 export uses PowerPC big-endian 32-bit and contains
1,953,157 instructions. Exports, program segments and diagnostic data remain
ignored and private.

r3 points to the first date. r4/r5 contain the second date **by value**: r4's
high half is the signed 16-bit day, its low half is the unsigned 16-bit year;
r5 holds the leap byte followed by three padding bytes. The return is exactly
32-bit `0` or `1`, answering whether the second date is strictly after the first.
Years take priority and are unsigned; equal years compare signed days. Equality
returns zero. No leap byte, month length or normalization participates, so
negative and invalid days retain their signed ordering. This is not a ternary
comparison and does not calculate a day interval.

The original first stores all of r4 to old r1+28, then all of r5 to old r1+32.
It then reads the second year from r1+30 and the first year from r3+2. When the
second year is not greater it repeats those ordered year loads. Only equal
years read second day from r1+28 and first day from r3. Both stores precede all
date reads, so a first date overlapping the spills can change before comparison.
The adapter preserves these accesses, including all padding bytes. There is no
frame allocation, back-chain write, scratch record or initialization flag.

For different years, r0 retains the first unsigned year, r4 the second unsigned
year, and r5 its original packed tail. For equal years, r0 is the Boolean result,
r4 is the arithmetic right shift by one of the sign-extended days' xor, and r5
is the sign-extended second day. CR0 remains the unsigned year comparison with
XER.SO copied into its low bit; equal-year day comparison does not update CR0.
XER.CA changes only in the equal-year shift: it is set exactly when the xor is
negative and its discarded bit is one. Other architectural registers, LR, CTR,
TOC, context TOC, stack pointer and generated temporaries remain unchanged.
PC becomes the existing LR. No imports, allocation or RNG consumption occurs.

The supported direct checks use mapped records and spills, including first
record overlaps at old r1+24 through old r1+35. Unmapped pointers, wrapping
address-space accesses and externally mutating memory observers are outside
the proven input domain; no broader compatibility claim follows from these
checks. Input flags and padding are preserved in spills but ignored by ordering.

## Architecture and production integration

`calendar.hpp::is_after` expresses year/day ordering independently of legacy
memory. The existing difference helper delegates to this predicate without
changing its arithmetic or replacing any other generated entry.
`calendar_adapter.cpp::execute_ordering` preserves by-value spills, ordered
reads and observable clobbers. Its xor/shift calculation reproduces clobbers,
while the return comes from the semantic predicate; it never calls the original
operation in normal production. The unsigned shift representation avoids signed
arithmetic undefined behavior.

The generator hooks the destination label on page `0x10331`, so the actual
same-page `goto` caller cannot bypass the adapter. Dispatcher calls from other
pages, indirect entry and callback marshalling reach the same hook. Existing
addition/subtraction/difference hooks and original-contract fingerprints remain.
`State::date_ordering_calls` counts actual modern dispatches; prior operation
counters do not establish ordering coverage. Private validation apps log the
first 32 input/result samples using the existing validation environment.
No diagnostic option is added to the game UI.

Private replay modes retain their distinct meanings:

| Mode | Addition/subtraction | Difference | Independent ordering |
| --- | --- | --- | --- |
| Default | original | original | original |
| `--calendar-baseline` | semantic | original | original |
| `--date-ordering-baseline` | semantic | semantic | original |
| `--modern` | semantic | semantic | semantic |

The historical `--calendar-baseline` is not silently redefined. Private app
reference capture selects entirely original calendar entries when the existing
`CM_AOT_SEASON_REFERENCE` capture environment is present, preventing capture of
a semantic candidate as its own oracle. Normal app defaults remain modern.
Diagnostic mode flags and counters are not serialized into snapshots or saves.

## Comparison boundaries and reproduction

Direct checks compare complete registers, temporaries, PC, ordered writes,
entire mapped memory, allocation state and absence of native imports/RNG. The
ordering footprint must be exactly two four-byte spills with unchanged padding.
An independent tuple oracle covers effective fields after aliasing; C++ chrono
provides a separate oracle for valid Gregorian dates. Pure checks cover every
signed 16-bit day against zero and both signed extremes. Direct entry has a
finite 100-instruction budget; callbacks and actual callers are checked separately
from the unmasked direct ABI comparison.

The real simulation comparison retains strict original-against-original replay.
Semantic comparison permits only translated instruction accounting to differ;
PC, complete registers/temporaries, mapped bytes, allocator, import/callback
order and ordered RNG writes remain strict. Existing final morning/transcript
EOF and the original finite execution budget bound termination. The private
calendar edge stream adds ordering's raw arguments, first date, Boolean return,
complete registers and exact spills. Original streams are recorded only in
original mode and remain unchanged while consumed by both baseline and candidate.
The private ordering negative control toggles the first modern Boolean return;
it must be rejected at that first return boundary. Existing shift, difference
and RNG negative controls remain enabled. No perturbation is active in delivery.

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

Instrument the changed adapter and original ordering page explicitly; linked
platform bridges are not thereby claimed instrumented:

```sh
clang++ -std=c++23 -arch arm64 -O1 -g -ffp-contract=off \
  -fsanitize=undefined -fno-sanitize-recover=all -I native \
  native/date_ordering_checks.cpp native/calendar_adapter.cpp \
  .local/recompiled/page_10331.cpp \
  .local/aot-build/libcm_aot.a .local/aot-build/libcm_ui.a .local/aot-build/libcm_audio.a \
  -framework AppKit -framework Carbon -framework AVFAudio -framework Foundation \
  -lz -framework CoreFoundation -framework CoreServices -framework ApplicationServices \
  -o .local/date-ordering-reconstruction/ordering-ubsan
.local/date-ordering-reconstruction/ordering-ubsan .local/recomp-input
```

## Automated validation

The final adapter/generator candidate passed 5,646 direct original/modern cases,
156 independent Gregorian oracle cases and 196,608 pure signed-day checks.
The actual three direct callers (including the same-page call), indirect
`bctr` dispatch and callback marshalling passed separately. Complete registers,
temporaries, PC, exact ordered spills, entire mapped memory and allocator agree.
No native import or RNG call occurred in the direct operation.
Dispatch checks enter the exported caller PCs with synthetic date/register
inputs. Generator checks establish the hook structure; the separate real UI
action below exercises caller `0x10332c24` with actual game data.

The final suite passed **32/32 CTest groups** and **26 Python tests** (four
optional groups skipped by discovery and then executed explicitly). The optional
control-cursor, match-presentation, unsigned-division and local-temporary groups
passed 5, 6, 7 and 5 tests respectively. The local-temporary audit covered
1,953,157 instructions and 6,437,244 P-code operations; generated UBSAN comparison
completed 199,742 executions over 2,050 actual instruction patterns, including
23,247 matching memory failures. Focused UBSAN instrumented the changed adapter
and original pages and passed ordering (5,646 cases), difference (6,892 cases)
and shifts (16,380 cases plus the preserved bounded SHRT_MIN failure).
This does not claim instrumentation of every linked platform bridge or ASan
approval.

After the final `State` header change, dependency files and final link inputs
confirmed **1,965 linked State-dependent objects** were newer than that header.
The 22 other linked objects had no State dependency. The header SHA-256 is
`7c50b6ff0d0fd9df5f3ab8cbb0b576f430669dea7a974464f4269cee80c88f42`.

The previous immutable fixtures passed two strict original runs, two modern
runs, both historical/delivered baselines, and RNG/shift/difference controls.
Both fixtures honestly report zero independent-ordering calls; they establish
preservation of the previously delivered arithmetic, not coverage of this entry.

| Existing fixture | Modern add / subtract / difference / ordering | Original edge SHA-256 |
| --- | --- | --- |
| One day | 24,534 / 322,828 / 62,897 / 0 | `cd3eb269c6e3a6f95edf54b31483eae67e1dbb2da126009a2c385561a6c2f5a1` |
| Seven days | 99,279 / 893,137 / 367,792 / 0 | `35cb1d573eac87495e723edbf0242129be6cbe289acf754e3aa926690c61db93` |

Fixture files and original edge streams remained unchanged in every accepted
comparison. The additional original fixture below supplies actual ordering
coverage.
The day/week edge hashes also exactly match the previously delivered difference
report; no original fixture or stream was replaced.

## Exercised bounded original fixture

The new immutable fixture under
`.local/performance/date-ordering-capture-02/SeasonFixture` spans **20 July 2002
morning through 21 July morning**. It was captured entirely in original mode,
including the contract-expiry sorting action on 2,024 search results. The capture
log confirms original mode for all four calendar entries and zero semantic
counters. Native effects and callbacks are recorded by the existing tools; the
candidate is never captured as its own oracle.

Two strict original runs, two candidate runs, the delivered date-difference
baseline and the historical calendar baseline passed. Each candidate performed
13,336 additions, 51,567 subtractions, 25,546 differences and **38,051 independent
orderings**, with zero exceptional original shifts. The final phase was morning,
year 2002/day 201, RNG seed `0x00002c17`, cursor `0x108eb0bc`; every mode ended with
167,634,020 native calls and agreed on the strict state/transcript boundaries.
The capture recorded 114,502 ordered RNG state writes.

| Mode | Final translated instruction count |
| --- | --- |
| Entirely original | 132,028,129,381 |
| Historical shifts-only semantic baseline | 132,022,854,288 |
| Delivered shifts/difference semantic baseline | 132,020,020,923 |
| All four semantic entries (both runs) | 132,019,456,540 |

Only instruction accounting is permitted to differ. These counts are actual
execution accounting, not a matched time benchmark or speed claim.
The ordering perturbation failed at **calendar edge 24**, the first independent
ordering return, `0x10331860`, return byte 36, expected `1` / actual `0`. The shift
perturbation failed at edge 0; difference at edge 22; RNG failed on original
consumption order. All controls are private and inactive in delivery.

| Immutable new fixture file | SHA-256 |
| --- | --- |
| `capture.txt` | `cf456ebc61bdbbe0e3352231dc76c508ebb5afffa2c396dbe141085365629dc0` |
| `initial.snapshot` | `f06ed20d5a7088601edb628f195a75b8f359932d672b2c96f5c8c6770beb96d9` |
| `expected.snapshot` | `63bba904650e217b35ade05f11cafb0878c12bc970289a70361996454416507f` |
| `imports.gz` | `fcf8f0f4cbf14b521f7189e2aea882f0e8b966bc044f658bd34bbcdf1ac4b853` |

The separate original calendar-edge stream SHA-256 is
`69826af3920ad8ed66f9b014c1fdb0c7ebbc28f920c994b8e79d5cf400738301`.
Fixture and edge hashes were checked before/after comparison and remained
unchanged. Logs, transcripts, snapshots and streams remain private.

## Real candidate application observations

The isolated ARM64 candidate under `.local/performance/date-ordering-live-01`
loaded the previous 3 August 2002 afternoon checkpoint. The calendar and Rincon's
Cruzeiro profile/contract were inspected, then a normal private Save
`DateOrder-03Aug2002.sav` was created. After normal exit, reopening and Load
restored the same date and inspected fields. Its SHA-256 is
`327e5ad2f18a680cbb5a88bd3777523bdf262db3462d0785e10987a314077400`.

Normal holiday simulation advanced to 10 August afternoon (7 August night was
also observed). The calendar showed Fluminense away 1-1 on 3 August and Atletico
Paranaense home 2-0 on 7 August, with Ponte Preta still pending on 10 August.
Rincon moved to Flamengo during normal simulation before the first logged
independent ordering call. At 10 August his profile showed birth 14 August 1966,
age 35, 92 caps/19 international goals, condition 100% and zero official Flamengo
appearances. The contract showed full-time BRL 18,750/week, start 4 August 2002,
expiry 1 December 2003, BRL 825 assist bonus and zero manager-job release amount.
These are UI observations, not a matched original UI run or evidence attributing
the transfer to the new module.

The real ordering action was **Player & Staff Search / Search Player / View
Contract / click Contract Expiry heading**. The club squad criterion and opening
profile/contract panels alone did not exercise this entry. The advancing run
`app.NgHigM` recorded **38,422 modern ordering calls**, 120,452 additions,
1,773,285 subtractions and 416,935 differences, with zero exceptional original
shifts. All 32 logged ordering samples agreed with independent Python datetime
ordering on their valid-date domain. Caller return PC was `0x10332c28`.

| First year/day | Second year/day | Packed second tail | Boolean return |
| --- | --- | --- | --- |
| 2005/349 | 2004/352 | `01000014` | 0 |
| 2005/340 | 2005/349 | `00000000` | 1 |
| 2004/150 | 2005/355 | `00000000` | 1 |
| 2004/180 | 2004/180 | `00000000` | 0 |

Normal Save `DateOrder-10Aug2002.sav`, normal exit, reopening and Load restored
10 August afternoon, those calendar results and the inspected Flamengo profile
and contract. The post-advance save SHA-256 is
`3fc8a13c6a14878b3495e139da3bcfd2c3c2356237ac9d625b462df0c8217334`.
Save hashes, normal-exit logs and private observation records are
kept under `.local/date-ordering-reconstruction`; these private saves are not
published. No dates, saves, seeds or memory were patched to force advancement.

## Additional original capture procedure

Prepare a fresh private validation bundle with the existing tool (each name must
be unused), `--runtime` selecting a private writable copy and
`--season-reference-days 1`. Load the preserved `DateDiff-19Jul2002` checkpoint.
Use normal Continue Game to read the pending news and reach 20 July morning.
Confirm `capture.txt` reports recording before opening Player & Staff Search,
viewing contracts and clicking Contract Expiry. Then normal holiday simulation
may complete the next morning; the recorder stops at that boundary regardless
of later UI activity. No date, save or RNG patch is used. Compare that immutable
fixture with the command above; positive `modernOrderingCalls` and the isolated
ordering-return negative control are required as separate acceptance evidence.

The accepted capture uses one ARM64 game instance. The earlier automatic
three-day attempt completed before the sorting action and is excluded from
ordering acceptance. An old UI helper also briefly relaunched the prior bundle
before that earlier recording started; it exited normally with zero semantic
calls and is excluded. Controls were then bound explicitly to their target app.
The accepted modern gameplay and subsequent original capture ran individually.

## Installation and preservation

The validated source was installed with `python3 tools/install_native.py` at
`/Users/victordmor/Applications/CM 01-02 Native Prototype.app`. The installed
executable is ARM64, byte-identical to the validated source, and its bundled
program/segment bytes match the immutable private input. The signed isolated
candidate passed strict signature verification. UI gameplay and Save/Load were
validated in that isolated candidate; installation was verified by identity,
architecture, bundled program and preservation inventories, without writing to
personal runtime data.

| Artifact | Executable SHA-256 |
| --- | --- |
| Final source bundle | `b61765e722c6e5f7624550ac15524fcdb623f933e9306c3329644310a887f545` |
| Signed isolated candidate | `01395e5f2e345635f414390dc89d8816103ef68dc5af9490a4ee538fd46a227c` |
| Installed application | `b61765e722c6e5f7624550ac15524fcdb623f933e9306c3329644310a887f545` |
| Prior installed executable backup | `ca725a106fade1354e6e774364543b86dbf1eb9baa930c2ab55e096d72da0e61` |
| Signed original-capture bundle (diagnostic only) | `db44b053338628e08ebc5e925249fb32dcf9e16e9b2b87f51258fc73a62a2b49` |

The prior installed app was copied with Mac resource forks and extended
attributes to `.local/date-ordering-reconstruction/reference/installed.app`
before any candidate replacement; its ARM64 executable and original hash were
rechecked at delivery. It remains distinct from signed diagnostic copies.
Before, pre-install and after inventories agree exactly for **809 Game / 5
Runtime / 42 Runs files**, including content, resource forks, Finder metadata,
mode, flags, modification timestamps and captured extended attributes.

All 18 preexisting inputs/checkpoints remain unchanged, including the two old
simulation fixtures, previous July/February calendar checkpoints and the
July/August date-difference checkpoints. The immutable LFS save remains
`82c504f46ce54a723137dbf42c005d458a4d7b18a9abab4c9f92627aa94f08fb`.
Private builds, writable copies, new saves, logs and captures remain ignored;
ignore/LFS rules and the preexisting untracked difference handoff were preserved.
Normal Git delivery follows review of the exact source/documentation diff;
commit and fresh remote identity evidence are kept in the private delivery record.

## Limits

This proves one independent predicate's direct ABI and the recorded bounded
simulation, plus observed candidate UI progression and persistence. It does not
establish full-career equivalence, every comparison caller or inlined comparison,
historical hardware equivalence, or a time improvement. The larger date-record
routine containing caller `0x10717f50` remains generated. No annual UI crossing
was repeated for this increment; century/year limits have direct-test evidence.
The ascending-year difference quirk remains deliberately preserved and its
future correction remains deferred. No new feature, graphic change or additional
engine subsystem was reconstructed.
