# Semantic Mac date-to-month conversion

This increment targets only the independent Mac entry `0x10718360`. The five
previously delivered operations and the deliberately deferred ascending-year
leap behavior of date difference remain unchanged. The adjacent day conversion,
weekday, formatting, age, contracts, scheduling and other engine routines remain
generated. No optimization or timing improvement is claimed.

## Provenance and contract

The private PowerPC big-endian 32-bit export contains 54 instructions in
`[0x10718360,0x10718438)`, with its last return at `0x10718434`.
The pinned fingerprint, using the exact `calendar_audit.py` fields, is
`aa64b3cf2df3f13dba6c660b2a102a3d98c0492b60a5b63b603df3e12a747798`.
The index has 92 direct `bl` callers; none shares the entry's generated page.
No indexed external control-flow edge targets its interior. Indirect targets
remain a runtime coverage question. Static counts do not demonstrate execution coverage. Synthetic checks enter
actual callers `0x1002a12c`, `0x1002a138`, `0x100746e0` and `0x10089e80` with
controlled operands, separately from real application observations.

The private index SHA-256 remains
`2baf0e4af36613ba9ec3c7ceacdc8f663f24f65bc679d01c2c0e2d7e43d84fec`;
`program.json` remains
`50dda1a1470924b6ae5d7195ad44d042c18fe0cda254bdb6570e5ec176a962d0`.
The export contains 1,953,157 instructions, originating from PEF SHA-256
`91692f95df17cf851e7c1d6dbdecd0bd0a5809ce1937133e209aee859a6b516b`.
These exports and original payloads are private and absent from a clean public
clone. No original instruction listing is published with this report.

r3 points to a date. The first read is one byte at r3+4. Zero selects the common
table through TOC-0x3fa8; every nonzero byte, including 128 and 255, selects the
leap table through TOC-0x3fa4. The signed extension of this byte briefly affects
r0 and CR0, but table selection tests zero, rather than positivity. The year
at r3+2 is never read. The result therefore depends on the stored flag even when
it disagrees with Gregorian leap classification of the record's year.

With the original TOC `0x108bba90`, the pointer slots are `0x108b7ae8` (common)
and `0x108b7aec` (leap), resolving to `0x108e14ac` and `0x108e14c4` respectively.
Each table has twelve signed big-endian 16-bit thresholds: the last day before
the corresponding zero-based month. January's threshold is -1; February's is
30; March's is 58/common or 59/leap; December's is 333/common or 334/leap.
These tables and pointer slots are visible mutable mapped data, not constants
that the adapter may substitute. The adapter honors changed pointers, values
and TOC within the mapped, nonwrapping input domain.

The return is a signed 32-bit zero-based month: January=0 through December=11.
Search visits thresholds from December back to January and returns the first
month whose threshold is strictly less than the signed16 day. Equality advances
to the preceding month. With the supplied tables, all negative days return -1;
day zero returns January. There is no upper-year-bound check: days above the
last day of the year, up to signed16 maximum, still return December. Invalid
days are not normalized. For arbitrary mutable tables the same reverse search
contract applies, including nonmonotonic thresholds.

## Ordered accesses and ABI

After the flag and selected four-byte pointer, the first signed threshold is
loaded from table+22. Only then is the signed16 day loaded from r3. The day is
retained in r0 for the next five probes. If that group is exhausted, the second
group loads its first threshold and then reloads the day from r3. No year, padding, stack, scratch record
or initialization byte is read by this routine. There are no memory writes,
allocations, imports, callbacks or RNG consumption inside the entry.

The legacy loop groups six probes per CTR iteration. Initially r4=table+22,
r7=11, r6=22 and CTR=2. Each additional probe decrements r7. An exhausted group
subtracts 10 from r6, 12 from r4 and 1 from r7, then decrements CTR. An early
return in months 6..11 leaves r4=table+22, r6=22 and CTR=2; months 0..5 leave
r4=table+10, r6=12 and CTR=1. The sentinel path leaves r4=table-2, r6=2, r7=-1
and CTR=0. r5 is the last signed threshold inspected; r0 is the signed day.
r3 holds the result, and PC becomes the existing LR.

CR0 reflects the final signed threshold-versus-day comparison plus XER.SO.
Recognized months leave its less-than bit set. Sentinel results retain equality
or greater-than from the final threshold. XER, LR, TOC, context TOC, stack pointer,
all remaining architectural register bytes and generated temporary storage are
preserved. The adapter reproduces these effects independently of its semantic
month predicate. Callback marshalling restores its own register context and
is tested separately; it cannot conceal direct ABI clobbers.

