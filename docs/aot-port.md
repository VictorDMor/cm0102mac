# Native recompilation of the original Mac program

This pipeline produces the playable ARM64 development build described in the
[current port status](native-port-status.md). It recompiles the original Mac
program and progressively replaces generated routines with reconstructed C++23.
The separate Windows reconstruction has 32-bit pointer assumptions and unfinished
gameplay/UI functions; it is not the active game build.

The implementation notes below retain historical checkpoints. Their individual
verification limits describe the stage recorded there; consult the current status
and subsystem reports for subsequent gameplay and integration evidence.

## Build pipeline

1. Ghidra imports the exact audited Mac PEF with `PowerPC:BE:32:default` and resolves
   its internal relocations. `ExportRecomp.java` exports the analyzed instructions,
   raw P-code, register layout, symbols, and relocated initialized segments.
2. `index_recomp.py` creates a local SQLite build index. It verifies the instruction
   count and records the compressed export's SHA-256.
3. `compile_recomp.py` translates the selected original instructions into C++.
   `--all-analyzed` streams the entire analyzed listing with bounded memory.
4. Apple Clang compiles the generated C++ and native support functions for ARM64.
   Generated code transfers directly between labels within a code page; a native
   dispatcher handles transfers between pages.

The executable contains no instruction decoder, runtime code generator, or CPU
interpreter. It never grants execute permission to the original program's bytes.
It retains a 32-bit **data address space** so original pointers, structures,
tables, and stack values keep their meaning. ARM64 instructions operate on that
data through checked big-endian reads and writes. This is static recompilation,
not recovery of the developer's original C++ source.

All generated code, binary inputs, oracle outputs, and compiled programs stay in
`.local`. They are derived from the user's supplied game and are not distributable
project source files.

## Reproduce

After running the media analysis described in the main README:

```sh
JAVA_HOME='/path/to/jdk/Contents/Home' bash tools/build_aot.sh '/path/to/ghidra'
```

The first full build compiles 1,922 translation units and takes several minutes.
Subsequent builds preserve unchanged generated files. The script checks the
original routines and the native bridge; it does not launch a game session.

To inspect the current startup boundary separately:

```sh
CM_AOT_INSTRUCTION_BUDGET=10000000000 .local/aot-build/cm-aot-probe .local/recomp-input 0x10773710 \
  "/Volumes/CM 01-02/Championship Manager" "$PWD/.local/runtime"
```

An unimplemented OS service or missing original address produces an explicit
diagnostic and nonzero exit. This is expected while the port is unfinished.
There is no fallback that returns fake success or interprets missing code.

## Loader and native services

The audited executable's entry transition vector is at initialized data offset
`0xa0c4`. It contains entry `0x10773710` and table-of-contents value `0x108bba90`.
The current loader is intentionally specific to that executable's SHA-256.
Relocated segment payloads are checked against their build-time SHA-256 before
they enter the data address space.

Ghidra's PEF import placeholders are four bytes each. The real CFM call sequence
needs an eight-byte `{entry, TOC}` descriptor. The loader resolves all 259 exact
`.TVect` pointer slots to separate descriptors. It does not scan and replace
arbitrary matching words in the game's data.

Implemented services cover pointer/handle allocation, native Munger text editing,
process information, Classic file specifications and catalog/volume queries,
private preferences folders, resource loading/writing, native menus, cursor
initialization, key-state queries, page-format persistence, and launch-event callbacks. Unsupported services stop explicitly. Original 32-bit
records are marshalled field by field; host pointers are never truncated.

The native Resource Manager opens the original fork read-only. Its automatic
byte-order conversion for known resource types is reversed on copied payloads;
all 138 resources match the original bytes. Preference resource files are created
with native macOS APIs under `.local/runtime/Preferences`. Their Finder metadata,
resource-chain lookup, writes, byte order, removal, and close/reopen behavior have
been checked on real files. Data-fork opening, reading, seeking, EOF reporting,
and closing now use native macOS fork APIs. Checks cover partial reads, zero-byte
reads, file marks, invalid references, and reads crossing the temporary buffer
boundary. New logical game directories and files use private backing under
`.local/runtime/Files`, with the original volume and directory IDs preserved.
Lookups and enumeration merge that backing with the read-only supplied files.
Synchronous data writes, resizing, EOF marks, and close/reopen are now implemented.
Opening an original file for writing copies it into private backing first. Checks
verify actual persisted bytes, chunk boundaries, Finder metadata, and preservation
of the original. These checks do not establish complete game save/reload support.

A real startup regression exposed a volume-name parsing error: the original
runtime passes `CM 01-02:Data:language.ldb` when asking for its volume. Matching
the whole path as a volume name returned `nsvErr` and led to the game's own
`GAME_CONFIG..CPP 84` failure. The corrected lookup selects the volume prefix;
the original program now reads `language.ldb` and updates its menu text. Relative
names use the requested/default volume. Font loading also uses `fsCurPerm` to try
a resource fork before reading a TrueType data fork; this permission is mapped to
read-only access for supplied assets. Resource writes are rejected before changing
cached data when the native file was opened read-only.

AppKit builds the original menus from `MENU` and `MBAR` resources. Five menus and
30 original item texts have been checked against actual native menu objects. The
startup's duplicate Quit-command lookup, deletion, and localized title updates
also pass. A real AppKit launch notification is converted into an AppleEvent and
invokes the original recompiled launch routine. The callback test checks its four
instructions, the expected game-state flag, and preservation of caller registers
and temporaries. Mouse/keyboard/window events now reach the original handlers;
open-document marshalling and queued menu actions still stop explicitly.
Host CLI input paths are excluded from AppKit's launch-time document events.

Printing checks cover native object lifetimes and page-format serialization.
Startup restores the serialized format on subsequent runs. The private `PFor`
resource uses an identified native representation; old flattened formats trigger
the original fallback to defaults. Print dialogs and print jobs are not verified.
One remaining compatibility detail is the original startup's `PMRelease` call:
it passes the address of the session slot rather than the opaque session token.
The bridge returns an invalid-object error and retains ownership until cleanup;
it does not pass that original address to the native API.

