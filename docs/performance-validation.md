# Real-game performance validation

This document preserves historical performance experiments and their status at
the time of each run. On 2026-10-20 the user made native match speed the immediate
priority: Turbo in the original speed selector by default, faster watched and
background matches, and measured scaling toward all playable leagues. General
semantic reconstruction resumes after this performance work. New-career and
league loading are explicitly part of this priority. The ongoing
[background and league-loading investigation](native-background-performance.md)
separates whole-game observations from measured subsystem improvements. The
[native season reference](native-season-reference.md) remains behavioral
evidence; timing claims require matched baseline/candidate measurements.

## Native match-speed delivery, 2026-10-20

The [native match-speed report](native-match-speed.md) records the current
installed delivery, source changes, exact identities and measurement boundaries.
Turbo is now the default sixth choice in the original speed selector, with a
persistent native preference. The existing runtime shortcuts are enabled by the
native UI independently of the selected commentary duration.

Against revision `e263d3e483f8e1593ad74401d94c0a9e82512fab`, localized spatial-role
execution and dispatch changes reduce paired wall-time medians by **3.07%**
(Arsenal) and **2.26%** (Cruzeiro); paired CPU reductions are **2.90%** and **2.42%**.
Each scenario uses ten alternating pairs of identical 600-advance intervals,
two warmups and five measured samples per process, with byte-identical complete
final snapshots and unchanged instruction/import/sampler counts. Faster pairs
are 10/10 and 9/10 for both clocks. General memory-read experiments were rejected
because their measured gains were absent or inconsistent.

Twelve focused CTests and eighteen source-audit checks pass. The final candidate
completed a real match and the other afternoon fixtures, then saved, exited,
reopened and retained the score/report. The signed ARM64 delivery preserves all
31 native code/data sections of that tested candidate, passes strict signature
verification, and opened/exited normally after installation. Full-round,
season and all-playable-league speed are **not measured by these intervals**;
near-instant all-league processing remains a target. The following sections
preserve earlier experiments and their original references.

## Scope and reference

Optimize the native ARM64 game while preserving its simulation, original
800 x 600 interface, resources, save format, and normal input behavior. Use
`saves/Mac-Teste-Validado-Pos-Jogo.sav` as an immutable starting point. Its SHA-256
is `82c504f46ce54a723137dbf42c005d458a4d7b18a9abab4c9f92627aa94f08fb`.
Never load a personal save for destructive testing or overwrite the reference.

The performance work starts from `a453bda`. The local machine is an Apple M4 Pro
with 24 GiB RAM, running macOS 26.6.2. The existing build uses C++23, `-O3`,
`-ffp-contract=off`, and the Command Line Tools. No simulation optimization is
established by the measurement infrastructure itself.

## Isolated UI validation

The user explicitly added autonomous game operation to the goal on 2026-09-26.
Some UI drivers deliver AppKit mouse events without moving the desktop pointer.
The original game queries `GetMouse` before `WaitNextEvent`, so a delivered click
can otherwise act at the physical pointer or at the previous click position.

After a successful build, prepare a **new** diagnostic app:

```sh
python3 tools/prepare_validation.py baseline-01
```

The tool copies the selected app, original program segments, and private runtime
under `.local/performance/baseline-01`. It preserves forks and metadata with
`ditto`, creates `Perf-Reference.sav`, verifies the reference hash and ARM64
architecture, checks the copied executable before ad-hoc signing, and writes a
private manifest. Existing destinations are rejected. `--app` selects a preserved
before/after build; `--runtime` selects a prepared private starting state.

Launch and operate the app through the available UI tools. This preparer does
not launch the app, inject inputs, edit game memory, or bypass game routines.
Inspect the screen after each action; a submitted click is not an observed result.

Only a separately identified diagnostic bundle can specify `CMValidationRuntime`.
The launcher rejects runtime paths inside the installed game's support directory.
Normal installed launches retain their existing runtime and physical mouse.

The diagnostic input mode delivers an original mouse-moved OS event before a
press at a new position. The next original `GetMouse` therefore sees the new
position before `WaitNextEvent` delivers the queued press. Press/release order
is preserved. A diagnostic click lasts at least 80 ms so a
one-millisecond synthetic gesture is not consumed entirely between game updates.
The original simulation clock, input callbacks, and game code still execute.
This is an explicit test-device policy, not a change to normal mouse behavior.
It is not yet a general guarantee for drag gestures or every keyboard shortcut.

