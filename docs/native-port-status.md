# Native port investigation — updated 2026-10-02

## Required outcome

Run the actual Championship Manager 01/02 simulation as an ARM64 macOS application,
without an emulator. Asset viewers, recompiled test programs, decompiler output,
and a native launcher around a legacy runtime do not satisfy this outcome.

The current native build has completed the core single-player flow in live use:
managed game creation/loading, player and competition screens, date advancement,
a full match, return to management, saving, normal exit, and reloading the
post-match save with the Anapolina result preserved (user confirmed). Multiplayer and some other services remain
unfinished; this is a working development build, not a fully validated release.

## Original Mac media: observed facts

- Image: `CM-01-02.cdr`, 628,631,552 bytes, HFS+.
- Mounted disc inventory: 831 files; no C, C++, Objective-C, Swift, or assembly
  source files were found.
- Game data fork: 9,394,972 bytes; PowerPC 32-bit PEF, format version 1.
- Data-fork SHA-256: `91692f95df17cf851e7c1d6dbdecd0bd0a5809ce1937133e209aee859a6b516b`.
- Resource fork: 37,297 bytes; SHA-256
  `1f4ae4f499d43f163b9153026939b19a2ad022a8b4181690ba7b1b49cf5e551c`.
- Code/constant section: 9,124,484 bytes. This is not a pure instruction count.
- Three PEF sections; 259 imports in four library records: two CarbonLib records,
  Apple Carbon Multimedia, and Apple Carbon Networking.
- Zero exported symbols. This does not mean there are no internal functions.
- Ghidra 12.0, explicitly configured for `PowerPC:BE:32:default`, detected 5,833
  functions. Analysis completed without its time limit firing. This count is an
  analyzer result, not proof of complete function recovery.
- The original readme identifies Elegant Chaos and Sports Interactive as the Mac
  port authors and Feral Interactive as publisher. It documents manual folder-copy
  installation, but the copied PowerPC executable remains incompatible with ARM64.

The input image was already mounted read/write when work began. The tools created
here only read the supplied media; do not infer an original-distribution image
checksum from the current mounted copy.

## Working native code

`CMAssets` and `cm-assets` compile into a Mach-O ARM64 executable. Four deterministic
test groups cover byte order, index boundaries, path rejection, stale image masks, RGB555
channels, truncated images, and excessive dimensions.

The real disc pass reads 22 index entries and validates the fixed-record boundaries
of the 21 present tables. `staff.dat` has zero records and is absent on this disc.
The inspector decodes all nine root `.rgm` images. The descriptor bytes at 12...47
are obsolete: eight files retain Windows-style mask words and `feral.rgm` has
other values. The actual Mac payload uses big-endian QuickDraw RGB555 pixels.
All pixels across the nine files have the high bit clear; white is stored as
`0x7fff`. Visual checking caught and corrected an initial stale-mask interpretation
that tinted whites pink. The output also tracks unsupported images explicitly.

The `.dam` index uses an eight-byte prefix and 68-byte entries with big-endian
numeric fields. Present table files have a four-byte prefix. Relevant observed
record sizes differ from the existing Windows definitions:

| Record | Mac bytes | Existing Windows bytes |
| --- | ---: | ---: |
| Index entry | 68 | 67 |
| Player | 72 | 70 |
| Club | 584 | 581 |
| Staff history | 20 | 17 |

Sizes establish structure only; individual Mac fields have not yet been mapped.
RGM descriptors retain little-endian Windows mask words despite their big-endian
Mac dimensions and pixel payload. Do not use those stale masks to interpret pixels.

## Existing Windows reconstruction

The user reports having Ghidra-decompiled Windows code. A local candidate was found
in a Google Drive-synced `recm-0102` folder.
The exact preferred source location is still awaiting the user's reply.

The candidate includes original decompilation snapshots, Ghidra exports, and a
substantial C++23 reimplementation. Its README describes 9,243 decompiled functions;
that historical metric has not been re-counted in this investigation. Its roadmap
explicitly describes the main day-advance loop as orchestration with unfinished
subsystem functions, not an end-to-end runnable game.

Current verification against those files, without editing the source project:

- CMake configuration targeting ARM64 succeeds.
- Full `engine` compilation fails real 32-bit pointer-layout assertions. Example:
  `si::ListNode` expects 14 bytes and measures 26 bytes with 64-bit pointers.