Graphics now include native connected-display metadata, the usable desktop region,
plain AppKit windows, original window-title strings, and original paint callbacks.
The desktop region preserves gaps between displays. A new-window invalidation is
queued even when Quartz defers physical drawing; the original background handler
paints its native surface and restores the previous drawing port. Actual startup
also invokes the original main-window draw handler. Mouse/wheel handlers receive
native input; physical interaction with a playable game remains unverified.

Original GWorlds own checked RGB555/ARGB pixel storage. `GetPort`/`SetPort` select
real drawing surfaces; `PaintRect`, equal-size `CopyBits`, and port flushing feed
Quartz presentation. Tests check nonzero origins, row padding, RGB555 red/green/white,
overlapping copies, and native window callback drawing. A wide desktop background
uses RGB555 storage where a 32-bit QuickDraw row would overflow its 14-bit limit.
Quartz receives a converted 32-bit image. Unsupported depths, scaled copies, and
additional raster operations still stop explicitly. The supplied color cursor and
three monochrome cursors are decoded with their masks/hotspots into native cursors.

The original program creates its background and 800-by-600 main windows. Retained
frames now show the Sports Interactive opening logo and the original Game Settings
interface, including text and controls. These are original-code rendering results,
not asset-viewer mockups. Physical on-screen display, interaction, and gameplay
still require separate verification.

## Verification and limits

The build-time oracle executes the original PowerPC bytes using Ghidra's
`EmulatorHelper`; this dependency is absent from the native executable. The cases
also require independently calculated arithmetic results, so agreement between
two consumers of the same P-code is not the only check.

The initial 297 cases cover five original routines: stack-chain initialization,
table-of-contents retrieval, signed/unsigned 64-bit division, and conversion from
double to unsigned integer. They compare 77 architectural register values,
instruction counts, and selected stack contents. These cases passed on ARM64.
They do not establish equivalence of the entire simulation or every floating-point
operation. In particular, the oracle is an analysis-tool reference, not a capture
from original PowerPC hardware.

The expanded exported listing has 1,953,048 instructions. This count is analysis
coverage, not proof that all indirect function targets and embedded code were
recognized. Unsupported vector intrinsics and floating operations without a
verified lowering trap explicitly. Native windowing, filesystem/resource services, input, audio,
and networking remain substantial work.

The full listing compiles into a Mach-O ARM64 executable. Its dependencies now
include AppKit, Carbon, AVFAudio, CoreServices, ApplicationServices, CoreGraphics, Foundation,
and the system runtime libraries. The Carbon dependency supplies existing native
64-bit input-state and event-parameter APIs; it does not execute PowerPC code.
No emulator library is linked. The CTest
groups pass, including 297 original-routine cases. Separate checks against the
mounted game verify all 138 native resources and the five native menus. A changed
relocated segment is rejected by its checksum before any translated instruction.

Startup has passed language/font loading, private directory creation, native window
creation, cursor loading, and creation/writing of the private `CM3 Hall Of Fame`
file, which subsequent runs reopen and read. It now also passes clock, pointer,
rectangle-pinning, event-wait, sound, uppercase, and math calls. A 100-million
instruction limit interrupted the real-time opening too early. The diagnostic
launcher now allows 10 billion instructions; the installed app defaults to an
unlimited budget. An earlier limited run stopped at `0x1071a84c` during settings
redrawing after 44.47 seconds. Counts depend on elapsed time and input, not just
fixed initialization work. Two opening sounds reached native render completion
and invoked their original callbacks. The original program wrote and closed the
324,012-byte Hall of Fame file. No game session has been created or restored.

`Install Native Prototype.command` now installs 809 original game/Extras files,
with resource forks and Finder metadata, under user Application Support. It
bundles the original relocated segments with the ARM64 app; Ghidra and the source
workspace are not runtime dependencies. The installer verifies hashes, updates
the app without replacing runtime data, and can run again with the CD absent.
The original opening and main menu were observed on the physical display with
the image unmounted. User mouse input reached new-game league selection.

The original CD check constructs `CM 01-02:Extras:qpr for the cup` from interleaved
strings. Copying the CD's actual Extras folder into the installed logical volume
satisfies this lookup. No check or return value was patched. The user dismissed
the old prompt after installation; the image remained unmounted and database
loading reached "Iniciando Ligas". The retained image and log are in the installed
`Runs/app.YEt2eA` directory. That run stopped at `fctiwz f2,f6`, original PC
`0x1022cd24`, after 109,166,643,823 instructions and 18,338,941 native calls.

