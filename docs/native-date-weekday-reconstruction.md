# Semantic Mac weekday conversion

This increment addresses only independent entry `0x10715b20`. The seven
delivered operations and the [deferred ascending-year leap difference](native-date-difference-reconstruction.md#deferred-improvement--2026-10-02)
remain unchanged. Formatting/localized weekday names, date adjustment
`0x10715a30`, date construction, ternary comparisons, age, contracts, scheduling,
competitions, players and match processing remain generated. No speed gain is claimed.

Status: delivered on October 3, 2026 with direct/bounded equivalence, real ARM64
weekday-changing persistence and protected installation acceptance.

## Provenance and boundary

The read-only private index contains 40 contiguous instructions in
`[0x10715b20,0x10715bc0)`. Returns at `0x10715bac`, `0x10715bb4` and
`0x10715bbc` are all part of the audit. The exact address/assembly/P-code/flows/
fallthrough fingerprint is
`faab9fb0f48231766f5e9b0595dd109a2be7639b984ec830b14021ad64892c43`.
The 63 indexed direct callers include same-page `0x10715a54` and cross-page
`0x1006655c`, `0x10067fd4`, `0x101197e4` and `0x104946c4`. No indexed external
flow enters the interior; the preceding routine returns before an unexported
four-byte gap. Static analysis does not exhaust indirect targets or prove runtime coverage.

The private index SHA-256 is
`2baf0e4af36613ba9ec3c7ceacdc8f663f24f65bc679d01c2c0e2d7e43d84fec`;
program metadata is
`50dda1a1470924b6ae5d7195ad44d042c18fe0cda254bdb6570e5ec176a962d0`.
The 1,953,157-instruction export originates from PEF
`91692f95df17cf851e7c1d6dbdecd0bd0a5809ce1937133e209aee859a6b516b`.
Original listings/payloads and all saves, snapshots, transcripts, logs and builds
remain private. A public clone cannot reproduce original execution without them.

## Semantic contract and Gregorian domain

r3 points to the five-byte date. Exactly three reads occur, in order:
unsigned big-endian 16-bit year at date+2, byte flag at date+4, signed big-endian
16-bit zero-based day at date+0. No field is reloaded. TOC, tables, stack,
padding, scratch and initialization are not accessed. All nonzero flags,
including 128/255 and flags inconsistent with the year, subtract one day.

Let `n = year - 1600`, with every division below truncating toward zero:

```text
total = 365*n + n/4 - n/100 + n/400 + signed_day - (flag != 0)
remainder = total % 7
return remainder > 0 ? remainder-1 : remainder == 0 ? 6 : remainder
```

The valid normal enumeration is Monday=0, Tuesday=1, Wednesday=2, Thursday=3,
Friday=4, Saturday=5, Sunday=6. For example September 1, 2002 (day 243, flag 0)
returns 6 and the next day returns 0. February/March and century boundaries
retain the stored flag's role. No validation, clamp or date normalization is added.

Gregorian correspondence is established for valid dates in years 1601..65535
with a flag matching Gregorian leap classification. The independent wide-year
oracle uses civil ordinal arithmetic; chrono additionally checks its supported
year range. The epoch year has a deliberate exception: January 1, 1600 with
flag 1 returns -1, while January 2 returns Sunday=6. Before the epoch, truncating
year corrections and negative remainders need not agree with proleptic Gregorian
weekday. Year 1599/day 0/flag 0 returns -1; year 0/day 1/flag 0 returns -6.
Negative/inconsistent/out-of-year inputs retain the original result, including
-6..-1. Remainder zero returns 6 even for a negative total divisible by seven.

All input totals fit signed32 (`n` is -1600..63935); signed16 narrowing of the
remainder cannot overflow because it is -6..6. No saturation is inferred from
these bounds. Arithmetic shifts and signed64 products used for ABI residue are
defined in C++23 and fit their types.

## Direct ABI and ordered effects

The year multiplier's signed high word is `h = (0x51eb851f*n) >> 32`.
The original uses arithmetic shifts, sign corrections and `addze` to implement
truncating `/4`, `/100`, `/400`. A negative `/4` shift sets carry if low bits
were discarded; `addze` adds that carry and replaces CA with unsigned addition
carry. Its later signed division by seven overwrites CA independently.

Final r4 is truncating `n/400`; r5 is the sign of `h >> 5`; r6 is `h >> 7`;
r7 is the sign of r6; r8 retains total. r0 retains the signed remainder and r3
the semantic return. The positive path records CR0 against the remainder before
subtracting one from r3. The nonpositive path sign-extends the remainder into
r0 and records CR0 again; zero then replaces only r3 with 6. These distinct
exits therefore do not record CR0 against the returned weekday.

The division-by-seven intermediate is
`v = ((-1840700269 * total) >> 32) + total`; final XER.CA is true exactly when
v is negative and its low two bits are nonzero. CR0 preserves its upper byte
bits, sets LT/GT/EQ for the signed remainder and copies XER.SO. Other CR fields
and XER bytes, LR, CTR, TOC/context TOC, stack pointer, other registers and all
generated temporaries remain unchanged. PC becomes LR.

No writes, mapping, allocation, imports, callbacks or RNG effects occur inside
the entry. Entire mapped bytes include stack and RNG data in direct comparisons.
Callback marshalling's stack writes/context restoration are separate checks.
The supported domain requires the three mapped fields without address wrapping
or concurrent mutation. Failed/unmapped reads and mutation between loads are
outside the successful full-state equivalence claim. Diagnostic streams/samples
additionally copy the five input bytes; this is not the production footprint.

## Architecture and diagnostic modes

`calendar.hpp` expresses the epoch arithmetic and negative-remainder contract.
`calendar_adapter.cpp` reads the three fields lazily in original order, computes
the semantic result and preserves legacy residue independently. Production
has no original weekday fallback. The generated destination on page `0x10715`
is hooked, covering the real same-page goto, inter-page dispatch, indirect entry
and callbacks. Neighboring routines/callers are not replaced. The manifest lists
all eight entries. Optional read tracing remains disabled in production.

`State::date_weekday_calls` counts only this operation. Private UI samples report
entry, pointer, year/day, flag, result and caller LR. Flags/counters are excluded
from snapshot and game-save serialization. Entirely original app capture selects
weekday through the existing `calendar_original` flag.

| Mode | Previous semantic operations | Weekday |
| --- | --- | --- |
| Default reference | none | original |
| `--calendar-baseline` | add/subtract | original |
| `--date-ordering-baseline` | add/subtract/difference | original |
| `--date-before-baseline` | add/subtract/difference/later | original |
| `--date-month-baseline` | add/subtract/difference/later/before | original |
| `--date-day-of-month-baseline` | preceding five plus month | original |
| `--date-weekday-baseline` | all seven including day-of-month | original |
| `--modern` | all seven | semantic |

Historical meanings remain intact. Recording in modern mode is rejected by both
CLI selection and the weekday stream boundary.

## Independent stream and validation

`CMWKD001` is a new private gzip stream; CMCAL002/CMMON001/CMDOM001 are unchanged.
Each record contains entry (4), pointer (4), caller LR (4), input date (5),
signed32 result (4), all architectural register bytes and final PC (4).
Return bytes occupy 17..20. The isolated negative changes only the first modern
r3 return and must fail at weekday edge zero, return byte 20, before downstream
effects. Closing the stream clears perturbation. Direct tests reject unread
edges, truncated header/body, checksum damage and candidate oracle recording.

Direct tests use finite budgets, ordered read tracing and a write observer,
compare full registers/temporaries/PC/every mapped byte/allocator/import state,
cover years 0/1/1599/1600/1601 and unsigned extremes, all month/year boundaries,
1900/2000/2100, seven-day cycles, invalid signed days, flags 0/1/128/255,
different initial CR/XER and unmapped TOC/stack/tables. Actual caller instructions
with controlled inputs are synthetic evidence, distinct from real game calls.

Each immutable fixture requires two strict originals, entirely original stream
capture, all six baselines, two eight-operation candidates and exercised negatives.
The historical-edge checker now compares all three historical files, including
CMDOM001, against `.local/date-day-of-month-reconstruction/{day,week,ordering}-edges-01`.
Only translated instruction accounting may differ; all existing state/transcript/
PC/register/temporary/memory/allocator/import/callback/RNG checks and EOF remain.
Zero own calls mean absent coverage, requiring an additional original fixture.

## Reproduction

```sh
python3 tools/compile_recomp.py .local/recomp-input .local/recompiled \
  --root 0x10773710 --all-analyzed
cmake -S native -B .local/aot-build -DCMAKE_BUILD_TYPE=Release \
  -DCM_GENERATED_DIR="$PWD/.local/recompiled" -DCM_ORACLE_DIR="$PWD/.local/recomp-input"
cmake --build .local/aot-build --clean-first -j 4
.local/aot-build/cm-date-weekday-checks .local/recomp-input
ctest --test-dir .local/aot-build --output-on-failure
python3 -m unittest discover -s tools -p 'test_*.py'
python3 tools/test_control_cursor_audit.py .local/recomp-input/instructions.sqlite
python3 tools/test_match_presentation_audit.py .local/recomp-input/instructions.sqlite
python3 tools/test_unsigned_division_audit.py .local/recomp-input/instructions.sqlite
python3 tools/test_local_temporaries.py .local/recomp-input/instructions.sqlite
python3 tools/test_generated_temporaries.py .local/recomp-input/instructions.sqlite --sanitize
```

For each fixture, choose a new private edge destination; do not replace retained
streams. Substitute `week`/`season-week-01` or
`ordering`/`date-ordering-capture-02` as appropriate:

```sh
python3 tools/check_calendar_season.py .local/performance/season-day-01/SeasonFixture \
  --edges-directory .local/date-weekday-reconstruction/day-edges-01 \
  --historical-edges-directory .local/date-day-of-month-reconstruction/day-edges-01
```

After headers are stable and all linked State-dependent objects rebuilt, focused
UBSAN can be reproduced with the following script. It instruments the adapter,
all seven direct harness variants and five original pages, including `0x10715`.
Platform bridges remain normally linked; no whole-program or ASan claim follows.

```sh
python3 - <<'UBSAN'
from pathlib import Path
import subprocess
out = Path('.local/date-weekday-reconstruction/ubsan-repeat')
out.mkdir(parents=True, exist_ok=False)
flags = ['clang++', '-std=c++23', '-arch', 'arm64', '-O1', '-g',
         '-ffp-contract=off', '-fsanitize=undefined',
         '-fno-sanitize-recover=all', '-Inative']
sources = [('weekday', 'native/date_weekday_checks.cpp'),
           ('day', 'native/date_day_of_month_checks.cpp'),
           ('adapter', 'native/calendar_adapter.cpp'),
           ('month', 'native/date_month_checks.cpp'),
           ('later', 'native/date_ordering_checks.cpp'),
           ('before', 'native/date_ordering_checks.cpp'),
           ('difference', 'native/date_difference_checks.cpp'),
           ('shifts', 'native/calendar_checks.cpp')]
pages = ['page_10331', 'page_10332', 'page_10715', 'page_10717', 'page_10718']
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
for name in ['weekday', 'day', 'month', 'before', 'later', 'difference', 'shifts']:
    objects = [str(out / (unit + '.o')) for unit in [name, 'adapter', *pages]]
    binary = out / name
    subprocess.run([*flags, *objects, *libs, '-o', str(binary)], check=True)
    subprocess.run([str(binary), '.local/recomp-input'], check=True)
UBSAN
```

## Acceptance evidence — October 3, 2026

These are results of the final weekday build, separate from prior increments.
The original-only characterization captured 720 cases before implementation;
the semantic equation and observed r0/r3-r8/CR0/CA/read count agreed in all cases.
The final complete-state harness then passed 4,240 direct original/semantic cases,
559 chrono oracle cases, 319,675 wide Gregorian oracle cases and 786,432 pure
signed-day cases. Ordered reads, no writes, sparse mappings, TOC independence,
synthetic same-page/cross-page/indirect callers, callback restoration, EOF,
checksum/truncated streams, original-record enforcement and isolated return
perturbation all passed.

The final clean build regenerated all 1,953,157 instructions across 1,922 units,
with zero missing/pending instructions and all eight manifest entries. Headers
were stable before rebuilding and remained identical throughout validation.
All 1,969 actually linked State-dependent objects and 22 other linked objects
were inspected; none predates the final State header. The header SHA-256 is
`73155c433e4717aaf0769dc15d83caed82e8df46950e7a7d4e192cac652965ad`.

CTest passed 36/36, including all seven prior operations. Python discovery ran
30 tests: 26 passed and four optional audit groups were skipped by discovery.
Those groups were executed explicitly: control cursor 5/5, match presentation
6/6, unsigned division 7/7 and local temporaries 5/5. The temporary audit examined
1,953,157 instructions / 6,437,244 P-code operations and passed 2,080 local
fixtures. Generated UBSAN passed 199,742 differential executions across 2,050
actual instruction patterns (57,054 normal completions, 435 transfers, 23,247
matched memory failures). Focused UBSAN passed weekday, day-of-month, month,
before, later, difference and shifts using the final archive. This instruments
the changed adapter and pages `0x10331`, `0x10332`, `0x10715`, `0x10717`, `0x10718`;
it does not claim whole-program UBSAN or ASan acceptance.

### Immutable bounded fixtures

Every fixture passed two strict originals, entirely original four-stream capture,
all six historical/delivered baselines and two equivalent eight-operation
candidates. All state/transcript checks and EOF remained active. RNG, shifts,
difference, month, day-of-month and weekday negatives were rejected. Later/before
were absent in day/week and are explicitly unexercised there; the ordering fixture
exercised and rejected both. Own weekday coverage exists in all three, so no
additional fixture was required.

| Fixture | Own weekday calls per candidate | First-return negative |
| --- | ---: | --- |
| `season-day-01` | 1,690 | edge 0, byte 20, 2 → 3 |
| `season-week-01` | 5,831 | edge 0, byte 20, 3 → 4 |
| `date-ordering-capture-02` | 1,419 | edge 0, byte 20, 6 → 7 |

All three controls identify entry `0x10715b20`, field `weekday return`, before
any downstream mismatch. Entirely original capture and the seven-operation
weekday baseline report zero modern weekday calls. The latter checks all previous
semantic operations with weekday original. Ordering additionally exercises 38,051
later and 12,911 before calls. Only translated instruction accounting differs.

New private streams reside in
`.local/date-weekday-reconstruction/{day,week,ordering}-edges-01`. Independent
Gregorian calculations checked every original weekday record (1,690 / 5,831 /
1,419). The first same-page return LR is `0x10715a58` in all three. Fixture files
remained identical, and recaptured CMCAL002/CMMON001/CMDOM001 files are byte-identical
to their retained day-of-month originals. Full gzip identities:

**day**

- CMCAL002: `bdce201d4fc83a5c7e8eb9144421726f8bbbe76a1e12f1beaf424f756f5fd083`
- CMMON001: `0161ab1c87aa34562f70e54d3049736b27aab35e188303623e5ae6efad463608`
- CMDOM001: `09ac0ffd9445ddcfa2793766c6d7902decac235ac0180a02219a47817a44cf63`
- CMWKD001: `1d183246c61844490835a76b6c6b35dcd11da933cefd7be7e33ad31f9b4239aa`

**week**

- CMCAL002: `8ff9f77b64c342d5074136d350850cdf53b10eaf8b1987d9632a226e46974b12`
- CMMON001: `1cef11c1352e2f3a67186a6761212c03c08fda6182502dc6c2def6e0cd6f2e66`
- CMDOM001: `e106535c0c739ead6bb714b468a1dc6d4b3f60358e1a0358fbbcf807556713ba`
- CMWKD001: `97f7d2761f770f1db7ded0235d92199bdf70f432a4794e2884d18a12c8f2f6b8`

**ordering**

- CMCAL002: `a7849b48626ea92da01ed062417522323477196cd3a3a78c857357076f7b8954`
- CMMON001: `adfd71c6417a9a69d00bc5a250aab1d45b33c70e23a37f7c8688d15cf930c6bf`
- CMDOM001: `9157b1525021b92dc0e81b92cd297a9d2b47228e4ae505dc63fc4cffc89f5220`
- CMWKD001: `2a43559651d29c84f1c32ac468310c45da6d166672b271adcddeedeece440a61`

### Real ARM64 app and persistence

A new isolated candidate/Runtime/Runs was prepared at
`.local/performance/date-weekday-live-01`, copying the retained day-of-month
Runtime. The candidate was prepared only after the stable full build. The source
September 1 checkpoint still hashes to
`ce48d8af426557e18976292e3bd22172241b1d61d8a575108a6f862b3583463f`;
the August 28 alternative remains
`22a6263cd149251c58c74618d4cf73831218e74123dcee79edd4debbc2c339a3`.
No historical candidate, checkpoint, date or RNG was edited.

Original Load showed Victor Moraes / Cruzeiro, Sunday September 1, 2002 afternoon
(`Domingo 1.9.02 TAR`). Calendar results were July 31 Palmeiras 1:2, August 3
Fluminense 1:1, August 7 Atl.Paranaense 2:0, August 10 Ponte Preta 2:1, August 17
Internacional 2:1, August 20 Cerro Porteno 2:1, August 24 Vitoria 1:2 and August 31
Goias 0:3. September 6 Coritiba, September 8 Santos and September 11 Colo Colo
remained pending; Cruzeiro was 16th nationally. The localized weekdays remained
under generated formatting routines.

Luisao's profile showed February 13, 1981 / age 21 / one international cap,
condition 80%, 26 (4) official appearances, one goal, two assists, average 7.20,
form 8-7-9-8-6 and Good morale. His full-time contract showed BRL 8,750/week,
start N/D, bound Yes, expiry December 24, 2004, squad rotation, no bonuses/clauses.
Original Game Options → Save Game wrote `DateWeekday-01Sep2002.sav`. Original
exit completed normally; process absence was confirmed before rebinding/reopening
the exact candidate. Original Load preserved every listed observation.

The checkpoint had a backlog of unread July/August news. Reading it through
original Read Next/Continue Game changed no date. The first subsequent normal
simulation step stopped at a transfer-offer news item on **Tuesday September 3,
2002 morning** (`Terça-Feira 3.9.02 MNH`), skipping Monday naturally. No holiday,
forced September 2 or intervening match was used. All listed calendar results,
pending fixtures, club position, profile statistics and contract remained as
above; condition had normally recovered to 89%. This is the actual observed
Sunday → Tuesday acceptance interval.

Original Save wrote `DateWeekday-03Sep2002.sav`; exit was normal and the process
was absent before reopening the same candidate. Original Load preserved Tuesday,
morning, manager/club, every listed result/pending fixture, profile including
89% condition and the contract. The final session exited normally too. Only one
real game app ran at a time. The two new private save SHA-256 values are:

- September 1: `bbbc4426afdfd23569d5a0784da64c0da4a46d5088a702630e022d860a704351`
- September 3: `24bc76c79e4ebba79b5725fe01dc28181590aaa59ce2127257af8708b28d25ed`

Three normal real-app runs (`app.KIThrI`, `app.EERzVE`, `app.wp15lR`) report
160 / 1,794 / 152 own modern weekday calls, respectively, and zero exceptional
original calls. The normal selector has no weekday fallback. All 96 available
weekday samples passed independent checks against Python's Gregorian weekday;
arguments, pointers and LR are retained privately. The samples include real
same-page `0x10715a54` calls returning to `0x10715a58`, during normal load/fixture
date adjustment. Samples are limited to each run's first 32 calls; they do not
claim argument capture for every later current-date rendering. The displayed
Sunday/Tuesday observations and Save/Load evidence are separate from these
samples and from bounded automatic equivalence.

### Preservation and installation identities

The installed reference was copied with `ditto --rsrc --extattr` to
`.local/date-weekday-reconstruction/reference/installed.app` before any replacement.
The new ARM64 reference backup hashes to
`1fa57f3f11d3c911f4d452ed5abf58a55112ed6b71defafb6439428f0fd234dc`;
its complete inventoried contents/forks/Finder metadata/xattrs/modes/timestamps/
flags remain identical. It is distinct from older reference backups.

The final source executable hashes to
`7284838b3e6d74785a6969d64bf2d19619ad2cdc3e5e7c0601e6e551a97db063`.
The signed validation executable hashes to
`ace1c411a6e863fcec0125105f763df7e81543df53ae440c151b82580060db14`;
its signature is verified with `codesign --verify --deep --strict`. Both are
ARM64. The existing `tools/install_native.py` installed the approved production
source at `~/Applications/CM 01-02 Native Prototype.app`. Installed executable
SHA-256 equals the source hash above, and its architecture is ARM64. Installed
`program.json` and all three segment files match the private original inputs.
The production plist contains no private validation/capture properties.

Before, preinstall and after-install personal inventories agree for 809 Game / 5 Runtime /
43 Runs files, including content, resource forks, Finder metadata, all extended
attributes, modes, modification times and flags. Counts are observed, not fixed
requirements. All 745 protected historical identities remain unchanged, including
both retained September/August checkpoints, three historical stream formats,
immutable fixtures, private exports, the four preexisting handoff prompts and the
Git-LFS post-match save
`82c504f46ce54a723137dbf42c005d458a4d7b18a9abab4c9f92627aa94f08fb`.
New saves/streams/logs/builds remain private and ignored.

The successful-domain contract and bounded evidence do not establish a full-career
replay or reconstruct neighboring formatting/adjustment/player/contract routines.
The deferred asymmetric leap difference remains unchanged.