The demonstrated domain consists of mapped date flag/day fields, the selected
pointer slot and the threshold locations actually probed, without address-space
wrapping or concurrent mutation. Diagnostics that record streams additionally
read the whole five-byte date and twelve-entry table. Unmapped pointers,
wrapping accesses and externally mutating observers are outside the full-state
compatibility claim. Memory failures must not be interpreted as equivalent
successful conversion. Alternate mapped tables and TOC have explicit checks;
no equivalence to every possible corrupt memory configuration is claimed.

## Architecture and dispatch

`calendar.hpp` provides a pure strict-threshold predicate and reverse month
search over a supplied table. `calendar_adapter.cpp` retains lazy ordered memory
access and the two-group legacy ABI effects. Normal production uses semantic
C++ without original month fallback. `State::date_month_calls` counts only
actual modern month dispatches. Private validation logs sample the first 32
calls with entry, year/day, effective flag/table, result and caller return PC.
No game UI selector or diagnostic option is added.

The generated destination label on page `0x10718` invokes the adapter. Same-page
`goto` emission, cross-page dispatch, indirect `bctr` and callback entry route
through that label. Fingerprints reject changed or incomplete bodies, including
missing entry/return/control-flow fragments. Neighboring routines and caller
bodies are not replaced.

| Replay mode | Shifts | Difference | Later | Before | Month |
| --- | --- | --- | --- | --- | --- |
| Default | original | original | original | original | original |
| `--calendar-baseline` | semantic | original | original | original | original |
| `--date-ordering-baseline` | semantic | semantic | original | original | original |
| `--date-before-baseline` | semantic | semantic | semantic | original | original |
| `--date-month-baseline` | semantic | semantic | semantic | semantic | original |
| `--modern` | semantic | semantic | semantic | semantic | semantic |

Historical option meanings are preserved. Original application capture uses
`calendar_original`, which also selects original month conversion. Diagnostic
flags, counters and optional read traces are not serialized into game saves or
private snapshots. No candidate is recaptured as its own oracle.

## Automatic verification and stream boundaries

The direct harness compares complete architectural registers, temporary bytes,
PC, every mapped memory byte, allocator state and absence of imports/RNG. An
optional direct memory read trace compares exact access order and widths; an
observer rejects every write/map/clear during conversion. Callback checks and
actual caller instructions with synthetic inputs are distinct evidence.
Gregorian chrono is an independent oracle only for valid dates with a consistent
flag. Signed invalid days, extreme unsigned years, mismatched flags, mutable
tables and alternate TOC use direct original/modern comparison. Pure tests cover
all signed16 days for both supplied tables. Direct execution uses a finite
160-instruction budget.

The existing unversioned and CMCAL002 calendar streams remain unchanged. Month
edges use a separate gzip stream with explicit `CMMON001` header. Each 17,205-byte
record includes entry, input pointer, caller LR, five input bytes, selected table
pointer, 24 table bytes, signed result, complete register bytes and final PC.
Result bytes occupy offsets 45..48. The isolated negative adds one to the first
modern result and must fail at month edge zero's return boundary. Closing the
stream clears the perturbation; malformed/truncated headers and unread trailing
edges are rejected. Other calendar and RNG controls remain enabled.

Immutable one-day, seven-day and ordering fixtures are replayed twice strictly
original, then with original edge recording, each historical baseline, delivered
five-operation baseline and two six-operation candidates. Only the previously
established translated instruction count difference is permitted. All state,
registers, temporaries, PC, mapped bytes, allocator, imports/callbacks and ordered
RNG remain strict. Transcript EOF and final morning terminate replay within the
original finite budget. Zero own calls must be reported as absent coverage.

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

`--edges-directory` retains new original calendar/month streams in a previously
unused private directory. Focused UBSAN compiles `calendar_adapter.cpp`, all five
direct harness variants and generated pages 10331/10332/10717/10718 using
`clang++ -std=c++23 -arch arm64 -O1 -g -ffp-contract=off -fsanitize=undefined
-fno-sanitize-recover=all -Inative`. The before harness adds `-DCM_CHECK_BEFORE=1`.
Link these objects before the normal `libcm_aot.a`, `libcm_ui.a`, `libcm_audio.a`,
`-lz` and the AppKit/Carbon/AVFAudio/Foundation/CoreFoundation/CoreServices/
ApplicationServices frameworks. This is focused instrumentation, not a claim
that every platform bridge is sanitized or that ASan passed.

## Acceptance evidence