During investigation, tool-generated clicks retained Command (`0x0100`) from an
earlier keyboard shortcut and the game correctly classified them as secondary
clicks. A plain Escape keypress cleared that tool input state. Check input logs
when a visually correct click has no effect; do not remap the original game's
secondary-click rules. Prefer on-screen controls during repeatable scenarios.

## Measurements

Diagnostic launches enable sparse `PERF` log records at frame presentation,
input delivery, and data-fork open/close. Records include monotonic elapsed wall
time since platform construction (after original-segment verification), process
user/system CPU seconds, original instruction count, original PC
and return address, and a numeric event/file/window detail. These records are
local and private. They do not log every translated instruction or OS call.
The optional environment switch is `CM_AOT_PERFORMANCE_TRACE=1`.

Use matched builds, runtime snapshots, league settings, and reference saves.
Measure at least three runs and report the median and range. Exclude unrelated
builds, tests, debuggers, and profiling from timing runs. Collect representative
sampling profiles separately. Distinguish cold process launch from warm filesystem
caches; do not call a warm run a cold-disk benchmark. Compare instrumented and
uninstrumented observations before claiming precise small gains.

Record these scenarios independently:

1. App launch to first usable main menu, retaining the opening presentation.
2. Confirmation of `Perf-Reference` load to the usable managed-club screen.
3. New career with the same database and leagues to the manager/club screen.
4. Continue from a fixed saved date to the next actionable screen.
5. A fixed pre-match save through full time, return to management, save and reload.

Mark which time belongs to original presentation delays or user interaction.
Set ambitious numerical improvement targets only after measuring the matching
real scenarios. Do not substitute filesystem or memory microbenchmarks for them.

The first clean pair establishes provisional engineering targets of at most
2.2 seconds for reference loading (about half the observed 4.44-second baseline)
and 20 seconds for the 11-to-18-January advance (over 55% below 45.92 seconds).
These are targets, not achieved or promised results. The profile's substantial
memory-check and instruction-accounting cost justifies investigating deeper
runtime changes beyond directory caching. Confirm the baseline distribution
with at least three matched runs before accepting a performance conclusion.
Opening, new-career, and match targets require their own measured baselines;
the two provisional targets do not replace those scenarios.

## Investigation notes

The first five-second startup profile had 448 main-thread samples; 313 were under
the native platform dispatcher and 228 under path resolution. Repeated directory
enumeration is a current investigation target. The sample spans only part of
startup and cannot establish the cost of save loading or simulation.

Initial diagnostic openings reached the final menu presentation at approximately
24 seconds. These are preliminary observations, not a completed baseline or a
speedup claim. Further scenario measurements and fidelity checks remain required.

An LLDB attach attempt did not complete and was stopped. It supplied no valid
runtime snapshot. Read-only original-code inspection and bounded diagnostic logs
were used instead. Temporary pointer-state dumps were removed from the source. Peeking the AppKit
queue was rejected: it added polling overhead and could still race with a click
arriving inside `WaitNextEvent`, selecting the previous position.

## First measured scenarios and catalog implementation

The unoptimized diagnostic build (`0575e7fe` before bundle signing) loaded the
reference in 4.367338 seconds (4.337727 CPU seconds), from the confirmation press
to the last managed-club frame. It showed Cruzeiro on 11 January 2002 and the
preserved 4-0 result against Anapolina. Continuing from that result screen reached
18 January in 46.139403 seconds (45.723206 CPU seconds). The latter run included
a separate 10-second sampling profile and is **not** a clean timing baseline.
Do not infer a speedup from either number.

That profile contained 1,709 main-thread samples, including 420 self samples in
`Memory::check`, 195 in `State::step`, and 111 in `stat`. Repeated volume queries
recursively enumerated and classified all visible entries before file opens.

`NativeDirectoryCatalog` now retains the merged original/private listing, decoded
CoreFoundation name index, and non-symlink entry types. Every query checks both
physical directories' device, inode, mode, and nanosecond mtime/ctime. It rebuilds
on change and does not retain a scan that overlapped a directory mutation. Volume
counts still visit all visible directories; symlink targets are classified afresh
and never recursively followed. Free space and individual file metadata remain
live. Original canonical containment checks, case/canonical-accent comparison,
private replacement precedence, and byte-sorted enumeration remain in force.
This cache targets local APFS directory metadata; remote filesystems with weak
metadata consistency have not been validated.

The 19-test suite passed in 40.17 seconds after this change, including external
nested additions with restored mtime, rename, removal, file-to-directory
replacement, and changing/dangling symlink targets. This verifies contracts, not
real-game speed. Matched before/after runs remain required.

