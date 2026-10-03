# Semantic Mac day-of-month conversion

This increment addresses the independent Mac entry `0x107182f0`. It preserves
addition, subtraction, difference, later, before and month, including the
[deferred ascending-year leap behavior](native-date-difference-reconstruction.md#deferred-improvement--2026-10-02).
Weekday, formatting, ternary comparisons, age, contracts, transfers, competition
and player processing remain outside this increment. No speed improvement is claimed.

Status: contract, direct equivalence, bounded fixtures, full tests and UBSAN
passed. The ARM64 app passed original Save/normal exit/reopen/Load on both
sides of the August-to-September boundary. The validated production build is installed.

## Private provenance and contract

The original export contains 24 instructions in `[0x107182f0,0x10718354)`,
with its last return at `0x10718350`. There is an internal gap at `0x1071833c`
and no exported instruction between the exclusive end and `0x10718360`.
The fingerprint using the exact calendar audit fields is
`a496c80a33e42a50319536aa245bb4a3a2d2fd64363d1a5e13f277b89ada57a5`.
The index contains 20 direct `bl` callers, none on the same generated page,
and no indexed external flow to the body's interior. Static counts do not
prove actual execution or exhaust indirect targets. Synthetic caller checks
use `0x10097f84`, `0x10097fa0`, `0x101450cc`, `0x10363940`, `0x1036395c`
and `0x103a76f8`, separately from real simulation and UI evidence.

The private instruction index SHA-256 is
`2baf0e4af36613ba9ec3c7ceacdc8f663f24f65bc679d01c2c0e2d7e43d84fec`;
program metadata SHA-256 is
`50dda1a1470924b6ae5d7195ad44d042c18fe0cda254bdb6570e5ec176a962d0`.
The 1,953,157-instruction export originates from PEF
`91692f95df17cf851e7c1d6dbdecd0bd0a5809ce1937133e209aee859a6b516b`.
Original listings, segments, saves, snapshots, transcripts and logs remain
private; a public clone alone cannot reproduce original comparisons.

r3 points to the five-byte date. Its first read is the byte at date+4, followed
by the selected four-byte pointer through TOC-0x3fa8 for zero and TOC-0x3fa4 for
any nonzero byte, including 128 and 255. The signed extension of the flag changes
r0/CR0 initially; selection tests zero. The year at date+2 is never read by the
routine. A flag inconsistent with the year therefore selects its own table.

At TOC `0x108bba90`, slots `0x108b7ae8` and `0x108b7aec` select common table
`0x108e14ac` and leap table `0x108e14c4`. Each contains twelve mutable signed
big-endian 16-bit thresholds: the last day before each zero-based month.
January is -1, February 30, March 58/common or 59/leap, and December 333/common
or 334/leap. The adapter reads actual mapped pointers and table data, including
alternate mapped TOCs, relocated tables and nonmonotonic thresholds.

Search visits December through January, accepting the first threshold strictly
less than the signed16 day. Each probe reads the threshold, then reloads the
day. On recognition it reads the selected threshold again, subtracts it from
the last loaded day and narrows to signed16, sign-extended into r3. Valid dates
with a consistent flag return one-based day-of-month: January 1 returns 1.
Nothing recognized returns signed32 -1. With supplied tables, all negative days
return -1, and out-of-year positive days continue subtracting December's threshold.
There is no normalization, validation or clamp. For mutated extreme thresholds,
positive differences above 32767 wrap modulo 65536 into signed16; the full
unnarrowed difference remains in r0.

## Ordered effects and direct ABI

After flag/pointer reads, the loop starts with r5=table, r4=table+22, r7=11 and
CTR=12. Each unsuccessful probe subtracts two from r4, one from r7 and one from
CTR. A recognized month m leaves r4=table+2*m, r7=m and CTR=m+1. r5 retains
the base pointer, r6 the signed day, r0 the full day-minus-reloaded-threshold
32-bit value, and r3 the signed16 result. Sentinel completion leaves r4=table-2,
r7=-1, CTR=0, r0 the final January probe and r6 its loaded day.

CR0 reflects the last signed threshold/day comparison plus XER.SO; recognition
sets less-than. Subtraction and narrowing do not change CR0. PC becomes LR.
XER, LR, TOC, context TOC, stack pointer, all other architectural bytes and
translation temporary storage are preserved. There are no stack spills,
frames, scratch/init accesses, memory writes, allocations, imports, callbacks
or RNG consumption within the entry. Callback marshalling's own stack writes
and context restoration are checked separately from direct entry equivalence.

The demonstrated successful domain requires mapped flag/day, selected pointer
slot and actually probed/reloaded thresholds, without address wrapping or
concurrent mutation. Diagnostic samples and edge streams additionally read the
whole date and table. Unmapped accesses, address wrapping and external mutation
between reads are outside the full-state equivalence claim. No universal
corrupt-memory or failure-state equivalence is claimed.

## Architecture and integration

`calendar.hpp` contains pure reverse recognition and signed16 subtraction.
`calendar_adapter.cpp` implements the independent twelve-probe loop and legacy
register effects, retaining lazy loads and selected-threshold reload. Production
executes semantic C++ without original day-of-month fallback. Its own
`State::date_day_of_month_calls` counter and first 32 private UI samples identify
entry, pointer, year/day, effective flag/table, result and caller return.
Read tracing remains disabled in normal production; no UI selector is added.

The generator hooks the destination label on page `0x10718`, so same-page gotos,
inter-page dispatch, indirect dispatch and callbacks reach the adapter. The
complete body audit rejects changed and incomplete fragments, including omitted
entry, return, reload and loop control. Neighboring bodies and callers remain
generated. The compilation manifest lists all seven semantic entries.

| Mode | Shifts | Difference | Later | Before | Month | Day-of-month |
| --- | --- | --- | --- | --- | --- | --- |
| Default reference | original | original | original | original | original | original |
| `--calendar-baseline` | semantic | original | original | original | original | original |
| `--date-ordering-baseline` | semantic | semantic | original | original | original | original |
| `--date-before-baseline` | semantic | semantic | semantic | original | original | original |
| `--date-month-baseline` | semantic | semantic | semantic | semantic | original | original |
| `--date-day-of-month-baseline` | semantic | semantic | semantic | semantic | semantic | original |
| `--modern` | semantic | semantic | semantic | semantic | semantic | semantic |

All historical meanings are preserved. Entirely original application capture
uses `calendar_original`, which also selects original day-of-month. Diagnostic
flags/counters remain outside snapshot and save serialization.

## Streams and verification protocol

`CMDOM001` is a separate versioned gzip stream. Historical `CMCAL002` and
`CMMON001` encodings are unchanged. Each 17,205-byte record contains entry,
input pointer, caller LR, five date bytes, effective table pointer, 24 threshold
bytes, signed32 result, all architectural register bytes and final PC. Result
bytes occupy offsets 45..48. An isolated diagnostic increments only the first
modern return; it must fail at day-of-month edge zero's return boundary. Closing
the stream clears perturbation. Direct tests require rejection of unread edges,
truncated header/body and damaged gzip checksum, then verify ordinary execution.

Direct tests use a finite 160-instruction budget and compare all architectural
bytes, temporary bytes, PC, every mapped memory byte and allocator state. An
optional read trace compares complete order/address/width, including each day
reload and selected threshold reload; a write observer rejects writes/map/clear.
Independent chrono expectations apply only to valid flag-consistent dates.
Unsigned year extremes, signed invalid days, flag inconsistencies, alternate
TOC/pointers and nonmonotonic/extreme thresholds use original comparison.
All signed16 days in both supplied tables have pure-domain checks.

For each immutable fixture, validation requires two strict originals, original
edge recording, every historical baseline, the delivered six-operation baseline,
two seven-operation candidates and each exercised isolated negative. Original
recapture is byte-compared with the preserved historical streams via
`--historical-edges-directory`. Only translated instruction accounting may differ;
PC, all registers/temporaries, mapped state, allocator, native calls/callbacks
and RNG remain strict. Original transcript EOF and final morning terminate
within the finite original budget. Zero own calls are reported as absent coverage.

## Reproduction

```sh
python3 tools/compile_recomp.py .local/recomp-input .local/recompiled \
  --root 0x10773710 --all-analyzed
cmake -S native -B .local/aot-build -DCMAKE_BUILD_TYPE=Release \
  -DCM_GENERATED_DIR="$PWD/.local/recompiled" -DCM_ORACLE_DIR="$PWD/.local/recomp-input"
cmake --build .local/aot-build --clean-first -j 4
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

`--edges-directory` must name a new private directory. To require byte-identity
with delivered streams, supply the corresponding preserved
`.local/date-month-reconstruction/{day,week,ordering}-edges-01` directory through
`--historical-edges-directory`.

Focused UBSAN compiles the adapter, all six direct harness variants, and generated
pages 10331/10332/10717/10718 with `clang++ -std=c++23 -arch arm64 -O1 -g
-ffp-contract=off -fsanitize=undefined -fno-sanitize-recover=all -Inative -c`.
The before variant adds `-DCM_CHECK_BEFORE=1`. Link the instrumented harness,
adapter and page objects before the finalized `libcm_aot.a`, `libcm_ui.a`,
`libcm_audio.a`, `-lz`, and AppKit/Carbon/AVFAudio/Foundation/CoreFoundation/
CoreServices/ApplicationServices frameworks, then run with `.local/recomp-input`.
This instruments changed semantic/generated boundaries, not every platform bridge.
State layout must be stable throughout the full rebuild and sanitizer linking.
The focused commands can be repeated after that clean full rebuild:

```sh
python3 - <<'UBSAN'
from pathlib import Path
import subprocess
out = Path('.local/date-day-of-month-ubsan')
out.mkdir(parents=True, exist_ok=True)
flags = ['clang++', '-std=c++23', '-arch', 'arm64', '-O1', '-g',
         '-ffp-contract=off', '-fsanitize=undefined',
         '-fno-sanitize-recover=all', '-Inative']
sources = [('day', 'native/date_day_of_month_checks.cpp'),
           ('adapter', 'native/calendar_adapter.cpp'),
           ('month', 'native/date_month_checks.cpp'),
           ('later', 'native/date_ordering_checks.cpp'),
           ('before', 'native/date_ordering_checks.cpp'),
           ('difference', 'native/date_difference_checks.cpp'),
           ('shifts', 'native/calendar_checks.cpp')]
pages = ['page_10331', 'page_10332', 'page_10717', 'page_10718']
sources += [(name, f'.local/recompiled/{name}.cpp') for name in pages]
for name, source in sources:
    define = ['-DCM_CHECK_BEFORE=1'] if name == 'before' else []
    subprocess.run([*flags, *define, '-c', source,
                    '-o', str(out / (name + '.o'))], check=True)
libs = ['.local/aot-build/libcm_aot.a', '.local/aot-build/libcm_ui.a',
        '.local/aot-build/libcm_audio.a', '-lz']
for framework in ['AppKit', 'Carbon', 'AVFAudio', 'Foundation',
                  'CoreFoundation', 'CoreServices', 'ApplicationServices']:
    libs += ['-framework', framework]
for name in ['day', 'month', 'before', 'later', 'difference', 'shifts']:
    objects = [str(out / (unit + '.o')) for unit in [name, 'adapter', *pages]]
    binary = out / name
    subprocess.run([*flags, *objects, *libs, '-o', str(binary)], check=True)
    subprocess.run([str(binary), '.local/recomp-input'], check=True)
UBSAN
```

No AddressSanitizer approval is claimed; historical local ASan limitations do
not establish verification of this increment.

## Acceptance status

The direct harness passed 3,173 original/modern cases and 1,700 independent
Gregorian oracle cases, plus 131,072 pure signed-day checks. It also passed the
separate sparse-mapping case, callback restoration and synthetic actual callers
and indirect dispatch. Complete ABI/temporaries/PC/mapped bytes/allocator and
ordered reads agree; writes/imports/RNG are absent. Mutable nonmonotonic and
extreme thresholds, relocated pointers/TOC and nonbinary/inconsistent flags
passed. Own stream checks rejected the first changed return, unread edges,
truncated header/body and damaged checksum; ordinary execution after the
perturbation passed.
A preliminary harness run treated the expected gzip close error in the damaged
checksum control as an uncaught test failure. Its cleanup was corrected; the
accepted rerun passed. Production source/layout were unchanged by that repair.

Python discovery ran 29 tests: 25 passed and four optional groups were skipped. Those groups
also ran explicitly against the private index: 5 control, 6 presentation,
7 division and 5 temporary tests passed. The full temporary audit covered
1,953,157 instructions / 6,437,244 P-code operations. Generated UBSAN passed
199,742 differential executions across 2,050 patterns, including 23,247 matched
memory failures. The full CTest suite passed 35/35 groups in 98.58 seconds. Focused UBSAN
passed day-of-month, month, before, later, difference and shifts against the
finalized archive/layout. All three bounded fixtures passed as recorded below.

The clean full rebuild compiled 1,968 State-dependent linked objects and 22 other
linked objects. No stale State-dependent object was found; finalized header
SHA-256 is `ccc977629f036d21ab5a20be9ef2fa25f20989eb9b207c176f210167146906a1`.
The production headers/source remained unchanged throughout that rebuild.

The private candidate is prepared at `.local/performance/date-day-of-month-live-01`,
using the verified August 28 checkpoint from the month candidate's copied Runtime.
The source executable SHA-256 is
`1fa57f3f11d3c911f4d452ed5abf58a55112ed6b71defafb6439428f0fd234dc`;
the signed ARM64 candidate is
`6e5ccab6215b16680874424bd8ad51cca2c4642456047ab1f0f01ce3b3deb964`.
Its signature verifies. After the user unlocked the Mac, the exact candidate
completed the real-app validation below. Source and candidate executables
remained unchanged throughout automatic and UI checks.
The current installed reference has been copied with `ditto --rsrc --extattr`
to `.local/date-day-of-month-reconstruction/reference/installed.app`; its ARM64
executable matches the initial source/installed reference SHA-256
`caf91df0b7d4313937711725022c38a59949bebe66a5d2c07e934efae1058756`.
The initial personal inventory contains 809 Game, 5 Runtime and 43 Runs files,
including hashes, resource forks, Finder metadata, extended attributes, modes,
modification times and flags. These counts are current observations, not invariants.

### Accepted one-day fixture

The immutable one-day fixture passed two strict originals, entirely original
edge capture, two seven-operation candidates and all five historical/delivered
baselines. Each candidate executed 8,639 modern day-of-month calls, alongside
8,807 month calls. Later/before each had zero calls and are not attributed
coverage here. Both candidate reports agree exactly, ending at 2002 day 39,
morning, with 41,620,536 native calls. Only instruction accounting differs.

The first original day-of-month edge contains 2002/day 38/flag 0 at
`0x1091edd4`, effective common table `0x108e14ac`, result 8 and caller return
`0x10486e68`. The isolated control was rejected at
`day-of-month edge 0 at 0x107182f0 field=day-of-month return byte=48`,
expected 8 / actual 9. All exercised RNG/shifts/difference/month controls also
passed. Original CMDOM001 SHA-256 is
`09ac0ffd9445ddcfa2793766c6d7902decac235ac0180a02219a47817a44cf63`.

Recaptured original CMCAL002 retained
`bdce201d4fc83a5c7e8eb9144421726f8bbbe76a1e12f1beaf424f756f5fd083`;
CMMON001 retained
`0161ab1c87aa34562f70e54d3049736b27aab35e188303623e5ae6efad463608`.
The fixture and edge files remained unchanged across consumers. Transcript and
applicable stream EOF, final state/ABI/memory/allocator/import/callback/RNG
checks all remained enabled. The seven-day and ordering results below are also accepted.

### Accepted seven-day fixture

The immutable seven-day fixture passed the same two strict originals, entirely
original edge capture, two candidates, all five baselines and all exercised
negative controls. Each candidate executed 14,209 modern day-of-month calls and
38,256 month calls; later/before remain unexercised here. Candidate reports agree
exactly and reached 2002 day 153, morning, with 238,517,060 native calls.

The first original own edge uses 2002/day 146/flag 0, pointer `0x1091edd4`,
table `0x108e14ac`, result 27 and caller return `0x10486e68`. The isolated own
negative was rejected at edge zero's return byte 48, expected 27 / actual 28.
CMDOM001 SHA-256 is
`e106535c0c739ead6bb714b468a1dc6d4b3f60358e1a0358fbbcf807556713ba`.
Historical CMCAL002 and CMMON001 recapture retained respectively
`8ff9f77b64c342d5074136d350850cdf53b10eaf8b1987d9632a226e46974b12`
and `1cef11c1352e2f3a67186a6761212c03c08fda6182502dc6c2def6e0cd6f2e66`.
All fixture/edge identities, EOF and strict state/effect boundaries remained intact.

### Accepted ordering fixture

The original 20–21 July fixture passed the same eight positive modes and all
seven isolated negatives (RNG, shifts, difference, later, before, month and
day-of-month). Each candidate executed 8,733 modern day-of-month calls, 8,433
month calls, 38,051 later calls and 12,911 before calls. Both reports agree
exactly at 2002 day 201, morning, with 167,634,020 native calls.

Its first original own edge uses 2002/day 200/flag 0, pointer `0x1091edd4`,
table `0x108e14ac`, result 20 and caller return `0x103fa334`. The own negative
was rejected at edge zero's return byte 48, expected 20 / actual 21. CMDOM001
SHA-256 is
`9157b1525021b92dc0e81b92cd297a9d2b47228e4ae505dc63fc4cffc89f5220`.
Historical CMCAL002 and CMMON001 remain byte-identical, respectively
`a7849b48626ea92da01ed062417522323477196cd3a3a78c857357076f7b8954`
and `adfd71c6417a9a69d00bc5a250aab1d45b33c70e23a37f7c8688d15cf930c6bf`.
The original fixture and all three streams retain their identities. No state,
PC, register, temporary, memory, allocator, import/callback or RNG mask was added.
The only permitted difference remains translated instruction accounting.

| Immutable fixture | Own modern calls per candidate | First own negative |
| --- | ---: | --- |
| One day | 8,639 | edge 0, return byte 48: 8 -> 9 |
| Seven days | 14,209 | edge 0, return byte 48: 27 -> 28 |
| Ordering | 8,733 | edge 0, return byte 48: 20 -> 21 |

All three exercise the new entry, so an additional original capture is unnecessary.
These are bounded real simulation comparisons, not real-app Save/Load evidence.

### Real ARM64 app and month-boundary persistence

Three fresh launches used the exact signed candidate with its isolated Runtime
and Runs, with a single game process at a time. The preserved source checkpoint
`DateMonth-28Aug2002.sav` retains SHA-256
`2e536421c2e2972a072c9538293b5d967b0846f3c1339bd40e6a5b63dc22a2af`.
Original Load showed Victor Moraes / Cruzeiro on Wednesday, 28 August 2002,
night (`28.8.02 NTE`). No date, save, memory or RNG editing was used.

The calendar showed Fluminense 1:1 on August 3, Atl. Paranaense 2:0 on August 7,
Ponte Preta 2:1 on August 10, Internacional 2:1 on August 17, Cerro Porteno 2:1
on August 20 and Vitoria 1:2 on August 24. Goias on August 31 and Coritiba,
Santos and Colo Colo on September 6/8/11 were pending. Luisao's profile showed
birth February 13, 1981, age 21, one international cap, condition 63%,
26 (4) official appearances, one goal, two assists and average 7.20. His
full-time contract showed BRL 8,750 per week, start N/D, bound Yes,
expiry December 24, 2004, squad rotation, no bonuses and no clauses.
These are observed UI facts, not semantic reconstruction of player/contract code.

Original Game Options -> Save Game created `DateDay-28Aug2002.sav`, SHA-256
`22a6263cd149251c58c74618d4cf73831218e74123dcee79edd4debbc2c339a3`.
Normal original exit and process absence were verified before reopening the
exact candidate. Load preserved the observed date/phase, club, calendar results
and pending fixtures, player statistics/condition and contract.

Normal original holidays then advanced through August 31 afternoon's Goias
fixture. The first observed September interval was Sunday, September 1, 2002,
afternoon (`1.9.02 TAR`); holidays were immediately stopped through the ordinary
return-from-holiday confirmation. This date was reached by gameplay, not forced.
The calendar now showed the August 31 Goias result 0:3, Cruzeiro 16th in the
national competition, and the same pending September fixtures. Earlier observed
results remained unchanged. Luisao's condition was now 80%; his observed birth,
age, cap, official statistics and contract retained the values above.

Original Save created `DateDay-01Sep2002.sav`, SHA-256
`ce48d8af426557e18976292e3bd22172241b1d61d8a575108a6f862b3583463f`.
A second normal exit/process-absence/exact-reopen/Load cycle preserved September
1 afternoon, Cruzeiro, the 0:3 result and pending fixtures, Luisao's 80% condition,
statistics and contract. The final session also exited normally, with no game
process remaining. CUA bindings and accessibility indices were refreshed after
reopening. Game canvas clicks used the existing validation coordinate helper,
which queues an unmodified original mouse event. Native File -> Save remains
outside this scope (`Native menu selections require original MenuSelect dispatch`).

The three runs recorded respectively **5 / 10,735 / 3** own modern day-of-month
calls, and all summaries selected modern day-of-month with zero exceptional
original calls. Their **40** available own samples (5 / 32 / 3) were independently
checked using Python calendar leap identification and `datetime.date` arithmetic,
restricted to valid dates with consistent flags and effective original tables.
For example entry `0x107182f0`, pointer `0x1091edd4`, 2002/day 239/flag 0,
table `0x108e14ac`, returned 28 to `0x10486e68`; Luisao's birth pointer
`0x216d2b54`, 1981/day 43/flag 0, returned 13 to `0x10713c70` and `0x103abb24`.
These actual UI calls and the destination hook establish normal semantic dispatch;
opening the calendar alone is not attributed coverage. No original day-of-month
fallback exists in normal production. Diagnostic sampling reads extra input/table
bytes and is enabled only in the private validation candidate.

### Protected installation

The existing installer installed the validated production build after all UI
checks. All four executable identities were rechecked as ARM64:

| Artifact | Executable SHA-256 |
| --- | --- |
| Final source | `1fa57f3f11d3c911f4d452ed5abf58a55112ed6b71defafb6439428f0fd234dc` |
| Signed private candidate | `6e5ccab6215b16680874424bd8ad51cca2c4642456047ab1f0f01ce3b3deb964` |
| Installed production | `1fa57f3f11d3c911f4d452ed5abf58a55112ed6b71defafb6439428f0fd234dc` |
| New retained reference backup | `caf91df0b7d4313937711725022c38a59949bebe66a5d2c07e934efae1058756` |

Installed program metadata and all three segment files match the private inputs.
The installed plist has the production bundle identifier and no private validation
environment settings. The backup's full content/metadata inventory is unchanged.
Before, preinstall and after inventories agree exactly for the observed 809 Game /
5 Runtime / 43 Runs files, including contents, resource forks, Finder metadata,
all inventoried extended attributes, modes, modification times and flags.
All 645 protected file identities, including historical saves/streams, exports,
immutable LFS save and three preexisting prompts, remain unchanged. The new saves,
logs, captures and builds stay private and ignored; ignore/LFS rules are unchanged.

This delivers the independent day-of-month conversion. It does not establish
full-engine reconstruction, a full-career replay or a speed improvement. The
ascending-year leap difference remains deliberately deferred.