The final direct harness passed 1,684 original/modern cases, 239 independent
Gregorian oracle cases and 131,072 pure signed-day checks. Complete registers,
temporaries, PC, mapped memory, allocator and exact ordered reads agreed, with
no writes/imports/RNG. Mutable tables, alternate TOC, four flag variants, unsigned
year extremes, signed invalid days, callbacks and synthetic caller dispatch
passed. Its separate original stream, first-return negative, trailing-edge and
truncated-header rejection checks passed; perturbation did not leak afterward.

The full suite passed 34/34 CTest groups in 97.61 seconds. Python discovery passed
28 tests with four optional groups skipped; the optional groups were executed
explicitly and passed 5 control, 6 presentation, 7 division and 5 temporary tests.
The complete temporary audit proved 1,953,157 instructions / 6,437,244 P-code
operations local. Generated UBSAN passed 199,742 differential executions over
2,050 patterns, including 23,247 matching memory failures. Focused UBSAN passed
month, before, later, difference and shifts on the final adapter/pages. An early
generated sanitizer run linked against the previous-layout archive while the
full rebuild was still running; it is excluded. The accepted run used the final
archive and passed.

The final dependency/link review found 1,967 State-dependent linked objects,
all newer than the finalized header, plus 22 other linked objects. Header SHA-256
is `ad7400ca529155244dfbb4f35efbf6ec1b4ef40af201ca2f9702587e85ccf4a3`.
No State header changed while these objects compiled.

### Bounded fixtures

The one-day immutable fixture passed two strict originals, original edge capture,
two six-operation candidates, all four historical/delivered baselines and the
exercised negative controls. Each candidate executed 8,807 modern month calls;
later and before had zero calls here and are not attributed coverage. The month
negative failed at `month edge 0 at 0x10718360 field=month return byte=48`,
expected 1 / actual 2. Its first original month edge reads year/day/flag
2002/38/0 at `0x1091edd4`, effective table `0x108e14ac`, result 1,
caller return `0x103868ec`. The week's first original edge uses 2002/146/0,
the same pointer/table/caller and result 4. These operands are directly retained
in the separate original month streams.

The seven-day fixture also passed two strict originals, original capture,
two candidates, all four baselines and all exercised negatives. Each candidate
executed 38,256 modern month calls; later/before remain unexercised here.
Its isolated month negative failed on edge 0 at return byte 48, expected 4 /
actual 5.

The original 20–21 July ordering fixture passed the same seven positive modes
and all six isolated controls (RNG, shifts, difference, later, before, month).
Each candidate executed 8,433 month, 38,051 later and
12,911 before calls. The month negative failed on edge 0 at return
byte 48, expected 6 / actual 7. Its first original month operand was
2002/200/0, result 6, caller return `0x103868ec`.
No additional original fixture was necessary because all three exercised month.

| Immutable fixture | Modern month calls per candidate | Original CMMON001 SHA-256 |
| --- | ---: | --- |
| One day | 8,807 | `0161ab1c87aa34562f70e54d3049736b27aab35e188303623e5ae6efad463608` |
| Seven days | 38,256 | `1cef11c1352e2f3a67186a6761212c03c08fda6182502dc6c2def6e0cd6f2e66` |
| Ordering | 8,433 | `adfd71c6417a9a69d00bc5a250aab1d45b33c70e23a37f7c8688d15cf930c6bf` |

The newly recorded CMCAL002 hashes are identical to the previous delivered
streams: one day `bdce201d4fc83a5c7e8eb9144421726f8bbbe76a1e12f1beaf424f756f5fd083`;
seven days `8ff9f77b64c342d5074136d350850cdf53b10eaf8b1987d9632a226e46974b12`.
The ordering CMCAL002 stream also retains
`a7849b48626ea92da01ed062417522323477196cd3a3a78c857357076f7b8954`.
Separate month recording has not changed the legacy stream encoding or effects.
All fixture files and original streams retained their hashes after replay.
Only translated instruction accounting differs; no PC/register/temporary,
memory/allocation/import/callback/RNG masks were added. Positive replay checks
consumed their applicable edge/transcript references to EOF and reached the bounded
final state. Two candidate reports agreed exactly per fixture.

### Real application persistence

A single private ARM64 candidate instance was used at a time, with Runtime and
Runs prepared by `tools/prepare_validation.py date-month-live-01 --runtime
.local/performance/date-before-live-01/Runtime`. The preserved
`DateBefore-27Aug2002.sav` checkpoint was hash-verified before copying/loading;
its original remains unchanged. Native menus supplied ordinary mouse events;
no date/save/RNG/memory manipulation forced coverage or advancement.