Autonomous operation currently reaches load and day advancement. After a UI tool
reported `noWindowsAvailable` for coordinate clicks (while screenshots and
keyboard still worked), selecting the native File/Save menu reached the existing
`Native menu selections require original MenuSelect dispatch` stop. The isolated
run ended before saving that week's progress; the immutable reference was
unchanged. On-screen game controls and native menu actions are distinct paths;
do not claim save/reload validation from a submitted menu action.

### Host free-space overflow discovered during repetition

A later unoptimized run displayed Disk Full despite approximately 88 GiB being
available. Read-only inspection of original routine `0x1073afc0` showed a 32-bit
`free_blocks * block_size` product followed by a 20-bit shift. The former bridge
represented the entire modern disk: 12,525 free blocks of 7,544,832 bytes wrapped
to only 9 MiB. This was a pre-existing capacity-dependent failure, not a catalog
cache regression.

`legacy_volume_geometry` now exposes a conservative Classic logical volume of
at most `UINT32_MAX` bytes, rounded down to complete allocation blocks. Neither
total nor free represented bytes exceeds the backing store, and both products
fit the original 32-bit arithmetic. This does not reserve disk capacity, increase
free space, or change save contents. Tests cover empty, small, 4 GiB, 460 GiB,
and 8 TiB host volumes, including zero free space.

Restart the matched baseline with this same compatibility fix in both builds.
The preserved `baseline-space-01` source binary is `75449a1e6b6d0b01df9d62c8c9da12e80c96b6ae9137584c396f1e8c594389d4`.
It retains the old uncached directory implementation. Do not count this
correctness fix as a catalog performance gain.

### Initial matched pair (not a completed performance claim)

Both builds used the free-space compatibility fix and identical reference-save
copies. No sampling profiler or build ran during these load/advance intervals.
The baseline app's opening overlapped another menu-only process briefly and is
excluded from launch timing. That process had exited before loading started.

| Scenario | Uncached wall / CPU (s) | Catalog wall / CPU (s) |
| --- | ---: | ---: |
| Load reference to club | 4.441882 / 4.397806 | 2.872859 / 2.815808 |
| Continue 11 to 18 January | 45.924438 / 45.933168 | 34.559225 / 34.521895 |

The observed wall reductions were 35.3% and 24.7%, respectively. These are one
pair of runs, not medians or guaranteed gains. The optimized source executable
was `4912eb0030aed3a484e582c937d927f0113dbbe6fce8437452d9d39da5267f51`.
Both reached the same date and retained the reference's 4-0 Anapolina result,
but generated news differed (including an injury in the optimized run). Random
and clock inputs are not yet controlled, so this is not deterministic simulation
parity. Repetition, new-career/match scenarios, and save/reload remain outstanding.

Both sessions encountered the UI driver's coordinate-click failure after the
long simulation. Their processes were suspended rather than restarted, retaining
the unsaved test state in memory. No personal save or installed executable was
changed. A diagnostic-only native Validation menu now provides an explicit
coordinate dialog and queues an ordinary unmodified left press/release through
the same event bridge. It does not invoke game functions, alter state directly,
or appear in normal installed launches. Its live end-to-end validation is pending.

The diagnostic menu successfully opened the real load screen through its
coordinate dialog in `catalog-controls-01`. After loading and opening Last Game,
the next week-advance attempt stopped producing frames after opening
`events_por.cfg`, with CPU busy in generated `page_10774`. A three-second sample
recorded 521 main-thread samples in that path. The cause is not established;
this run is excluded from timing claims and save/reload remains unverified.
The prior complete suite passed 19/19 in 36.67 seconds; after the final diagnostic
window-selection fallback, the three affected input groups passed in 2.38 seconds.

### Allocator non-progress diagnosis

A bounded read-only LLDB inspection succeeded on the stalled diagnostic process.
Its copied memory showed a 213-arena circular list that returned to its head,
with valid occupied block boundaries. One full arena had no free-list head but
retained a `0x7b0` largest-block hint. Original `0x10774190` returned failure for
that arena without revising the hint. Original pool replenishment `0x10774b60`
then recomputed the same 40-object attempt repeatedly: the failed request was
rounded to `0x7b0`, and the reported `0x7a8` usable hint recomputed the same count.
The source of the stale hint is not established; the non-progress mechanism is.

The new standalone allocator fixture reproduced this with unmodified translated
instructions: a 44-byte object request exhausted 500,000 instructions inside the
allocator. This fixture does not use the directory cache or native UI, separating
the failure mechanism from those additions. Private process snapshots remain
under `.local/`; no save data or captured heap contents are versioned.