- The standalone database-structure test compiles as ARM64 and passes 58/58 checks.
- A date-subsystem build probe reaches an unused-function warning in `langlib.cpp`
  under `-Werror`; omitting the language module instead leaves a real unresolved
  symbol. Neither probe is recorded as a passing date-engine port.
- `rng.cpp` still includes Windows time APIs. `MappedFile` uses Windows handles and
  mapping APIs. These need actual platform implementations.

## Next implementation boundary

An additional route now has working code: [static recompilation of the original
Mac executable](aot-port.md). Five original routines pass 297 ARM64 cases against
a build-time PowerPC oracle and independent arithmetic expectations. The compiler
has generated C++ for all 1,953,048 analyzed instructions; this still does not prove
a complete game. Native OS services and indirect control-flow coverage must be
validated through real startup and gameplay. The Windows route below remains
available as a source of semantic references.

The entire analyzed listing builds as ARM64. Native startup reads the original
language database and fonts, updates menus, restores private printing preferences,
delivers the original launch callback, and creates two native windows. The original
background paint callback runs; original-code rendering now shows opening logos
and the Game Settings interface.
RGB555 pixel buffers, color conversion, overlapping copies, native cursor resources,
and private file writes now have checks. The supplied executable and resource fork
still match their audited SHA-256 values.

The installer now copies 809 original game/Extras files into user Application
Support and installs an ARM64 app in `~/Applications`. Resource forks, Finder
metadata, and file checksums are verified. Updates preserve private preferences
and runtime files. The original opening and main menu were physically observed
with the image unmounted; the user navigated the new-game league selection.
Installing the original `Extras/qpr for the cup` also satisfies the original
CD check through the local logical volume, without replacing the check. After
the user dismissed the prompt, database loading reached "Iniciando Ligas".