The original Load showed Victor Moraes / Cruzeiro on 27 August 2002 night.
The calendar retained the 3 August Fluminense 1:1, 7 August Atl. Paranaense 2:0,
10 August Ponte Preta 2:1, 17 August Internacional 2:1, 20 August Cerro Porteno 2:1
and 24 August Vitoria 1:2 results. Goias on 31 August, Coritiba on 6 September,
Santos on 8 September and Colo Colo on 11 September remained pending.
Luisao's profile showed birth 13 February 1981, age 21, one international cap,
26 (4) official appearances, one goal, two assists and average 7.20.
His full-time contract showed BRL 8,750/week, start N/D, bound Yes, expiry
24 December 2004, squad rotation and no bonuses/clauses.

Game Options -> Save Game created `DateMonth-27Aug2002.sav`; normal exit,
process absence, exact candidate reopening and original Load were observed.
The same calendar, player statistics and contract survived. Original holiday
and return confirmation advanced to the observed 28 August 2002 night, where
the holiday was stopped. The calendar/results and contract remained consistent;
Luisao's condition was now 63%. Game Options -> Save Game created
`DateMonth-28Aug2002.sav`. A second normal exit/process-absence/reopen/Load cycle
restored 28 August night, the same calendar, player statistics, condition and
contract. The final application exited normally and was absent afterward.
These are UI observations, separate from bounded original/modern equivalence.

The three private runs `app.KKhepw`, `app.uxAmoB` and `app.A2qmTo` reported
774, 870 and 85 modern month calls respectively, with all six operations in
modern mode and zero exceptional-original shifts. Their first 32 month samples
each passed independent Python Gregorian checks on valid flag-consistent inputs
(96/96). Examples include year/day/flag 2001/354/0 -> month 11 at return
`0x10486724`; 2002/238/0 -> month 7 at `0x103868ec`; 2002/239/0 -> month 7 at
`0x103eab80`; and 2002/358/0 -> month 11 at `0x10089e84`, using the effective
common table `0x108e14ac`. Samples encompass loading and normal date presentation;
opening the calendar alone is not claimed to produce every sampled call.
The production path has no original month fallback. Original selection remains
available only through the private diagnostic State flag.

Both new save hashes remained unchanged after their reloads:

| Private checkpoint | SHA-256 |
| --- | --- |
| DateMonth-27Aug2002 | `570a254eef09e7fe38e5a535256151f834a9b2f1fa4b1a4ef856353d81e310c5` |
| DateMonth-28Aug2002 | `2e536421c2e2972a072c9538293b5d967b0846f3c1339bd40e6a5b63dc22a2af` |

The existing File -> Save native-menu limitation remains outside this increment;
the original Game Options save flow passed. No new full-season replay or timing
measurement is claimed.

### Protected installation

The final production ARM64 application is installed at
`~/Applications/CM 01-02 Native Prototype.app` using `tools/install_native.py`.
Its executable and Info.plist match the built source; all four bundled original
program files match the protected input hashes. The production plist has no
private validation/capture settings. The signed private candidate passes
`codesign --verify --deep --strict` and retains its separate identity.

| Artifact | Executable SHA-256 |
| --- | --- |
| Final source | `caf91df0b7d4313937711725022c38a59949bebe66a5d2c07e934efae1058756` |
| Signed private candidate | `f3e17857f83cf0922028847df8c2718b5f6fb9df83ea4d82f0f5b86e5a23de5d` |
| Final installed | `caf91df0b7d4313937711725022c38a59949bebe66a5d2c07e934efae1058756` |
| New executable reference backup | `d2784cfde149a28d8e8c1d067e26ac90cfff482581a36858a0d215a7790194a1` |

Before/preinstall/after inventories agree exactly for all 809 Game / 5 Runtime /
43 Runs files, including content, resource forks, Finder metadata, all inventoried
extended attributes, modes, modification times and flags. The private executable
backup retains its reference inventory. Protected inputs, all three original
fixtures, historical streams/checkpoints and both untracked handoff prompts
retain their hashes. Logs, streams, save files, snapshots, builds and exports
remain ignored private artifacts; ignore/LFS rules were not changed.

A new private executable backup of the current installed ARM64 reference was
created before changes at `.local/date-month-reconstruction/reference/installed.app`
using `ditto --rsrc --extattr`. Its executable SHA-256 is
`d2784cfde149a28d8e8c1d067e26ac90cfff482581a36858a0d215a7790194a1`, matching
source and installed references at task start. This differs from the older
backup retained for the before increment. Initial personal inventory contains
809 Game, 5 Runtime and 43 Runs files; historical counts are not invariants.
The immutable LFS save remains
`82c504f46ce54a723137dbf42c005d458a4d7b18a9abab4c9f92627aa94f08fb`.
Both preexisting untracked feature prompts are preserved independently.