The generator now audits the complete failed-allocation branch around
`0x10774c24` and calls `next_pool_attempt` there. Every already-decreasing retry
is unchanged. A repeated or increasing count is capped at the previous count
minus one, so the original bounded-pool loop can reach its existing arena-growth
fallback. Object sizes, pool layout, split/free routines, simulation, and random
state are untouched. This is a targeted allocator progress repair, not a bypass
of allocation failure or memory checks. Live validation remains required.

The repair passed 16 synthetic original-pool replenishment cases and all 20 test
groups (45.14 seconds). Replaying the captured real heap's 44-byte allocation in
a separate offline process also returned successfully after 27,453 original
instructions (0.000177 seconds locally), without changing the live stalled
process or patching a save. This is evidence for the allocator repair, not a
whole-game performance measurement. The source binary for the ensuing live
validation is `e3004b88d056c29553d5efb42771d32564b8bd538d20baf95d0288bc28c70fa3`.

### Autonomous save/reload validation after the allocator repair

The isolated `allocator-controls-01` build loaded the immutable reference copy,
advanced from 11 to 18 January, saved through the original in-game Save control,
exited normally, and reopened `Perf-Autonomous-18Jan`. Visual inspection after
reloading confirmed 18 January 2002, the Cruzeiro squad, and the preserved
Cruzeiro 4-0 Anapolina Last Game screen. The diagnostic coordinate dialog and
ordinary keyboard input completed this cycle without physical user clicks.
The private save is 18,313,800 bytes, SHA-256
`df983a9147903200fedb33bd9ade62566b8c898ef08472703b326cc4b0a74918`.

This supersedes the earlier pending save/reload and diagnostic-control status.
It verifies preservation of an existing result; a newly played complete match,
new-career creation, repeated performance comparisons, and installed-app delivery
remain outstanding. The three older diagnostic processes were terminated after
preserving the stall snapshot and verifying this saved session. Keep only one
test instance open at a time. The normal installed application remains unchanged.

### Indexed scalar memory bounds (under real-game evaluation)

`Memory` now indexes exact mapped interval ends by 64 KiB address prefix. An
entry is populated only when the entire interval from that prefix boundary to
the stored end is mapped. Scalar reads/writes can validate directly against
that end; partial starts, holes, and other misses retain the original merged
byte-range check and diagnostic. Host page permissions are not used as bounds.
The index is 512 KiB per `Memory`, allocated separately, and mapping additions
refresh only their merged interval. The 32-bit address limit is checked before
indexing; invalid mapping lengths are rejected without overflowing addition.

Instruction accounting still updates the PC and increments the count on every
instruction, including the failing instruction. Only exception construction
moved to a cold function so it does not inhibit inlining the normal path.

All 20 test groups passed in 41.21 seconds. The memory check includes 16,899
byte-oracle/boundary cases, scalar read/write agreement, fragmented and merged
pages, exact top-of-address-space bounds, wrapping lengths, big-endian payloads,
and budget failure state. The synthetic six-mix read benchmark fell from
1.02–2.34 ns/read to 0.56–0.71 ns/read; this is not a game speedup claim.
The candidate source executable is
`d26b3f05af78ee06df54f1e9ec0699b0613d68fe9ade64236f6ddd66309bdb6c`.
Real before/after validation remains required.

The first real comparison of this candidate used two private runtimes copied
from the same prepared state before either run. Only one game process ran at a
time, with no build, test suite, debugger, or sampler active. The prior binary
already includes the catalog cache and both compatibility repairs.

| Scenario | Prior wall / CPU (s) | Indexed wall / CPU (s) |
| --- | ---: | ---: |
| Load reference to club | 2.948221 / 2.855679 | 2.921694 / 2.809456 |
| Continue 11 to 18 January | 32.253948 / 32.257398 | 27.957500 / 27.993896 |

Both visually reached 18 January and saved to separate private files. This one
pair suggests a 13.3% additional reduction in week advancement and no material
load improvement. It is not a median, deterministic parity, or completion of the
targets: generated news differed, and week instruction counts were 10,000,367,818
and 10,229,649,356 respectively. Repeat matched runs before accepting the gain;
reopen the candidate's newly saved file and complete a new match as well.

A private pre-match save was created through normal game controls on 10 February
2002 afternoon, before Cruzeiro–Uberlândia. Preparing it overlapped compilation
and is excluded from performance measurements. It will provide the same starting
point for complete-match comparisons; no match was skipped or state patched.

### Three matched memory-index repetitions

