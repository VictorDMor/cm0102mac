# Real-game performance validation

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