The AOT compiler now explicitly lowers `fctiwz` using the documented PowerPC
conversion rules rather than Ghidra's incomplete generic `TRUNC`. Range checking
follows truncation; invalid results saturate, NaNs select the signed minimum,
and sticky exception flags distinguish signaling from quiet NaNs. Enabled
exception handling remains an explicit unsupported boundary. The result's upper
word and FPRF are architecturally undefined; the port retains sign extension and
the existing FPRF. See [NXP's programming manual](https://www.nxp.com/docs/en/user-guide/MPCFPE_AD_R1.pdf),
sections 2.2.3 and D.4.2. Regression checks execute the actual failing original
instruction with signed boundaries, infinities, NaNs, subnormals, and all four
rounding-mode settings. All 96 edge/mode cases and sticky/enabled/alias checks
passed, along with the 297 existing routine cases. The installed app was updated
without mounted media and reopened at the original main menu.

The user's next attempt (`Runs/app.8Wltp6`) passed the previous stop with an
actual quiet-NaN input (`0x7ff8000000000000`) producing the documented low word
`0x80000000`. League initialization then completed and physical screen inspection
confirmed the original "Escolha Nome" trainer-name form, with the CD unmounted.
Subsequent team-selection and managed-game evidence is recorded below.
`CM_AOT_TRACE_FLOAT=1` records exceptional conversion inputs/results;
it is now opt-in because this initialization path generated hundreds of thousands
of diagnostic lines when it was enabled. Subsequent runs use the default-off setting.

### Uninitialised stack reads (2026-10-23)

The original reads stack it never wrote: `cm-world-replay <capture> --timing
--stack-audit` lists 136 such read sites in one 26-country day. Most copy
struct padding into locals, but some leftovers reach game memory. One
example is the struct a caller passes to `0x1007f200` as r4. It is copied
with `BlockMoveData` into a heap record, and a word of it still holds a
register that `register_club_interest`'s prologue saved there earlier. So
leftover stack is a diagnostic only where nothing reads it later. A rewrite
that skips original calls must write the stack image those calls would have
left, or the end state differs. The registration shortcut in
`native/shortlist_recruitment_register.cpp` does this, and
`--check-registration-shortcut` compares its stack image with the
original's for every club it takes.

The leftovers also change results. `--scramble-stack <seed>` replaces each
stale byte with noise as the game reads it. On the day capture that changes
the RNG stream (write 4,845,515). With only some readers scrambled
(`CM_STACK_SCRAMBLE_PCS=lo-hi`), records come out different: for example,
`0x103750e4` read-modify-writes a bitfield byte of an uninitialised local
struct, keeps the garbage bits, and copies the byte into heap records
(8 vs 10). So the original's results depend on its call history through
the stack. Results stay identical only if every rewrite leaves the
original's stack image wherever later code reads leftovers.

## Driving the game (2026-10-21)

`tools/game_driver.py` runs an isolated validation copy (from
`prepare_validation.py --automation`) through `native/automation.cpp`. A
numbered request file in `<run>/Automation/inbox` names an original command,
a control-table index, keys or text, a window snapshot or a memory range. The
game takes it at an original event boundary and reports in `status.json`:
readiness, handler, screen, enabled controls (index, action, flags), date
and the last busy period. Requests never carry screen positions, and with
`CM_AOT_AUTOMATION` unset nothing changes. `profile start` and
`profile stop NAME` sample the guest PC in the running game (the SIGPROF
sampler `cm-world-replay --profile` uses) for `tools/guest_profile.py`.

`capture start NAME` and `capture stop` record a session for `cm-world-replay`
(`<run>/Automation/captures/NAME`): the initial state at the end of the
`WaitNextEvent` call that took the request, every import's inputs and effects,
and the end state at the first OS import after the stop request. A replay ends
there when its transcript runs out, and checks the end state. The recorder
watches memory from the `date_observer` slot, since the drawing observer needs
`memory.observer`; a capture that cannot continue is dropped with a message.

The original main loop calls `WaitNextEvent` with a sleep of 0, about 10,000
times a second, even on an idle screen (100% of one core). A load also runs
from that loop under the start screen's handler. So "ready" means an
interactive context whose loop runs fewer than 2 million instructions
between event calls for a 250 ms window: about 6,000 when idle, hundreds of
millions while loading or processing.

The app always has a memory observer attached: `NativeDrawingObserver`
watches writes to the drawing surfaces' address range. C++ fast paths that
step aside "when anything observes" therefore never ran in the app, only in
benchmarks. Fast paths must instead step aside only for read or boundary
observers, or for a write observer covering a write they skip
(`observes_write`).

Two runs of the same Continue sequence from the same save produced saves
that differ in 145 blocks (fixtures, results, histories). The runs used two
builds whose differences cannot alter a normal run, so the game probably
draws from the clock after a load (unconfirmed). Use captures
(`--season-reference-days`) and `cm-world-replay` for deterministic
comparisons, not real-game saves.

## Original background fallback correction

After league initialization, run `app.8Wltp6` reached the Brazilian club list and
stopped while selecting a team: null data access at `0x100d53d8`, return address
`0x10713af0`. The UI fallback at `0x10713214` explicitly supplies a null club to
the background wrapper `0x10713ad0`. That wrapper eagerly calls the descriptor
getter `0x100d5310`, which dereferences the null club in loaded-database mode,
before the common selector `0x107139c0` can take its default-image path. The
common selector does not use the descriptor for a null club.

The build-time compiler has a guarded correction at the audited call instruction
`0x10713aec`: a null club skips the unused descriptor lookup. Nonnull clubs still
execute the original getter, and the original common/default selectors still
choose the picture and consume random numbers. This is an intentional correction
to the original UI call sequence, not a claim that its null dereference was
bitwise preserved. It does not map address zero, suppress unrelated invalid
memory accesses, or modify the source executable/data. Applied corrections are
listed in the compilation manifest. A native regression fixture exercises the
real compiled routines with two nonempty picture lists, disabled/empty lists,
register preservation, and the original getter's remaining null-access error.

The FIFA rankings renderer exposed the equivalent nation fallback: its call at
`0x10074680` supplies a null nation, and `0x10713004` reaches an eager name getter
at `0x10713a9c`, faulting at `0x100d4ea0` on nation+0x68 before the rankings query.
The same null-only correction now covers that audited site (and the competition
site `0x10713b3c`). The shared selector still chooses the category's original
default image; nonnull getters and direct null-getter faults remain intact.
The background audit also pins all 59 adapter memory-access PCs for block
accounting, so table-driven fault checks retain the exact access instruction
rather than reporting the start of its straight-line run.

Runs `app.rCcF8C` and `app.tCEQVP` passed team selection. The user confirmed
assuming control, and the retained `app.rCcF8C` frame shows the original Cruzeiro
squad screen. Both runs then stopped when opening a player, at the missing
`CarbonLib::num2dec` import, original PC `0x10bf7b10` and return `0x1077a070`.

## Native decimal formatting

The player panel reaches the original printf implementation through `0x1077b720`,
`0x1077b680`, `0x1077a690`, and `0x1077a000`. The last routine requests 32
significant digits from `num2dec` and performs the final presentation itself.
The native bridge uses Apple's existing ARM64 `num2dec`, with explicit CFM
argument marshalling: format in r3, double in f1 (r4/r5 argument slots), output
pointer in r6. Its 42-byte decimal record retains the original big-endian
exponent and unused bytes. Zero, infinity and NaN have an unspecified exponent;
the native implementation leaves it untouched.

The bridge selects the original rounding direction and restores the host floating
environment afterward. Apple's *Inside Macintosh: PowerPC Numerics*, Table E-9,
documents no numeric exceptions for `num2dec`; host flags such as FE_INEXACT
therefore do not leak into the original FPSCR. Formats outside the documented
36-digit bound and non-IEEE mode remain explicit errors. See the original
[Apple manual, chapters 5 and 9 and Table E-9](https://www.vintageapple.org/inside_r/pdf/PPC_Numerics_1994.pdf)
and the installed CarbonCore `fp.h` for the native declarations.

Regression coverage compares 1,064 conversions against the native API across
four rounding directions, signed values/zeros, both decimal styles, subnormals,
extreme magnitudes, infinities, and NaNs. It checks byte order, output guards,
preserved original registers and host floating state. Fourteen literal expected
strings additionally execute the actual original bounded formatter, including
fixed/scientific notation, width/sign options and ties-to-even. This verifies
the stopped formatting path; it does not yet prove the complete player panel or
bitwise equivalence with every historical MathLib implementation.

## Save-file path and finalization

The first actual save attempt in `app.yT8qBP` displayed `LEAGUE.CPP 5009`
while collecting league files. CoreServices had created names such as
`Copa AmE\u0300rica_407.tmp`, whereas the original MacRoman lookup decoded to
`Copa AmÈrica_407.tmp`. The case-insensitive comparison alone did not consider
them equivalent, so the private overlay returned a missing-file error despite
the file being present. Both lookup and merge now also use
`kCFCompareNonliteral`, preserving accents while matching canonical spellings.
Regression checks cover a native-create/catalog/reopen cycle, duplicate-free
NFD/NFC overlay merging, and rejection of an unaccented lookalike.

Ten private temporary filenames were normalized in the still-running session;
inodes, content hashes, resource forks and extended attributes were checked.
The retry passed league-file collection and wrote 17,244,214 bytes, then stopped
at `PBHRenameSync`, original PC `0x10bf7ab4`, return `0x10777c10`.
The bridge now uses native `FSRenameUnicode` for private files/directories. It
preserves existing catalog IDs, descendant directory bindings, open forks,
resource data and Finder metadata, returns real duplicate/missing/name errors,
and rejects original-media renames that need unimplemented tombstones.
The original 32-bit parameter offsets are ioNamePtr +18, ioMisc +28,
ioVRefNum +22, ioDirID +48, with the result written at +16.
See [Apple's File Manager manual, pages 2-198–2-199](https://developer.apple.com/library/archive/documentation/mac/pdf/Files/File_Manager.pdf).

Inspection of the original save routine `0x1034ba30` places its final writes
before the rename wrapper `0x107766c0`; the remaining code updates session flags.
The temporary was backed up as `.local/saved-before-rename.sav`. A copied fixture
then ran the actual original rename wrapper successfully (2,376 instructions,
14 native calls), and the same wrapper completed the real rename to
`Runtime/Files/games/Mac-Teste.sav`. Both forks and Finder metadata were unchanged;
data SHA-256 is `84cfcb7c4ee3eb3aa5e2d731fb3fd8fc295b063ebd1a8dfce67d484deabc861f`.
This candidate's first load failed with `INDEX.CPP 5773`, diagnosed below. A fresh
complete save/reload cycle was subsequently confirmed, as recorded below. The 13-group run passed, recorded in
`.local/logs/save-final-tests.log`; installed build verification is in
`.local/logs/save-install-without-cd.log`.

### Saved-game English index alignment

The `app.ijfweg` load reached the original index verifier `0x1014d560`. Read-only
debugger inspection identified `Unable to find the _1_FC_SAARBRUCKEN index` in
its diagnostic buffer, although the club existed in both the save and loaded
memory. The original English file places Matav-Sopron at slot 8649, while the
supplied club database gives that club ID 5791. The intervening name positions
therefore do not match the database IDs retained in a saved game's mapping.
For example, loaded club 1845 maps to original ID 7959: the old language file
returns Saar 05 Saarbruecken, while the database record is 1.FC Saarbruecken.

The failure is reproducible by replaying the actual index-builder function from
a read-only snapshot. With the original English file it fails first on River
Plate. Moving the single 79-byte Matav record to its database position makes
the entire original verifier return normally: 2,618 club lookups, 54,533,199
translated instructions. No original validation is skipped.

`tools/language_compat.py` applies only to the audited English and club-database
SHA-256 values. The installer creates `Runtime/Files/Data/eng.lng` as a private
read overlay for future new games and leaves verified originals unchanged.
`tools/repair_save_language.py` creates a separate save copy, preserving both
forks/Finder metadata, every other compressed payload, and the original save.
It validates all block boundaries and decompressed lengths and refuses unknown
English data or an existing output. Eight boundary tests pass in
`.local/logs/save-language-tests.log`.

The repaired `Mac-Teste-Recuperado.sav` retains 117 of 118 compressed payloads
byte-for-byte. Both saves are 17,244,214 bytes; repaired SHA-256 is
`122fc3881a4f1cc7b51a356880eb79f653d70b97eea94b0097e9f4dca4b014f5`.
Its original index-builder replay passes (`.local/logs/replay-repaired-save-index.log`).
The actual `app.BAEOk6` run then loaded it to Victor Moraes's Cruzeiro squad on
21 December 2001; the user confirmed player pages now open. A later competition
click reached a separate null background-descriptor failure at `0x100d5084`.
The subsequent ordinary save/reload cycle is now verified: the user saved
`Mac-Teste-Validado` in `app.VDk5TM` and loaded it successfully in `app.pigLIp`.
The saved file has 17,255,573 bytes and SHA-256
`0c048140964e58a8415f2c7cba4cb7bacc80f6594646487401800f6428c45d11`.
All 118 blocks validate, and its embedded English table already has the aligned
SHA-256 `d949309b50c822c337546e04b7eb893507aba3cf0bec9aa95d4ef7f9d8f6ffa1`.
No repair was applied to this ordinary save. The read-only verification report
is `.local/logs/ordinary-save-validation.json`.

### Competition default background

The user opened players and the squad successfully in `app.BAEOk6`, then clicked
the competition. That run stopped at `0x100d5084`, return `0x10713b40`, with a
null competition. The fallback at `0x107133b8` deliberately calls wrapper
`0x10713b20` with zero. Its eager name getter `0x100d4fd0` dereferences it before
the shared selector `0x107139c0` can choose category 3's default image.

The compiler now guards exactly `0x10713b3c`, analogous to the existing club
guard at `0x10713aec`. The getter is skipped only for that null argument; the
original selector, random-number use, and all nonnull lookups remain intact.
The expanded `native/background_checks.cpp` first reproduced the exact live
fault, then passed for primary/alternate default lists, disabled/empty defaults,
nonnull getter results, preserved registers/RNG, and unchanged direct null
faults. All 13 CTest groups passed in 16.25 seconds after regeneration/build.
See `.local/logs/competition-background-before-tests.log`,
`.local/logs/competition-background-tests.log`, and
`.local/logs/competition-background-install.log`. The corrected ARM64 app is
installed; the user confirmed the competition opens in `app.esyws0`.

### Day-advance arithmetic

`Continue Game` in `app.esyws0` stopped at `0x101355d4`, `divw r3,r5,r4`,
with divisor zero. The original floating bound is checked before truncation;
nonzero values between -1 and 1 can therefore reach the integer division with
zero in r4. The previous generic C++ helper intentionally rejected this case.

PowerPC `divw` without OE or Rc leaves its exceptional quotient undefined,
without changing the status fields (see IBM's [assembler reference](https://www.ibm.com/docs/ssw_aix_71/assembler/assembler_pdf.pdf)).
At this specific site, the quotient's only use multiplies it by the same zero
divisor at `0x101355e0`. Register r3 is overwritten at `0x101355f8` before any
other use. Every possible quotient therefore produces the same continuation.
The build-time compiler checks this complete instruction sequence and chooses
zero only for this unobservable intermediate result. Nonzero division, the
original signed remainder, RNG consumption, and other division diagnostics
remain unchanged. No gameplay rule or original executable is patched.

`native/day_advance_checks.cpp` first reproduced the division stop, then passed
96 cases covering signed fractions, zero, integer boundaries, RNG state, and
six different possible undefined quotients. It compares the actual recompiled
continuation's registers and stack across those quotient choices. Logs:
`.local/logs/day-advance-before-tests.log` and
`.local/logs/day-advance-focused-tests.log`. Live run `app.cwxsEo` passed this
boundary: the user confirmed advancement, the interactive news screen showed
3 January 2002, and the game subsequently entered a friendly on 4 January.
The new match stop is described below.

### Match arithmetic

`app.cwxsEo` stopped at minute 4 of Cruzeiro–Anapolina (0–0) on 4 January 2002.
The log reports signed division by zero at `0x1024773c`, LR `0x102474a0`, after
60,443,664,062 original instructions. In the original routine `0x10247030`, a
nonzero floating bound can truncate to zero before the random remainder step.
The quotient is multiplied by the same zero divisor at `0x10247750`.
Unlike the earlier day-advance site, subsequent branches overwrite r3 at
different addresses. The compiler now checks every continuation for a complete
overwrite before any further use, rejecting calls, indirect flow, partial
writes, cycles, and unknown targets. Other division diagnostics remain strict.

`native/match_arithmetic_checks.cpp` reproduces the original failure and checks
540 arithmetic/quotient cases, including signed fractional inputs, the observed
random sample, optional additional draws, and all branches before r3 is killed.
`tools/test_quotient_audit.py` checks seven groups of safe and unsafe control-flow
cases, including overlapping register reads. The runtime correction is limited
to this instruction. Live run `app.yPRT55` completed the match: Cruzeiro 1–0
Anapolina, scorer Leonardo at minute 20, full time displayed at minute 93.
The user confirmed completion. Continuing from this screen exposed the separate
sound cleanup boundary described below.
All 17 CTest groups pass in 38.52 seconds (`.local/logs/match-arithmetic-tests.log`).
The updated ARM64 app is installed, byte-identical to the tested build, with
the pre-match save unchanged (`.local/logs/match-arithmetic-install.log`).

The latest `Mac-Teste-Validado.sav` was saved normally in this run before the
match. Its 119 compressed blocks validate, and a metadata-preserving private
backup was made before restarting. Its SHA-256 is
`d45ced9f9fac573f2731bbf4e13798d056267e084e497675605be61ff554cc7b`;
this is a later save than the already verified ordinary save/reload cycle.
Evidence: `.local/logs/pre-match-save-validation.json`. No save repair was made.

### Zero divisors in general (2026-10-21)

The two sites above were one idiom: a random draw modulo `(int)(bound * n)`,
where the bound can truncate to zero. Driving the 26-country career hit a
third one (`0x10289b28`, match play on 23 December). The generator now proves
the idiom for every `divw` (`zero_divisor_quotient_unused`). On every path,
the quotient may only feed a multiply by the same, unchanged divisor before a
full overwrite; calls, returns, indirect flow, cycles, record forms and
partial accesses fail the proof. 1,385 of the 1,831 `divw` sites qualify and
continue past a zero divisor, as the PowerPC does. The other 446 use their
quotient and still stop on zero (`tools/test_quotient_audit.py`). Rewritten
owners see the same set: `SemanticGuestCall::divide` asks the generated
`quotient_free_division`, and equivalence references use the whole-program
proof, so a rewrite and its reference continue at the same sites.

### Original application shutdown

Both `app.VDk5TM` and `app.pigLIp` reached `DisposeCCursor` while quitting.
Its bridge now releases the original color cursor's owned PixMap, pixel buffer,
palette, and descriptor, and removes the native cursor object. Repeated or
unknown disposal remains an error. Running the original initialization and
cleanup functions (`0x1076e7c0`, `0x1076e870`) also exposed a monochrome handle
ownership error: `GetCursor` must return a Resource Manager handle that the
original code can pass to `ReleaseResource`. It now shares the same handle
cache as `GetResource`, including repeated lookup and load behavior.

`ExitToShell` now unwinds original callbacks to the application host as a normal
exit. It does not return into a callback and resume gameplay, and does not
bypass native resource destruction with a process termination call. The
shutdown regression uses the supplied original resources and the original
three-cursor initialization/cleanup loop, including its missing-cursor fallback.
It checks that cursor payloads are freed, repeated cycles work, invalid disposal
is rejected, and a nested exit reaches the host as success.
All 16 CTest groups pass after these changes, in 26.94 seconds:
`.local/logs/day-advance-shutdown-tests.log`. The installed ARM64 executable
matches the tested build (`.local/logs/day-advance-shutdown-install.log`).
Live advancement and normal UI exit are verified. Run `app.pis6iu` exited through
the original `ExitToShell` path successfully after 47,838,604,786 instructions.

`PBHDeleteSync` now calls the native File Manager to remove private files and both
forks, preserving real busy-file and nonempty-directory errors. Deleting original
media entries requires unimplemented private tombstones and stops explicitly.
An original quit attempt advanced through temporary-file deletion and then hit
the missing `DisposeCCursor` bridge.
The latest precise stop and verification state are recorded in
`reports/generated/aot-status.json`.

For targeted diagnostics, `CM_AOT_TRACE_IMPORTS=1` records native API calls and
results. Set it before launching the process: import tracing captures whether the
variable is present at the first engine dispatch and keeps that policy for the
process lifetime, including nested calls and additional game states. An empty
value or `0` also enables it; omit the variable to disable it. This avoids repeated
environment searches on every semantic dependency call.
`CM_AOT_DUMP_ADDRESSES` accepts comma-separated original data addresses
and dumps 256 bytes at each on failure. Keep such runtime artifacts under `.local`.
`CM_AOT_INSTRUCTION_BUDGET` sets a positive diagnostic limit (default 10,000,000,
maximum 10,000,000,000, or `unlimited`; the diagnostic Finder launcher selects
10 billion while the installed app selects `unlimited`).
Generated pages count instructions per straight-line run (`account_blocks` in
`tools/compile_recomp.py`): a run's first instruction steps and charges the whole
run, and a run that starts within the budget finishes, so budget stops fall on run
starts. Complete runs keep exact counts; at a fault or budget stop the PC is the
start of the run.
Host code that stops at or watches an exact instruction compares the PC with a
literal address; the generator finds every such comparison in `native/` and
gives that instruction a run of its own (`generated_block_accounting` checks it).
Leaf routines (at most six straight-line instructions ending in `blr`) are
inlined at their direct call sites in generated pages: the original
instructions run in the caller and are charged, but the leaf's own hook is not
entered. Only leaves whose hook belongs to an exact game-state owner family
(`INLINE_OWNER_MODULES`) are inlined, so those owners' call counters see only
calls from C++ owners, and a check that watches a leaf's PC sees only direct
entries. `run_until` also runs a leaf entry directly (`run_leaf` in the generated
`leaves.cpp`, with the page's own runs), so calls from C++ owners skip the
dispatcher and the leaf's hook too; `is_leaf_entry` tells checks which entries
behave this way. Owners in `RUN_LEAF_EXCLUDED_OWNERS` keep their hook there.
`CM_AOT_CAPTURE_DIRECTORY` saves the native windows' current
rendered images on failure. These are retained presentation buffers, not screenshots
proving that a frame was physically displayed. A targeted Ghidra decompilation can
use `--temporary-functions` with `analyzeHeadless -readOnly` when an already compiled
instruction address was not recognized as a function; those temporary functions
are discarded without rewriting the analyzed project.

The original event-loop table at `0x1091c330` exposed missing code when the first
OS event reached `0x1076ffb4`. `ExportRecomp.java` now accepts explicitly audited
code destinations after its output directory. Disassembling the six case roots
listed in `build_aot.sh` discovered 424 additional instructions. Before promoting
the expanded export, checks proved that every previously exported instruction,
all relocated segment bytes, register layouts, imports, and import transition
vectors remained unchanged. The previous export is retained under
`.local/recomp-before-event-discovery`. This remains offline code discovery and
native compilation; missing runtime destinations still stop explicitly.

Clock tests compare the original big-endian `GetDateTime`, `ReadLocation`,
`TickCount`, and `Microseconds` outputs with the host's time APIs. The original
eight-byte `UnsignedWide` layout is preserved despite reversed native hi/lo
fields. `GetMouse` converts the native pointer into the selected window port;
`PinRect` preserves the documented right/bottom equality quirk.

AppKit responders now enqueue MacRoman key messages, repeats, mouse buttons,
global coordinates, activation changes, and wheel deltas. `WaitNextEvent`
preserves mask-filtered events, fills the 16-byte original EventRecord, and
reports movement outside the supplied region. Original window-click and wheel
callbacks execute with native Carbon event parameters marshalled to big endian.
Tests drive native queues/responders and verify the original handler's actual
right-click, Control-click, and wheel-accumulator writes. These are synthetic
input tests, not verification of physical input in a playable game. Native menu
selection dispatch and open-document marshalling remain unimplemented. Parameter
coercion errors are forwarded from the native Carbon API; only the exact scalar
types requested by the original handlers have been validated.

Physical user clicks have also navigated the installed prototype's setup screens.
Tool-delivered mouse events can disagree with the real global pointer: the game
polls `GetMouse` before processing queued events and may hit-test its cached
position. Do not treat a submitted automated click as proof of its UI result, or
change physical input behavior merely to compensate for that automation limit.

The original sound path starts at `FUN_10744860`: it requests a sampled channel with
initialization `0x85`, sets volume with command 46, and stores user state at channel
offset 12. `FUN_10744a60` reads an entire sound, builds a 66-byte extended sound
header, submits buffer command 81, and queues callback command 13. Callback
`0x10744ee0` either sets the completion flag or resubmits the looping buffer.
These calls now use `NativeSound` and an AVAudioEngine backend. Channels retain
their original 1,060-byte layout and game-owned user information. Native buffers
own PCM copies. AVAudioSourceNode render blocks supply samples and atomically
publish completion after consuming the final frame. A lock-free hazard pointer
keeps canceled buffers alive until the native render thread releases them;
sample allocation and reclamation stay on the control thread. The main thread then
invokes the actual original callback, preserving registers and the PowerPC TOC.
Event waits periodically poll audio completion rather than blocking callbacks
until another input event. Callback delivery now occurs at `TickCount` and
`WaitNextEvent` boundaries, including waits inside the event loop; it does not
re-enter original game code from unrelated synchronous filesystem/resource calls.
This is a cooperative native scheduling policy, not a claim of identical legacy
interrupt timing. Quiet stops playback while keeping queued commands;
flush drops queued commands while leaving active playback intact. Immediate
disposal cancels both, and deferred disposal drains the existing queue. Custom
caller-owned channel memory is retained. Unsupported formats, synthesizers,
commands, and overlapping immediate buffers still stop explicitly.

Post-match cleanup in `app.yPRT55` exposed why that scheduling boundary matters.
The original `0x10744770` sends quiet, disposes the PCM and extended header,
calls `FSClose`, then disposes user data and the sound channel. Previously,
polling on every platform import delivered a pending loop callback *inside*
`FSClose`, after the callback's header was freed. Allocating its command reused
freed memory and the next buffer submission failed as unsupported PCM at
`0x10bf7bb4`, LR `0x10744f68`. This was a lifetime error, not a new audio format.

The regression executes the actual original ten-slot cleanup loop with three
active channels, including one-shot and looping audio. It covers playing and
completed buffers, with and without queued callbacks; every case takes the same
540 original cleanup instructions, releases all owned payloads, closes the native
files, and leaves no callback to run at the next poll. It first reproduced the
same unsupported-PCM stop and now passes (`.local/logs/match-exit-before-tests.log`,
`.local/logs/match-exit-focused-tests.log`). Quiet and flush retain their distinct
semantics described in Apple's [Sound Manager reference](https://dev.os9.ca/techpubs/mac/Sound/Sound-101.html);
the fix changes main-thread callback delivery, not sound commands or match logic.
All 18 CTest groups pass in 38.82 seconds (`.local/logs/match-exit-tests.log`).
The installed ARM64 binary matches the tested build; the latest save and its
private backup remain intact (`.local/logs/match-exit-install.log`).
Live return from full time is verified in `app.pis6iu`; the user confirmed success.
The club screen then showed further simulation into 10 January. The new normal
save, `Mac-Teste-Validado-Pos-Jogo.sav`, is 17,917,514 bytes with 120 validated
compressed blocks and SHA-256
`82c504f46ce54a723137dbf42c005d458a4d7b18a9abab4c9f92627aa94f08fb`.
A private backup preserves its content and metadata. The user confirmed saving,
and the log subsequently records a normal application exit. In run `app.oXBAh4`,
the user confirmed that this save loads and the Anapolina result appears in Last
Game. This closes the post-match save/reload check; see
`.local/logs/post-match-save-validation.json`.

The native backend decodes big-endian signed 16-bit and unsigned 8-bit PCM. The
observed channel flags `0x85` request mono output and no interpolation. Stereo
input is folded to mono for that mode; stereo mode and independent left/right
volumes also have offline native-render checks. Sample-rate conversion honors
the no-interpolation flag. Legacy no-drop conversion and amplified gains above
256 remain unsupported. This does not establish sample-for-sample equivalence
with an old Sound Manager hardware mixer or identical callback timing.
Both audio test groups passed 20 consecutive runs after adopting the native
source-node backend. The current audio check also cancels/replaces buffers 2,000
times while another thread renders through the native offline engine. The
optional AddressSanitizer build succeeded but hung in sanitizer/dyld allocation
initialization before `main`; the sampled stack is retained in
`.local/logs/audio-asan-stack.txt`. It was stopped and is not a passing memory-
sanitizer result.

The ten-byte AIFF extended rate must remain big endian when passed to native
`x80tod`, even on ARM64. `X2Fix` uses the host implementation's signed 16.16
rounding/saturation; the original floating argument/result is in register f1.
A5 save/restore keeps a logical original context, never a truncated host pointer.
Native offline tests verify PCM routing, frame counts, cancellation, queue limits,
and execution of the original one-shot/loop callbacks. Actual device startup also
scheduled both original opening sounds and delivered their original callbacks.
This is native device rendering/completion evidence, not an audibility check;
downstream device latency is not included in the callback timestamp.
`CM_AOT_TRACE_AUDIO=1` records those transitions without logging every OS call.

## Memory-access performance follow-up (2026-09-26)

The live match sample in `.local/logs/match-live-profile.txt` has 2,223 main-thread
samples, including 408 self samples in `Memory::check`, 502 in `page_1071a`, and
134 in `State::step`. This identifies investigation targets, not end-to-end costs.

Sorted-end and branchless range scans were rejected after synthetic comparisons.
The adopted change extracts exception formatting into a cold non-inlined helper:
valid reads retain the same checks while spilling fewer registers. The existing
13,823 mapped-byte oracle cases pass; invalid-access checks now also verify the
exact diagnostic. Six alternating before/after rounds show approximately 4.1%
and 7.8% throughput gains for the two mixed-access benchmarks; the remaining
single-region mixes range from approximately 0.5% to 10.5%. These are isolated
measurements, not evidence of a faster full game. Raw results are in
`.local/logs/memory-cold-comparison.json` and the installed-build benchmark in
`.local/logs/memory-cold-production-benchmark.log`.

All 18 test groups passed in 30.63 seconds (`.local/logs/memory-cold-tests.log`).
The tested ARM64 build is installed (`.local/logs/memory-cold-install.log`), and
`Mac-Teste-Validado-Pos-Jogo.sav` retains its recorded hash. The existing process
`app.oXBAh4` uses the previous code until the user next opens the application.
There is no live gameplay validation or whole-game speedup measurement for this
last optimization yet.

The user's next priority is substantial loading and simulation-time reduction.
Use the preserved post-match save as a reproducible starting point, establish
end-to-end timings for loading and day/match processing, and compare results and
save/reload behavior after each optimization. Do not infer a satisfactory user
experience from compilation, instruction counts, microbenchmarks, or green tests.

## Measured filesystem performance

The installed run `app.YEt2eA` issued 2,746 volume queries before its conversion
stop. Each query enumerated the visible filesystem, and each directory listing
used a quadratic scan to merge case-insensitive names. The merge now decodes each
filename once and uses an ordered index with the same `CFStringCompare` options.
Private entries still replace original entries, enumeration keeps its existing
byte ordering, and each query sees current filesystem contents. No persistent
directory cache or stale-count policy was introduced.

A local diagnostic harness, `.local/volume-benchmark.cpp`, calls the real
`PBHGetVInfoSync` bridge ten times against the installed Game tree and a fresh
private directory. Both builds returned 818 visible files and nine folders
(Finder metadata files are included in these live counts). Excluding the first
query, the median fell from 36.0341 ms to 7.67592 ms, approximately 4.69 times
faster for this isolated operation. Logs are
`.local/logs/volume-query-benchmark-before.log` and
`.local/logs/volume-query-benchmark-after.log`. This does not establish a whole-game
loading speedup. Both runs shared the machine with the native app's idle menu;
they are local diagnostics, not controlled cross-device benchmarks.

All eleven CTest groups passed after the change. New filesystem cases verify
accented/case-insensitive replacement, preserved spelling and sort order, no
duplicate entry, and visibility of an external addition on the next query.
The corrected build is installed; an already running process uses its loaded
code until the next launch. The startup sample in
`.local/logs/native-startup-profile.txt` covers opening screens/event polling,
not database or league initialization. During the user's subsequent new-game
attempt, `.local/logs/native-database-profile.txt` sampled the still-running
pre-optimization process for five seconds: 335 of 453 main-thread samples were
inside directory enumeration beneath the volume query. This confirms the hotspot
in real database initialization, but is not a measurement of total load time or
of the optimized build's end-to-end speedup.

`FindWindow` uses AppKit's visible window-number list in native z-order, not its
scriptable-window list (which excludes panels). Tests verify overlapping windows,
reordering, hidden windows, signed global coordinates, and original pointer width.
Only the currently supported borderless window class has a hit-region mapping.

`UppercaseText` is unavailable in 64-bit Carbon. Its Roman-script bridge uses
Core Foundation uppercase conversion while preserving the original byte count;
characters with no single-byte uppercase counterpart remain unchanged. The
system-script selector is resolved natively. Other scripts require additional
mapping. Tests cover ASCII, accents, ligatures, and buffer boundaries; exact
equivalence to every historical language-specific `itl2` table is unverified.

The six imported math functions (`sin`, `cos`, `atan`, `atan2`, `sqrt`, `pow`)
call native C++ math with the original f1/f2 double ABI. Tests check quadrants,
argument ordering, signed zero, neighboring registers, and square-root domain
behavior. This does not prove bitwise equivalence with every old math-library
rounding or exception behavior.

Completion still requires the original UI, new-game creation, simulation/day
advancement, a playable match, and save/reload verified on Apple Silicon.

## API evidence

- The local macOS SDK's `CarbonCore/Gestalt.h` defines `ppcf` and the vector feature
  bit; [Apple documents the selector](https://developer.apple.com/documentation/coreservices/1472037-powerpc_attribute_selectors/gestaltpowerpcprocessorfeatures).
- The local SDK's `CarbonCore/MacMemory.h` documents pointer/handle APIs;
  `Files.h`, `Resources.h`, and `Endian.h` define filesystem/resource behavior.
- [Apple Core Printing](https://developer.apple.com/documentation/applicationservices/core_printing)
  documents native page-format persistence and opaque printing objects.
- The local SDK's `HIToolbox/Menus.r` defines the original MENU/MBAR layouts.
  Native menu objects are implemented with AppKit.
- `CarbonCore/Timer.h`, `OSUtils.h`, and `UTCUtils.h` document the clock APIs;
  [Apple's date epoch constants](https://developer.apple.com/documentation/corefoundation/predefined-time-interval-values)
  establish the 1904 conversion. `HIToolbox/Events.h` and `CarbonEventsCore.h`
  define event fields, modifier bits, parameter sizes, and tick conversion.
- [Inside Macintosh: PinRect](https://dev.os9.ca/techpubs/mac/Toolbox/Toolbox-248.html)
  documents the edge-equality behavior retained by the bridge.
- The local AVFAudio `AVAudioSourceNode.h` and `AVAudioEngine.h` headers define
  native render blocks, thread-safety requirements, PCM layouts, and explicit
  offline rendering.
- Apple-authored [Sound Commands](https://leopard-adc.pepas.com/documentation/mac/Sound/Sound-47.html)
  and [SndDisposeChannel](https://leopard-adc.pepas.com/documentation/mac/Sound/Sound-98.html)
  document the original queue, quiet/flush, and disposal behavior.
- [Apple Text Utilities](https://leopard-adc.pepas.com/documentation/Carbon/Reference/Text_Utilities/DeprecationAppendix/AppendixADeprecatedAPI.html)
  documents the original script-aware in-place uppercase operation and its
  Core Foundation replacement; the local SDK confirms the 32-bit-only boundary.
- [Ghidra's P-code reference](https://ghidra.re/ghidra_docs/languages/html/pcodedescription.html)
  defines operation widths, signed shifts, and instruction-relative P-code branches.
- The supplied original executable and its resource fork supply the program,
  layouts, and observable test inputs; no Windows ABI structures are substituted.