The prior executable already includes directory caching and both compatibility
repairs. Three prior and indexed runs used separately prepared copies of the same
private runtime. Pair 02 and 03 inventories matched across 11 files before launch.
Only one process ran at a time, with no build, test, debugger, or profiler during
load or advance. Each run loaded `Perf-Reference` and advanced from 11 to 18
January 2002; the final date was checked in the game. Times below are seconds,
from the load confirmation or Continue mouse-down to the final frame before the
next input. CPU is process user plus system time.

| Run | Load wall / CPU | Week wall / CPU |
| --- | ---: | ---: |
| Prior 01 | 2.948221 / 2.855679 | 32.253948 / 32.257398 |
| Indexed 01 | 2.921694 / 2.809456 | 27.957500 / 27.993896 |
| Prior 02 | 2.842648 / 2.775514 | 32.599281 / 32.591526 |
| Indexed 02 | 2.937470 / 2.834165 | 27.356915 / 27.408155 |
| Prior 03 | 2.958304 / 2.838562 | 33.322352 / 33.294147 |
| Indexed 03 | 2.909420 / 2.801626 | 28.183736 / 28.177891 |
| **Prior median (range)** | **2.948221 (2.842648–2.958304)** | **32.599281 (32.253948–33.322352)** |
| **Indexed median (range)** | **2.921694 (2.909420–2.937470)** | **27.957500 (27.356915–28.183736)** |

The median week reduction is 14.2% relative to the preceding repaired/cached
build. The load difference is 0.9%, too small to establish a material gain.
These are warm local measurements; presentation timing, new-career creation,
match processing, and a more substantial end-to-end gain remain unresolved.
News and instruction counts varied, so the runs do not establish deterministic
simulation parity. Prior 03 and indexed 02/03 saved to separate private files
and exited normally; save/reload validation is recorded separately.

### Newly played match and reloaded result

The indexed diagnostic build loaded the unchanged private
`Perf-Pre-Match-10Feb.sav` (10 February 2002 afternoon). The original squad
screen reported three selected foreign players where the competition permits
two. One foreign substitute was deselected in the normal squad UI; the game
then accepted the six remaining substitutes and started Cruzeiro–Uberlândia.
The original match reached half time at 1–0, resumed through its normal control,
and ended 3–0 at minute 91. The post-match afternoon results and return to the
Cruzeiro squad were visible. The in-game Save control wrote a separate private
`Perf-New-Match-10Feb.sav`, 18,969,336 bytes, SHA-256
`09d615bce103dc81d7071b491e69794cf030ea704ad69c74763c6b71d3146cfb`.
The pre-match source retained SHA-256
`ac167b19877e04f6998301acc145d571d010c6a1caa453a58e6c6c1b1b5becdc`.
After a normal exit, reopening the new save showed 10 February 2002 and
Cruzeiro 3–0 Uberlândia in Last Game. This validates a newly completed match
and its persistence for the indexed build; no save or result was patched.

The match used the default commentary speed and required a normal half-time
resume. Its two uninterrupted play intervals were 14.371075 and 44.852616 wall
seconds (14.391820 and 44.955538 process CPU seconds). This is one run and
does not establish a match baseline distribution or an optimized match speed.

### Hover delivery repair under evaluation

The native view did not enqueue AppKit mouse-moved events. In the diagnostic app,
clicking the sidebar's Competitions menu exposed entries with child arrows, but
an attempted click on a parent dismissed the menu. The view now accepts and
forwards mouse movement as the original `osEvt` record. Normal input continues
to query the physical pointer; the isolated validation app also offers
`Validation > Move to position…` to deliver a move without pressing a button.
Both input test groups pass, including movement event fields and pointer policy.
In a newly built isolated app, moving onto `Campeonato Mundial` opened its child
menu and choosing `Campeonato do Mundo` opened that competition screen. The
normal installed app has not yet been updated or physically tested for this fix.

### Match-speed experiments and isolated replay

The in-game commentary speed was already set to `Muito Rápida` in the fixed
Cruzeiro–Uberlândia pre-match save. The separate native `Match > Turbo` menu
currently selects an experimental translated-code execution path. It does not
change the original half-time, injury, dismissal, or tactical-decision stops.
The first two page-accounting variants showed no established speed gain in live
matches, so this menu is not a completed performance feature.

A further experiment cached the 32 general-purpose PowerPC registers across
three frequently sampled match pages. Its isolated game completed the match
0–0, stopped at half-time as normal, returned to the club, and wrote a private
post-match save. After a normal exit, reopening that save showed the 0–0 result
in Last Game. The first and second uninterrupted halves took 23.968607 and
34.075905 wall seconds, with 3.271780647 and 4.587626794 billion translated
instructions respectively. The different event sequences and instruction
counts prevent a same-work speed claim; the candidate provided no convincing
benefit and the register cache was removed from the generator. The pre-match
source hash remained `ac167b19877e04f6998301acc145d571d010c6a1caa453a58e6c6c1b1b5becdc`.