That run stopped at the original `fctiwz f2,f6` instruction at `0x1022cd24`, an
exceptional floating-to-integer conversion missing from the translation. The
log records 109,166,643,823 instructions and 18,338,941 native calls. A separate
quit attempt reached the missing `DisposeCCursor` bridge. Private file deletion
now uses the native File Manager, including resource forks and busy-file errors;
deleting original media entries still requires unimplemented tombstones.
Both original opening sounds have native rendering/completion evidence, but
audibility remains unverified. At that stage, day advancement, matches,
and save/reload were still unverified. The conversion now follows documented PowerPC
rules; 96 edge/mode cases execute the original failing instruction. All eleven
CTest groups pass, including the 297 original-routine comparisons. The updated
installed app reopened at the main menu without mounted media. The user's next
new-game attempt passed the conversion boundary, completed league initialization,
and reached the original "Escolha Nome" trainer form, observed on the physical
display. The user then advanced to the Brazilian team list, where background-image fallback caused
a null access at `0x100d53d8`. The original caller deliberately supplies a null
club for its default picture but eagerly fetches an unused club descriptor first.
A guarded source correction at `0x10713aec` skips only that null lookup and keeps
the original picture selection. All twelve CTest groups now pass, including
nonempty default picture lists, preserved RNG consumption, nonnull descriptor
lookup, and checks that unrelated null accesses still fail. The updated ARM64
app passed team selection: the user assumed control of Cruzeiro, and the retained
frame from `Runs/app.rCcF8C` shows the original squad screen. Opening a player
then stopped at the missing `CarbonLib::num2dec` service (`0x10bf7b10`, caller
`0x1077a070`); a second attempt, `app.tCEQVP`, reproduced that same boundary.
The native decimal bridge now marshals the original floating argument, format,
and 42-byte output to Apple's ARM64 function. It preserves rounding settings,
unused record bytes, and the original documented exception behavior. Tests cover
1,064 ABI cases and 14 executions of the original formatter. The user has now
opened a player in the recovered game. The first save attempt
revealed a canonical filename mismatch in private league temporaries, followed
by a missing final rename operation. Both filesystem boundaries are implemented
and all 13 test groups pass. The original rename wrapper completed the recovered
`Mac-Teste.sav` with unchanged bytes/forks/metadata. Its first load failed with
`INDEX.CPP 5773`: the original English language table misorders one club, shifting
the names used by saved database IDs. A separate copy with only that language
block aligned, `Mac-Teste-Recuperado.sav`, loaded successfully in `app.BAEOk6`;
the Cruzeiro squad was observed and the user confirmed that player pages open.
Opening the competition then failed at `0x100d5084`, another eagerly evaluated
null background descriptor. The exact call at `0x10713b3c` is now guarded;
club and competition default selection, RNG state, nonnull getters, and unrelated
null faults pass the original-routine tests. All 13 CTest groups pass after this
change. The user confirmed competition navigation in `app.esyws0`. A normal save
as `Mac-Teste-Validado` in `app.VDk5TM` and its successful load in `app.pigLIp`
are also user-confirmed; the new file needed no repair and all 118 compressed
blocks pass boundary/length validation. `Continue Game` initially stopped at
`0x101355d4`, where the original code truncates a fractional divisor to zero.
The new focused test reproduces that stop; a local lowering now preserves the
nontrapping PowerPC continuation where the undefined quotient is provably unused.
Its 96 arithmetic/RNG cases pass. In `app.cwxsEo`, the updated game advanced
from 21 December 2001 to the interactive news screen on 3 January 2002, then
entered a friendly against Anapolina on 4 January. The user confirmed normal
advancement and reported a stop during the match. The retained match frame shows
minute 4, score 0–0; the log identifies another zero divisor at `0x1024773c`.
The correction subsequently passed live in `app.yPRT55`: the user confirmed
completion, and the retained frame shows Cruzeiro 1–0 Anapolina at minute 93,
with "Final Jogo" displayed. Continuing from full time then triggered an audio
callback during the original sound teardown, after its PCM/header had been freed.
The new match correction passes 540 original arithmetic/quotient cases and seven
compiler-proof test groups. All 17 CTest groups pass in 38.52 seconds; the updated
match arithmetic and the sound transition are now live verified.
The audio scheduling correction reproduces and fixes the teardown failure using
the actual original cleanup routine in four variants. All 18 CTest groups pass
in 38.82 seconds, and the matching ARM64 build is installed. The added memory
bounds test checks 13,823 mapped-byte cases; no memory optimization is applied
yet. The user confirmed return from full time in `app.pis6iu`, saved as
`Mac-Teste-Validado-Pos-Jogo`, and exited normally. The save's 120 compressed blocks
validate and a private backup is retained. Reloading that latest save is pending.
The latest installed build also reports an explicit unsupported-network error
instead of throwing at initialization. Multiplayer is not implemented.
The day-advance arithmetic correction and normal-exit bridges are now installed.
All 16 CTest groups pass in 26.94 seconds. Shutdown tests execute the original
cursor initialization/cleanup routines three times using the supplied resources,
verify Resource Manager handle reuse and released allocations, and check that
`ExitToShell` unwinds nested callbacks to a successful host exit. Normal UI quit
is now live verified in `app.pis6iu`: its log records successful exit after
47,838,604,786 original instructions, without the diagnostic stop alert.
The optional AddressSanitizer build succeeded, but its runtime
deadlocked during initialization before entering the test; that check is
inconclusive and is not counted among passing tests.
Current evidence is in `reports/generated/aot-status.json` and the detailed
[port notes](aot-port.md). The Windows-specific possibilities below remain reference
options; current implementation proceeds through the original Mac program.

1. Establish the preferred ReCM source snapshot and its version; preserve the
   original source and decompilation artifacts.
2. Separate the Windows injection ABI from native runtime types. Preserve explicit
   32-bit serialized IDs/offsets and use native pointers only for runtime objects.
   Do not simply disable layout assertions or truncate ARM64 pointers.
3. Port time, filesystem, graphics, input, audio, and networking boundaries while
   keeping the already-recovered engine semantics and regression tests.
4. Map the Mac `.dam` fields or deliberately adopt a verified Windows data source.
   Mac records must not be reinterpreted as packed Windows structs.
5. Recover and implement the missing startup, simulation, match, UI, and save paths.
6. Validate new-game creation, a playable match, day advancement, and save/reload
   on ARM64. Compare recovered logic against trusted original-game outputs.