A separate profile used a sparse original-PC counter rather than a timing run.
It found visits in the interface-control page `0x1071a`, event loop `0x1073c`,
and PowerPC 64-bit division helper in `0x10773`. The counter's threshold includes
instructions outside these three pages, causing catch-up bursts upon reentry,
in addition to periodic aliasing. These counts are not unbiased CPU percentages.
Use a separate host sampling profile to choose optimization targets.
The current counter rebases an overdue threshold upon page reentry and saturates
increments at `UINT64_MAX`, preventing those catch-up bursts. Tests cover cold
page gaps and counter overflow. Selected-page coverage and periodic aliasing
still prevent interpreting these diagnostic counts as CPU percentages.

A private capture reproduces a bounded interface slice without opening the game
or drawing frames. It was initially misidentified as match-engine processing.
The captured stack is `0x1071a828 -> 0x107194a0 -> 0x1073cb20 -> 0x101cb9a4`;
the fixture index is -1 and the watched-fixture pointer is null. Original
`0x1071a280` traverses control lists and calls rectangle hit-testing at
`0x1071c190`. The block at `0x1071a828` is therefore a **UI control cursor**,
not selection of football actors. The older Settings investigation also visited
this address before a career existed.

Two independent 4,000-instruction replays stopped at original address
`0x1071a834` with matching register hash `bdd09a7140975110` and memory hash
`7511b2eb3430aedc`. A 100,000-instruction request stopped at the first native
import after 4,379 instructions, address `0x10bf7c34`. Normal and experimental
direct-cursor replay states matched at 1, 10, 100, 1,000, 4,000 and 4,379
instructions, comparing stop reason, address, instruction count, full register
hash and mapped-memory hash. This verifies the captured interface slice only.
The capture and save remain under ignored `.local/performance` paths.

The private CLI replay stops at the first native import and is not a whole-match
headless simulation. In-game event injection is deferred until the performance
work is complete.

Parallelizing the simulation has not been justified: the current guest code
shares mutable match state and random draws, and changing their order would
change the game.

### Scheduler hypothesis corrected and saved-result validation

On 2026-09-27, the `match-fullrate-on-01` post-match save was reopened through
the original load screen. The game showed the Cruzeiro squad on 10 February
2002 afternoon and Last Game showed **Cruzeiro 1-3 Uberlandia**. The save retained
SHA-256 `599f1a940343dfa78b70f59a897887de4a96a8c016b24daa956015c23e709f61`;
the validation session exited normally. This completes that run's previously
pending persistence check, without establishing a Turbo speed gain.

The corresponding `match-fullrate-off-01` run used the unchanged initial save,
SHA-256 `ac167b19877e04f6998301acc145d571d010c6a1caa453a58e6c6c1b1b5becdc`.
Normal squad actions removed Sorin from SB6 and Luisao from SB1, then assigned
Luisao to DE in place of Cris, and accepted five substitutes. Commentary speed
was visually confirmed as Muito Rapida; the native Turbo menu was unchecked.
The game reached half-time 2-0 at minute 45 and full-time 2-0 at minute 93.
No injury/dismissal decision occurred in this run. Its two uninterrupted play
intervals were 26.023697 and 27.019206 wall seconds (26.062837 and 27.073462 CPU
seconds), totaling 53.042903 wall seconds and 18,845,932,454 original instructions.
The in-game save wrote a separate `Perf-Fullrate-Off-01.sav`, SHA-256
`2a00a038af7b2f9e79558b53e550e5acc318ee7f90db98019f3b572810d441bf`, followed by
normal exit. The initial pre-match copy stayed unchanged. Reopening this new
off-run save subsequently showed 10 February afternoon, the Cruzeiro squad, and
Last Game Cruzeiro 2-0 Uberlandia. A normal exit followed and the saved file hash
remained unchanged. This reload ran alongside compilation and is persistence
validation only, not a performance sample.

The on-run had injuries and extra tactical interaction. Its log does not label
match minutes, so its entry-to-last-frame intervals cannot all be assigned to
play versus tactical processing retrospectively. All intervals 12-28 total
56.559103 wall / 56.752663 CPU seconds; the full span including interaction is
126.865792 wall / 123.581719 CPU seconds. The difference demonstrates busy
execution while waiting for actions, not 70 seconds of additional simulation.
The two runs differ in events and work; neither establishes a speedup.