A native managed game, player/competition screens, normal save/reload, day
advancement, a full match simulation, return from full time, a post-match save,
and normal exit have run. The user also confirmed reloading the post-match save
and finding the completed Anapolina result in Last Game in run `app.oXBAh4`.

## Completed milestone: season reference and regression foundation

As of 2026-10-01, the user has replaced performance optimization with the
[season reference milestone](native-season-reference.md). Complete a real native
season, verify persistence on both sides of its annual transition, and establish
a bounded repeatable simulation comparison before semantic modernization.
Existing experiments and private data are preserved. New optimizations,
features, rules, and graphics changes are suspended for this increment.
The real application has completed 2002 and reached the ongoing 2003 edition.
Normal Save/exit/reopen passed on both sides, and exact bounded comparisons of
one and seven full days pass equivalent executions and reject a known RNG
divergence. Inspected titles, division changes, player history and contract/
transfer fields are documented separately from unexercised paths. The validated
ARM64 application is installed, with every installed Game/Runtime file and its
Mac metadata preserved. Calendar/date arithmetic is the first proposed semantic
replacement for the next increment, backed by 1,232 original-routine
checks; no semantic replacement is included in this milestone.

## Accepted implementation sequence

As of 2026-10-02, the user has selected calendar/date arithmetic as the first
semantic reconstruction increment. Its executable handoff is the
[calendar reconstruction goal prompt](features/native-calendar-reconstruction-goal-prompt.md).
The season reference milestone above is complete. The modern calendar module and
production entry hooks are implemented. Direct equivalence, original/modern
one- and seven-day simulation comparisons with negative controls, and the full
30-group suite have passed. Month advancement and preannual Save/Load were
observed in the app. Normal advancement also crossed into 2003; postannual
Save/exit/reopen/Load preserved the checked dates, club, player contract/history,
fixtures and Mineirao state. The validated ARM64 app is installed, with all
809 Game / 9 Runtime / 41 Runs files and their captured Mac metadata unchanged.
The [calendar reconstruction report](native-calendar-reconstruction.md) records
production call counts, delivery identities, commands and the bounded scope.
Later subsystem ordering remains open and should follow
the contracts and behavior comparisons established by each increment.

First produce and verify the playable native ARM64 game through static recompilation
and native OS bridges. Then recover clear names, types, and responsibilities and
replace generated low-level C++ incrementally with maintainable C++ modules, using
behavior comparisons before and after each replacement. C++23 compilation already
works; semantic modernization is a separate stage. The ReCM reconstruction remains
a reference, with its Windows-specific layouts and incomplete subsystems accounted
for. Its engine is not linked into the current Mac prototype.

Historical performance work ran alongside functional porting. Measure real startup and
league initialization before selecting changes; prioritize redundant work,
filesystem overhead, and diagnostic logging without changing game semantics.
Larger simulation optimizations need a working new-game/day/save baseline and
behavior comparisons. Correctness and performance evidence remain separate.
The first measured change replaces quadratic filename merging with an ordered
index using the same native comparison rules. Median isolated volume-query time
fell from 36.0 ms to 7.7 ms against the installed data (nine warm samples per
build). Eleven CTest groups pass; complete game-loading speedup is not yet measured.

## References

- [Apple: PEF structure and PowerPC runtime](https://developer.apple.com/library/archive/documentation/mac/pdf/MacOS_RT_Architectures.pdf), chapter 8.
- [Apple: porting an application to Apple silicon](https://developer.apple.com/documentation/apple-silicon/porting-your-macos-apps-to-apple-silicon).
- [Apple: video pixel formats](https://developer.apple.com/documentation/coremedia/video-pixel-formats), including 16BE555.
- Original disc readme, preserved locally by `tools/audit_media.py`.
- Existing ReCM `AGENTS.md`, `CLAUDE.md`, `src/cpp/CMakeLists.txt`, and current source files.

## Semantic date difference delivery — 2026-10-02

Future work, explicitly deferred by the user: investigate correcting the
ascending-year leap quirk after assessing caller dependencies and gameplay
effects. The [deferred improvement](native-date-difference-reconstruction.md#deferred-improvement--2026-10-02)
requires a separate decision and validation; the delivered behavior remains
compatible with the original.

Mac date difference `0x10718130` now uses the C++23 calendar module and legacy
adapter in production. Its second operand is an eight-byte by-value record,
including three padding bytes. The ascending-year leap quirk, initialization,
ordered scratch writes, ABI and stack residue are preserved. Independent date
comparison/conversion and other engine subsystems are unchanged.

The [date difference reconstruction report](native-date-difference-reconstruction.md)
records 6,892 direct equivalence cases, 494 independent-domain oracle checks,
31/31 CTest groups, Python/export audits and UBSAN. One- and seven-day immutable
fixtures passed strict original runs, the delivered addition/subtraction baseline,
two modern runs each and all RNG/shift/difference negative controls. Only the
existing instruction-accounting difference is permitted; no state masks were added.

A single isolated ARM64 app advanced normally from 19 July 2002 night to
3 August afternoon. The advancing run logged 500,953 semantic difference calls,
with 32 auditable argument/result samples and no original fallback. Calendar,
player/contract information and Save/exit/reopen/Load were checked before and
after the month boundary. The validated app is installed with its prior
executable backed up privately. Current before/after inventories agree for all
809 Game / 5 Runtime / 42 Runs files, including captured Mac metadata.

This establishes a bounded semantic increment, not full-career or historical
hardware equivalence. The previous calendar/annual report remains separate
historical evidence. No matched speed benchmark or new optimization is claimed.

## Semantic independent date ordering delivery — 2026-10-02

Mac date-ordering predicate `0x10331860` now executes the C++23 calendar module
through the legacy adapter. It returns exactly whether the second by-value date
is strictly later, preserving unsigned years, signed days, complete padding
spills, aliasing and architectural clobbers. Production dispatch covers the
actual same-page caller, other pages, indirect calls and callbacks. Addition,
subtraction and difference remain delivered; the deferred leap quirk is unchanged.
Other independent/inlined comparisons and larger date-record routines remain
outside this increment.

The [independent ordering report](native-date-ordering-reconstruction.md) records
5,646 direct original/modern cases, 156 Gregorian oracle cases, 196,608 pure
signed-day checks, 32/32 CTest groups, 26 Python tests with four optional groups
also executed explicitly, temporary auditing and focused/generated UBSAN.
The old day/week fixtures passed strict original and modern/baseline comparisons,
with zero ordering calls reported honestly. A new entirely original 20–21 July
morning fixture exercises 38,051 modern ordering calls in each of two equivalent
candidate runs; an isolated flipped return fails at its first ordering boundary.
Existing RNG/shift/difference controls and original fixture/stream hashes remain
preserved. Only translated instruction accounting may differ.

The isolated ARM64 candidate advanced normally from 3 to 10 August afternoon.
Contract-expiry sorting logged 38,422 modern orderings; 32 live samples agreed
with independent date ordering. Calendar, player/contract panels and normal
Save/exit/reopen/Load passed before and after advancement. The report separates
these UI observations from automatic equivalence and records excluded capture
attempts. No annual UI crossing or matched speed benchmark is claimed.

The candidate is installed. Source/installed executable SHA-256 is
`b61765e722c6e5f7624550ac15524fcdb623f933e9306c3329644310a887f545`;
the signed isolated candidate is
`01395e5f2e345635f414390dc89d8816103ef68dc5af9490a4ee538fd46a227c`.
The prior installed ARM64 app remains backed up privately. All 809 Game / 5
Runtime / 42 Runs files and inventoried metadata match before/after installation;
personal saves, original data and the immutable LFS reference remain unchanged.
This is a bounded semantic increment, not reconstruction of the remaining engine.

## Semantic independent date-before delivery — 2026-10-02

Independent entry `0x103317f0` now executes the C++23 predicate for second date
strictly earlier than first, preserving unsigned years, signed days, complete
by-value spills, aliasing and its distinct first-day r5 clobber. The actual
same/inter-page callers, indirect dispatch and callbacks reach the destination
hook. The four previous contracts and deferred ascending-year leap quirk remain
unchanged; other comparisons and date/engine subsystems remain outside scope.

The [date-before report](native-date-before-reconstruction.md) records 5,646
direct comparisons, 156 Gregorian oracle cases and 196,608 signed-day checks
for each predicate; 33/33 CTest groups, 27 Python tests with all four optional
groups also run explicitly, temporary auditing and generated/focused UBSAN passed.
All linked State-dependent objects use the final layout. Immutable day/week
fixtures passed with zero before calls. The original ordering fixture passed
two candidates and all three baselines, with 12,911 modern before calls per
candidate; its inverted return failed at the first before edge, 26. Existing
negative controls and strict state/transcript/RNG comparisons remain preserved.
New versioned original streams remain separate from historical streams.

Real isolated ARM64 contract-expiry sorting exercised 13,153 modern before calls,
with 32 independently checked samples. Calendar, player/contract information and
normal Save/exit/reopen/Load passed on 10 August and after normal advancement to
27 August 2002 night. These observations are separate from fixture equivalence.
The report records an excluded native File-menu save attempt and the accepted
original in-game save route. No annual crossing, whole-career equivalence or
speed improvement is claimed.

The production app is installed. Source/installed executable SHA-256 is
`d2784cfde149a28d8e8c1d067e26ac90cfff482581a36858a0d215a7790194a1`;
the signed private candidate is
`c20788488bb734e5a97590ca75c315a6d5d4f39e75f93fbc65613892ec606511`.
The current reference remains backed up privately as executable ARM64 with hash
`b61765e722c6e5f7624550ac15524fcdb623f933e9306c3329644310a887f545`.
Before/preinstall/after inventories agree for all 809 Game / 5 Runtime / 42 Runs
files and captured Mac metadata. Personal saves, prior checkpoints, immutable
LFS reference, private inputs and historical fixture/stream hashes are preserved.


## Semantic independent date-to-month delivery — 2026-10-02

Independent entry `0x10718360` now executes semantic C++23 month conversion.
The stored byte flag selects a mutable common/leap table through the current
TOC; the year is not read. A reverse signed16 strict-threshold search returns
zero-based month or -1, preserving invalid-day behavior, exact ordered reads,
CTR groups, complete ABI/temporaries/PC and absence of writes/imports/RNG.
The destination hook covers generated same/inter-page dispatch, indirect calls
and callbacks. The five previous semantic contracts and the explicitly deferred
ascending-year leap quirk remain unchanged. Adjacent day conversion, weekday,
formatting and the remaining engine are outside this increment.

The [date-to-month report](native-date-month-reconstruction.md) records 1,684
direct original/modern cases, 239 Gregorian oracle cases, 131,072 pure signed-day
checks, mutable tables/TOC, synthetic caller checks and independent ordered-read
comparison. The final suite passed 34/34 CTest groups and 28 Python tests, with
all four optional export groups separately executed, full temporary auditing
and generated/focused UBSAN. All 1,967 linked State-dependent objects use the
final layout. Immutable day/week/ordering fixtures each passed two strict originals,
original stream capture, two candidates and all four historical/delivered
baselines. Own month counts were 8,807 / 38,256 / 8,433 per candidate.
The ordering fixture also exercised 38,051 later and 12,911 before
calls. All six exercised negatives passed, including month rejected at its first
return boundary. Legacy CMCAL002 hashes, original fixture files, strict state /
transcript / RNG comparisons and permitted instruction-accounting difference
remain preserved; month uses a separate CMMON001 stream.

A single private ARM64 candidate instance advanced normally from 27 to 28 August
2002 night, with original Game Options Save/normal exit/process absence/exact
reopen/Load checked on both dates. Calendar results and Luisao's profile/contract
survived; final condition 63% also survived reload. Three app runs counted
774, 870 and 85 modern month calls, with 96 independently checked valid samples.
These UI observations are separate from automatic original/modern equivalence.
No full-engine reconstruction or matched speed improvement is claimed.

The final production ARM64 application is installed; source/installed executable
SHA-256 is `caf91df0b7d4313937711725022c38a59949bebe66a5d2c07e934efae1058756`.
The signed private candidate is
`f3e17857f83cf0922028847df8c2718b5f6fb9df83ea4d82f0f5b86e5a23de5d`.
A new executable reference backup retains
`d2784cfde149a28d8e8c1d067e26ac90cfff482581a36858a0d215a7790194a1`.
Before/preinstall/after inventories agree for the currently observed 809 Game /
5 Runtime / 43 Runs files and all inventoried metadata. Personal saves, previous
checkpoints, immutable LFS save, original inputs and historical streams remain
unchanged. Private diagnostic settings are absent from the installed plist.