Source audit explains why the scheduler experiment was misdirected. The host's
`Gestalt('pclk')` returns -5551, and original `0x1073af50` selects hardware class 2
on failure, setting flag `0x80000`. However, `0x101cd5c0` rate-limits **background
fixtures** selected by the manager's index. The watched-fixture path in
`0x101ccdb0` calls `0x101d3f60` directly, outside that gate, and invokes background
work while waiting. Clearing `0x000c0000` therefore is not an established way
to accelerate the watched match and can increase competing work. The experiment
must not ship on that premise. The earlier captured flag was from interface
setup, not proof of the watched engine skipping five of six advancement steps.

### Broader instruction translation investigation

A separate five-second host CPU sample during the Jan 11 to Jan 18 advance
in `cpu-week-01` recorded 3,943 main-thread samples, including 3,791 in generated
execution. Work was spread across simulation pages (leading page `101df`: 349
samples), rather than dominated by the UI cursor. This run included profiling
and is not a clean timing measurement. It reached Jan 18, saved through the
normal interface as `Perf-CPU-Week-01.sav` (SHA-256
`8a86f17c0a6fb26501685154e75aa71f38f0302cd47a728c9c17a8d47505f5a3`), and exited
normally after 74,402,370,805 original instructions.

The next candidate lowers instruction-local P-code scratch into immutable C++
locals, preserving operation order, narrowing boundaries, unsigned expression
widths, and big-endian overlapping scratch slices. A complete export audit
proves that all 1,953,048 instructions define their scratch inputs locally;
generation rejects incoming scratch reads or internal P-code control flow.
Architectural registers and game memory must remain equivalent. Dead translator
scratch bytes are deliberately no longer materialized and are not architectural
state; `--legacy-unique-storage` retains the previous lowering for comparisons.
Repeated loading and weekly advancement measurements are recorded below;
new-career and match lifecycle validation of this candidate remain pending.

Local verification commands (the original instruction database is private):

```sh
python3 tools/test_local_temporaries.py .local/recomp-input/instructions.sqlite
python3 tools/test_generated_temporaries.py .local/recomp-input/instructions.sqlite --sanitize
python3 tools/test_control_cursor_audit.py .local/recomp-input/instructions.sqlite
python3 tools/test_unsigned_division_audit.py .local/recomp-input/instructions.sqlite
python3 tools/test_match_presentation_audit.py .local/recomp-input/instructions.sqlite
ctest --test-dir .local/aot-build --output-on-failure
```

The first compiled comparison covers 2,080 synthetic aliasing, truncation,
evaluation-order and exception cases under UndefinedBehaviorSanitizer. The
actual-export comparison selects 2,050 normalized instruction patterns spanning
all 46 exported P-code operation kinds; it compares full architectural register
state, mapped memory, exceptions, PC and instruction count. Test discovery
explicitly skips export-dependent audit classes when their private fixture was
not supplied, rather than counting empty loops as verification.

The compiled actual-export comparison passed 199,742 executions under UBSan:
57,054 normal completions, 435 transfers and 23,247 matching memory failures,
with 29 edge-value seeds, unequal initial scratch contents, finite budgets and
counter overflow. All 25 CTest groups passed after adapting the presentation
test to its architectural contract: incoming scratch is poisoned, and full
register/memory/PC checks remain, while outgoing dead scratch is not asserted.
These results establish translation checks, not real-game performance gains.

### Instruction-local temporaries: three real-game pairs

Six separate app/runtime copies ran in alternating before/after order, each
starting from the immutable reference save and the same source runtime. Native
Turbo was off. No build, test process or profiler ran concurrently, and only one
game instance ran at a time. All six loaded the Jan 11 squad, advanced to Jan 18
news, saved to separate new private files, and exited normally. This is a warm
filesystem comparison; it is not a cold-disk benchmark or a reload verification
of those newly written saves.

| Scenario | Before, seconds (three runs) | After, seconds (three runs) | Median reduction |
| --- | --- | --- | --- |
| Startup, opening presentation retained | 23.562 / 23.538 / 23.558 | 23.111 / 23.138 / 23.151 | 1.8% |
| Reference load | 2.951 / 2.982 / 2.901 | 2.332 / 2.368 / 2.355 | 20.2% |
| Jan 11 to Jan 18 | 28.450 / 28.699 / 27.353 | 19.497 / 19.926 / 19.136 | 31.5% |

Weekly CPU times were 28.475 / 28.732 / 27.380 seconds before and
19.506 / 19.944 / 19.140 after. Original instruction counts varied with the
simulation: 10.144 / 10.263 / 9.856 billion before versus
10.259 / 10.447 / 10.257 billion after. Thus the improvement is not explained by
doing fewer original instructions. Each weekly interval presented 78 frames;
each load presented 198. Load CPU times were 2.806 / 2.845 / 2.793 before and
2.199 / 2.221 / 2.199 after. The week meets the provisional 20-second target in
all three runs; reference loading remains above its 2.2-second target.

Load/week intervals start at delivered confirmation/Continue input and end at
the final frame before the next action, with the completed state visually
verified. Startup ends at the last frame before the first input; its clock
starts at platform construction after program-segment verification, so it does
not include the entire process-launch path. The original opening presentation
was not skipped. Its small improvement does not establish drastic startup
acceleration.

The before source executable SHA-256 was
`7bc9dba516d3b03f1e9aa174204f518af56749e51a70cb00fe068031bdf6577f`;
the after source was
`dccad5585e195f69c89edc12d43f4a1df975149a9e8a05b81a18f587f7afb572`.
Re-signing isolated bundles changes their hashes; private manifests retain both
source and signed identities. Besides local temporaries, the candidate contains
the mouse-move flush correction and sparse-profile threshold correction. The
profiler was disabled, but this is a build comparison rather than a formally
isolated single-change experiment. Private raw results are retained locally;
no personal saves or run logs are published. The installed app is still the
previous version.

Read-only original-code inspection explains most of startup's remaining wall
time. Routine `0x101317c0` starts INTRO1 and waits for completion at
`0x10131a84..94`, then starts INTRO2 and waits at `0x10131b94..bd0`. The logged
audio lengths are 201,380 and 150,312 frames at 22,050 Hz: 9.132880 and
6.816871 seconds. Two subsequent 2,500 ms holds occur at `0x10131c70..98` and
`0x10131d24..80`. Their combined presentation floor is approximately 20.95
seconds. The six paired logs also retain exactly 774 fade-related frame events.
The CPU fade renderer at `0x10769460` has no timed delay; its pixel loop at
`0x10769660..88` is a possible future optimization while retaining every step.
Observed fade spans total about 1.89 seconds in the first candidate run, partly
overlapping audio. Cutting the introduction would change presentation rather
than accelerate equivalent work, so it is not included in this comparison.

### Register-access inlining experiment

Inspection of the current Release ARM64 object for generated page `0x101df`
found 665 calls to `State::r<8>`. Its register offsets are constants in generated
code, but the compiler retained calls and dynamic checks. A private compilation
with `always_inline` on `State::r` and `State::wr` eliminated all 665 calls and
reduced that page's text from 147,524 to 135,276 bytes (8.3%). The bounds checks
remain in source and still apply to dynamic offsets. Only those two attributes
are applied in the next candidate. The complete ARM64 rebuild succeeded and all
25 CTest groups passed (50.56 seconds). Its source executable SHA-256 is
`68711279182f82afc164a5adc36c1c2f93b61dee71b2a8f1ca7350022e4b0956`.
An isolated app was prepared; game measurements remain pending because macOS
locked during the earlier new-career baseline. The installed app is unchanged.

Moving exception construction to a cold function alone retained all 665 calls.
Forcing memory read/write inlining increased text to 149,824 bytes and mostly
moved calls to `check_scalar`, so it was not adopted. Existing endian byte loops
already compile to ARM64 loads/stores and byte reversals. These observations
describe generated code shape, not a measured game-speed improvement.

Review also corrected an overbroad generator guard: unrelated partial exports
from the division/commentary pages no longer require an unused complete helper.
Selecting its relevant entry/call/load still requires the entire fingerprinted
context. Seven division, six presentation and five cursor audit tests passed.
Generated text for all four affected complete pages was byte-identical before
and after this correction in both scratch modes (eight comparisons), so this
tooling correction does not change the full-game candidate.

### New-career persistence recovered on 2026-10-01

The later `career-local-before-01` log supersedes the paused checkpoint: the
normal Save operation renamed its temporary file to `adsasdas.sav` successfully
and the application exited normally. The private file has SHA-256
`3f4ef38c285e34077b6574321455f247c8c2997403550f0810773c16a740ba66`.
A separate runtime copy reopened it in the register-inline candidate. Visual
inspection confirmed manager **Perf Career**, Cruzeiro's accessible squad, and
**28 December 2001 morning**, later than the appointment originally observed on
21 December. Selected Leagues showed Brazil and Portugal selected, real players
enabled and attribute masking enabled. The validation exited normally and the
save hash remained unchanged. This establishes persistence of the previously
created career, not a new repeated career-creation measurement.
