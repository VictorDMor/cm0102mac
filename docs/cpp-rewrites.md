# Rewriting original routines as plain C++

How a routine of the original becomes hand-written C++ that the game runs in
place of its generated body, and how it is checked. The tactics system
(`FORMATION.CPP`, `TACTICS.CPP`, `TACTICS_SCREENS.CPP`) is the first
subsystem done this way; `tools/tactics_audit.py` lists its 234 routines.

## What must stay identical

The rewrite may compute however it likes, but must leave everything the game
can observe exactly as the original does (CLAUDE.md):

- **Guest memory**, written through `guest::Cpu` (big-endian, checked; memory
  observers see each write). Order of writes does not matter at a return, where
  final values do; a fault on both sides compares memory where each one stopped, so
  writes before a load that can fault stay before it.
- **The stack image.** The original reads stack bytes it never wrote
  (docs/aot-port.md, "Uninitialised stack reads"), so every byte the original
  leaves below the stack pointer must be left too: its prologue's saved
  registers, LR and back chain (`guest::Frame`), its stack locals, and
  everything its callees leave.
- **Registers that reach memory later**: r1, r2, r13-r31, f14-f31 and the
  results (r3, f1), and also r4-r10 and every CR field as the original leaves
  them at return: a caller's next varargs call (`sprintf`) stores r3-r10 into
  its frame, and a callee that saves cr2-cr4 stores the whole CR (`mfcr`).
  The checker compares all of these; r0, r11, r12, XER and CTR only with
  `--strict-volatile`.
- **Calls and their order**, so RNG draws, allocations and imports happen in
  the same sequence with the same arguments.

## Lessons from Bosman Edition

Bosman Edition statically recompiles the Windows 3.9.68 executable to C and
extends it through checked hooks and native code. The reCM goal is to
reconstruct every module of the Mac 3.9.62 executable in C++23 while improving
performance in parallel. The faithful implementation gives future gameplay
and simulation changes a reference to compare against.
[Bosman recompilation design](https://github.com/maxevans60/cm0102-bosman-edition/blob/v0.6.1/docs/specs/2026-10-14-recompile-spec.md#bulk-recompile)

- **Keep gameplay rules in one place.** Bosman's 48-team World Cup uses
  separate hooks for entrant slots, the draw, and predicted places. Keep the
  legacy offsets and calling conventions at the boundary where practical, and
  let typed C++ callers share the rule for entrants, phases, or rankings.
  [Bosman World Cup implementation](https://github.com/maxevans60/cm0102-bosman-edition/blob/ec04eb8983b42d68312b93b561458849458fceff/src/recomp_rt/win/season_wc48.c)
- **Check fidelity and gameplay changes separately.** Bosman documents a
  staff routine that reads an unwritten stack slot whose contents can depend
  on event timing. Preserve observable stack, register, memory, and RNG
  effects in rewrites and optimizations. Compare both bodies from the same
  state, with a replay at the affected boundary. For a deliberate gameplay
  change, retain the original behavior as a test reference and assert the
  intended new outcomes separately.
  [Bosman same-save checks](https://github.com/maxevans60/cm0102-bosman-edition/blob/ec04eb8983b42d68312b93b561458849458fceff/docs/TESTING.md#same-save-checks)
- **Measure the work the player waits for.** Bosman tracks Continue and
  holiday performance with all leagues loaded. Our `MATCH_ENG.CPP` takeover
  also showed that fewer guest instructions can leave elapsed time unchanged.
  After `MATCH_PL.CPP` closes, try direct typed role and position queries in
  the spatial and pass-receiver scans. Compare paired match replays for
  equivalence and timing, then measure the complete Continue separately.
  [Bosman performance issue](https://github.com/maxevans60/cm0102-bosman-edition/issues/76) ·
  [reCM performance notes](native-background-performance.md)
- **Treat rules and save compatibility as one change.** Bosman 0.6.1 fixed a
  save failure that could leave a career without its season data. The reCM
  English playoff keeps the rules of the current saved season and adopts the
  new preset at the next rollover. When a feature adds persistent state,
  version it with the career, check the transition from older saves, and
  verify that an interrupted write leaves the previous save usable.
  [Bosman 0.6.1](https://github.com/maxevans60/cm0102-bosman-edition/releases/tag/v0.6.1) ·
  [reCM English playoff](native-competition-rules.md)

## Writing one

`native/guest_abi.hpp` has the pieces:

- `Frame frame(c, entry, size, {.first_gpr = 29})` writes the prologue's image
  (saved GPRs/FPRs at their slots relative to the caller's stack pointer, LR
  at +8, CR at +4 when saved, back chain) and runs the epilogue on return
  (registers reloaded from their slots). Read the original's prologue for the
  size and the saved registers; `stw r31,-4(r1); stw r30,-8(r1)` is the same
  layout as `stmw r30,-8(r1)`. Frameless routines that store below r1 (red
  zone: `stmw r27,-0x14(r1)` without `stwu`) must write those bytes too.
- `c.call(target, return_address)` calls a routine that still runs generated
  code (or any routine through the dispatcher). `return_address` is the
  address after the original `bl`: the callee saves it as its LR.
- `c.link(return_address)` before a direct C++ call to another rewritten
  routine, for the same reason.
- `guest::report_assertion` replays the game's assert macro (split file name,
  version text, `sprintf`, error dialog, clear the success flag), used by
  almost every routine for out-of-range arguments.

**Registers at calls.** A callee's prologue saves the non-volatile registers
it uses, so the stack image holds whatever the caller had in r13-r31 (and
f14-f31) at the call. Before each call, set every non-volatile register the
original routine has changed by then to the value the original holds there.
Callers of varargs routines (`sprintf` 0x1077b720) and routines that spill
their arguments also leave r3-r10 in the stack image, including registers the
call does not use: leave those as the previous callee left them, or set them
as the original does. `tools/call_registers.py ENTRY` prints, for every call
of a routine, which instructions define each argument and non-volatile
register there ("entry" means the caller's value); per-path differences
(a register set only on one branch) need reading the listing.
CR at a call does not reach memory for the tactics modules: none of their
380 callees reaches an `mfcr` (prologue CR saves, `setjmp`) before writing
cr0 itself (checked 2026-10-23, following calls and jumps six deep), so only
the CR left at return has to match.

**Arithmetic.** Use 32-bit wrapping arithmetic as the PowerPC does
(`std::uint32_t(index) * 8` for a signed byte index), sign extension where the
original uses `extsb`/`extsh`/`lha`, and the original's exact comparisons.
Reproduce the original's quirks (a shared bit, an off-by-one) and say so in
a comment. Floating point must round as the PowerPC does (single-precision
results rounded to single, multiply-add with one rounding) and leave FPSCR
as the original does; prefer a helper in `floating_point.hpp`.

**Shape of the code.** Named record views (`formation::Record`), plain
control flow, one C++ function per original routine with a typed signature,
and a hook lambda in the routine's `tactics_routines_<group>.inc` line that
reads the arguments from the registers and sets the results. Uninitialised
inputs are part of the contract: if the original returns a register it only
sets on some paths, take its entry value.

## Checking one

1. Add the routine's line to its group's `.inc` with the digest
   `python3 tools/tactics_audit.py ENTRY` prints.
2. Compile-check the file alone, then build (builds are serialized by a lock;
   a file that does not compile breaks everyone's build):

   ```sh
   DEVELOPER_DIR=/Library/Developer/CommandLineTools xcrun clang++ -std=c++23 -arch arm64 \
       -Wall -Wextra -Werror -Wpedantic -ffp-contract=off -fsyntax-only -I native native/FILE.cpp
   tools/dev.sh build cm-world-replay
   ```
3. Run it on a capture that calls it, comparing each call with the original:

   ```sh
   .local/current/build/cm-world-replay .local/benchmarks/world/season-day 1 \
       --timing --check-routines 200 500 --only 0xENTRY,0xENTRY
   ```

   At each checked call two forked children run the original and the C++
   from the same live state; the parent compares every page either wrote,
   the heap tables and the registers above, prints the differing bytes
   (stack addresses relative to the entry sp) and stops. The routine runs as
   C++ even before the generator hooks it: the check tool asks the adapter at
   every dispatch. `--only` limits that to the listed routines (others not yet
   hooked keep their generated originals); without it every listed C++
   routine runs and is checked. `--census FILE` counts which routines a
   capture calls.
4. The replay must still reach the capture's recorded end state.

Routines the captures never call (screens, file dialogs, error paths) need a
session capture of their own (automation `capture start NAME` / `capture
stop`, with `drag FROM TO` for mouse drags between two controls), or
synthetic calls: `--synthetic FILE` lists lines such as
`trigger=0x10111310#3 call=0x10114ba0 r3=@r3 r4=0x5`, and when the replay
dispatches the trigger for that time, the target runs with those registers
(`@rN`, `@rN+off`: the trigger's registers) in two forked children, original
and C++, compared like a checked call; the replay itself is untouched. A
synthetic call's frame lies below the trigger's, so its imports see other
inputs than the recording: its children only require the recorded import
order (not the recorded heap state), and both sides get the recorded
outputs. `from=0xRETURN` counts only the trigger's dispatches from one call
site; `m8=`/`m16=`/`m32=ADDRESS:VALUE` sets memory the call sees (undone
after it), with `[EXPR]` for the word at EXPR, e.g.
`m32=[@r2-0x7b74]:0x3e015` for an older save version. When both children
stop at the same import with the same message (the first import the
recording has no output for, such as an error alert),
their states are compared, the condition register aside, and
the line reports `ok (both stopped ...)`. Matching internal faults fail the
check: an equal exception message does not establish equivalent memory,
imports or results, and the C++ need not mirror intermediate registers.
`expect8=`/`expect16=`/`expect32=ADDRESS:VALUE` additionally require the
specified final memory in both completed children. Expressions use the
trigger registers and initialized input memory; partial stops and unmapped
expectations fail, even if both runs would otherwise compare equally.
A routine is done
when its checks pass on every capture that reaches it, including the error
paths someone can reach in play.

## MAIN.CPP: start-up, the event loop and paths no capture reaches

`MAIN.CPP` (44 routines, `tools/main_program_audit.py`, `native/main_program_*`)
is the second subsystem done this way. Its routines read as plain C++ over
typed views (`main_program_detail.hpp`): `Field<T>` for a value in guest
memory, `EventRecord`, `FSSpec`, MAIN.CPP's `Globals`, the game objects it
reaches through the TOC (`Shared`), one `Toolbox` call per Carbon routine
and one `Program` call per routine of the rest of the game or the C
library, each setting exactly that call's arguments. What the game can
observe stays in step, under names: `Kept<R>` is a local the original keeps
in non-volatile register R (the routines it calls save R in their frames,
so the value reaches the stack image), `keep()` the same for values the C++
needs no name for, `enter()` the original's `bl` into another MAIN.CPP
routine, `leave()` what it leaves in r3-r10 at return (a caller's varargs
call stores them), and cr0 through `compare()`.
r0, r11, r12, CTR and XER are not reproduced (nothing reads them before
writing them). A Toolbox import is checked on the argument registers it
reads (`import_contract` in season_reference.cpp lists each one's arity).
Three things it needed:

- **Launch captures.** `CM_AOT_LAUNCH_CAPTURE=DIR` (or `game_driver.py launch
  NAME --capture-launch CAPTURE`) records a session from the program's first
  instruction; any session capture ends before ExitToShell, so quitting
  through the game gives a capture that holds main()'s return. Checking
  0x1076f2b0 there compares the whole run, original against C++. A checked
  call that the capture ends inside is compared where both runs stop.
- **setjmp in a C++ routine.** run() calls setjmp, and fatal_exit()'s
  longjmp (quitting from the start screen takes it) lands back in run()'s
  frame. The transfer runtime would resume the generated body at the setjmp
  return; run() catches that one transfer (pc at the return, sp at its own
  frame) and goes on in C++.
- **Host paths for the rest.** The host now has a small Dialog Manager
  (GetNewDialog/ModalDialog/DisposeDialog over DLOG and DITL, shown as an
  AppKit panel centred on the game window; automation `dialog N` picks an
  item, observation requests still run, and a `quit` waits for the dialog to
  close) and delivers Finder
  document opens as one 'odoc' event, with real Apple Event Manager lists of
  FSSpecs behind AEGetParamDesc/AECountItems/AEGetNthDesc/AECoerceDesc/
  AEGetDescData. A launch with documents sends no 'oapp', as the original
  system did. A document outside the game's folders, or one whose name has
  no FSSpec (over 63 MacRoman bytes), is left out with a line in the log; the
  game still starts. `game_driver.py launch NAME --open DOC` launches that way.
- **What no capture can reach.** `debug_menu` handles menu 4, which is only
  in MBAR 2: the program installs one menu bar, MBAR 1 (menus 1-3), and adds
  no menu, so neither MenuSelect nor MenuKey can return it (a ctest checks
  the calls in the export and MBAR 1 in the installed original). fatal_exit's
  exit() runs only for a fatal error before the game has started.
- **Capture paths added for the rest.** `nothing()` is the original's Restart
  hook, empty in this build: Game Options > Restart, Yes, reaches it, and
  the game carries on. Automation `popup N`
  chooses item N of a closed pop-up (`control N` refuses those), `activate`
  and `print PATH` queue what AppKit's windowDidBecomeKey and print-files
  queue, so nothing, activate_event and print_documents are capture-checked.
- **Speech.** `CM_AOT_SPEECH=on` (or `silent`, volume 0, for validation)
  makes Gestalt('ttsc') report the Speech Manager and bridges its channel
  calls to macOS's; unset, the game sees none, as before. The original
  speaks each news headline it shows, and Cmd-S or Option-S toggles speech
  before the menus see the key, so Cmd-S never reaches File > Save from the
  keyboard (Option-S types 'ß' on a Mac, so only Cmd-S toggles in practice).
- **Fixture checks** (`cm-main-program-checks`): for the paths the game
  itself cannot reach and for every
  branch of the pump and the menus, each case runs the original and the C++
  from a real snapshot with the Toolbox imports replaced by scripted
  stand-ins, compared as the routine checker compares (the volatile scratch
  registers aside), plus each Toolbox call's arguments and r13-r31. That the
  scratch registers and a void routine's r3 are never read is a static audit
  over the original (`tools/main_program_audit.py --volatiles`, a ctest).
  Under `--check-routines --only`, MAIN.CPP
  routines left out run their originals, so a nested routine is checked at
  every call from the original's callers.

## PLAYER_STATS.CPP: season records, histories, the stats screens

`PLAYER_STATS.CPP` (36 routines, 8,759 instructions, 0x10301540-0x10309ee0;
`tools/player_stats_audit.py`, `native/player_stats_*`) is the third
subsystem, written in six groups (relations, queries, text, records, daily,
files) with one routine list per group (`player_stats_routines_<group>.inc`)
and the shared views in `player_stats_detail.hpp`. What it added:

- **Finding a module's bounds.** The assert's file name lives in the
  module's string pool; the TOC slot pointing at the pool is loaded only by
  the module's code, so the first and last routines loading it (and the next
  module's first routine loading its own pool) bound it. Entries are every
  `bl` target and transition-vector target inside; TOC words pointing into
  the code are often jump-table case labels, not routines.
- **guest_values.hpp** holds MAIN.CPP's typed values (Field, Kept, enter,
  leave, compare...), shared now; **guest_fpu.hpp** computes each PowerPC FP
  operation and its FPSCR bits exactly as the export's p-code does (fsub and
  fsubs do not fold FI into XX; divisions set ZX and VXZDZ; multiply-adds
  round once).
- **A census misses same-page calls.** `--census` counts dispatches, and a
  generated page reaches a routine on the same page without the dispatcher
  (0x10305700 -> 0x10305c60), so a routine can be reached where the census
  shows none. A direct C++ call from a hooked caller makes the checker see it.
- **Callbacks.** The sort comparators return through the pointer glue
  (0x10781320) into qsort and bsearch; the volatile audit also scans after
  those glue calls (CALLBACK_SITES).
- **CR at calls.** Following each external callee six calls deep, the only
  instruction that reads condition-register fields before writing them is
  the exception runtime's context save (mfcr at 0x10772d28), reached only
  when something throws; so CR is reproduced at returns only.
- **Captures** in `.local/benchmarks/ui`: ps-profile and ps-profile2 (a
  player's profile and Historial screens, column arrows), ps-newgame (a new
  game, Scotland: the real-world history import, ~40 s creation, 367 MB).
  Match-actor construction's branch into 0x103058f0/0x10305ae0 was not
  reached by a real match start, and 0x10309cc0 (the transfer-rule
  predicates) by none; those three are checked by synthetic calls.

## SHORTLIST_MANAGER.CPP: the recruitment module

`SHORTLIST_MANAGER.CPP` (57 routines, 26,197 instructions, 0x10356410-0x1036ff60;
`tools/shortlist_manager_audit.py`, `native/shortlist_manager_*`) is the fourth
subsystem: the shortlist manager object and the daily recruitment over it
(candidate scans, assessments, interest evaluations, registrations), in eleven
groups written in parallel. What it added:

- **Bounds by pool slot.** Its string pool slot is TOC -0x4dd8; SETUP.CPP's
  (-0x4e18) is last loaded at 0x10356250 and SIMULATED_STATS.CPP's slots
  (-0x4dd0..-0x4dc4) first at 0x1036ff60. The module's strings sit in the code
  segment's constant area (the data segment starts at 0x108b3a90).
- **Taking a module over from builder owners.** Seven older builder families
  owned 33 of its routines, and production pages called them in place, so a
  draft's entry never reached the dispatcher's check probe. The generator
  therefore hooks every routine of the module from the start
  (SHORTLIST_MANAGER_ENTRIES is all of ROUTINES): the adapter runs the
  SHORTLIST_MANAGER_ROUTINE lines, drafts only under a checker, and declines
  the rest to their generated bodies. The four families wholly inside
  (SHORTLIST_MANAGER_FAMILIES) left production; quotation and
  profile_qualification are taken over at their module entries only.
- **A builder owner was not the reference.** With the builder owners out of
  production, ui/tactics-continue and ui/ps-newgame (recorded with them in)
  stopped reaching their end states with the generated code alone. Turning
  the families off one at a time showed it was shortlist_links (a heap byte
  its owner left 255 where the reference leaves 0). Both captures' expected
  states were rewritten from the generated reference.
- **Calls inside the module.** `Program::call` into another routine of the
  module runs that routine's hook in place (`call_rewritten`: the link
  register, then the hook), the same state change as the dispatcher path
  without the round trip; a check of the caller covers its callees.
- **Synthetic triggers.** A trigger fires only on a dispatch, and hooked or
  in-place-called routines never dispatch: trigger on an unhooked routine the
  same caller reaches (0x1036ff00 for the interest and context evaluations,
  0x10082f90, 0x1014c2e0 for the squad assessment).
- **Register-only loops.** Loops that only shuffle registers leave values in
  r4-r10 that pass untouched through later callees to the return; the checker
  compares r4-r10, so every such write that can reach a return is reproduced
  (`leave()`).
- **Calls reproduced natively.** register_club_interest runs the contract
  lookup leaf 0x10082f20, the draft constructor 0x103721f0 and club
  control's plain path (0x1014c2e0 with 0x100ce250) in C++, leaving their
  registers and stack frames; anything off the plain path takes the real call.
- **f1 at return.** The checker compares f1 only for routines that return
  it, and no caller reads f1 after the others (checked statically), but an
  operation capture ending at a routine's return compares r3, r4 and f1
  (world/interest-pass ends at 0x10363ca0's). Every routine therefore leaves
  f1 as the original does; `--compare-f1` makes the checker compare it for
  every routine.
- **The builder families' fixtures check the C++.** The seven fixture
  programs of the older shortlist families (cm-club-shortlist-storage-checks
  and the like) script every call a routine makes, including legacy save
  versions, short reads, allocation failures and fatal exits; each now also
  runs the SHORTLIST_MANAGER C++ on every case
  (native/shortlist_rewrite_fixture.hpp). A call hook in the adapter
  (ShortlistManagerCallHook) routes the C++'s calls, in-place reproductions
  included (Program::inlined), to the fixture's model; direct calls inside a
  group are recorded (enter_direct) so the reference runs those callees too.
  They found nine wrong return addresses on paths no capture took.
- **Driving the game to a path.** The automation's `snapshot` writes the
  game's own 800x600 surface (the second image), which shows the screen even
  where a host screenshot cannot; the status names each screen's handler and
  each control's action code, and decompiling the handler maps an action to
  what it calls. Left-bar menus are `popup` items; rows of lists inside a
  pop-up group are refused by `control`. Routes found: Offer Contract and
  transfer status on a player (ui/contract-offer), a bid with a part
  exchange (ui/swap-offer), Options > add a manager, choose a club, take
  control (ui/take-control, the only route to 0x1036ec30 without waiting
  for a board to hire the human).
- **Coverage is separate from equivalence.** `CM_SHORTLIST_COVERAGE` adds
  source coverage only to this module; forked checker children flush their
  profiles before `_exit`. A hit from a failed or faulting synthetic call
  does not prove that branch equivalent. Keep such runs separate from
  successful comparisons, and distinguish a checked call from a parent
  replay that was stopped before its end-state comparison.
- **Diagnostic reads can create false failures.** The interrupted interest
  probe read `contract_at + 4` in a logging argument after 0x1036a8e4,
  without checking for a contract. Its fault at 0x4 is absent from the
  production C++; the original later reads a null contract at 0x1036b668
  (offset 0x50) for that artificial input. Do not use the probe's profiles
  or its differing fault addresses as evidence of a production rewrite bug.

## CLUB_RECORDS.CPP: club records, notices and persistence

The 92 original entries occupy 0x1000e6b0–0x1001dae0, between CLUB_HISTORY
and CLUB_SCREENS. TOC -0x599c identifies the source pool; two transition
vectors select record destructors, and six 33-word tables select record kinds.
The apparent code pointer at 0x108eccf4 is two RNG samples, not another entry.
`club_records_adapter` owns every entry in normal generation, including direct
calls; the separate checker emits hook-free original pages.

Club records occupy 0x3fc bytes, per-club indexes 0x6c, and pending rows eight.
Assignments preserve field widths, padding, order and overlapping source reads.
Date constructors initialize only dates; clear_record resets only the original
sentinel fields. Single-precision assignment quiets signaling NaNs without
changing their sign or payload. Floating-point guards retain the actual CR-bit
tests: PowerPC `ble` also branches when a comparison is unordered.

The file format writes capacity/count/used rows for modern and season records,
then club indexes and pending rows. Preserve raw padding, signed empty checks,
mandatory index allocation/read even for zero indexes, and one warning before
clearing invalid index rows. Archive's late null-modern creation guards follow
earlier dereferences; they do not make a missing modern record safe.

Post-match recognition increments the RNG cursor before reading, wraps and
reseeds before the draw, and preserves eligible-person order. Its reputation
reduction deliberately uses integer division before clamping. News type32
loads the player id into r9 before checking the sentinel; the name formatter
consumes this seventh argument even though decompilation omits it.

## CASH.CPP: monetary values, currencies and presentation

`CASH.CPP` owns 12 entries / 2,847 indexed instructions between
0x10009ef0 and 0x1000cbc0 (`tools/cash_audit.py`). All incoming calls target
entries; four in-body dispatch banks are switch labels, not callbacks.
`cash_adapter` replaces the older money-value, money-text and currency-profile
builder owners in normal generation, including direct calls from other pages.
The older components remain reference/check tooling only.

Stored amounts are signed 32-bit values; displayed amounts and quantization use
signed 64-bit pairs with explicit wrapping. Flag 4 takes priority over flag 8
when choosing a quantum, and flag 32 bypasses initial alignment. Currency
configuration caches all six destination pointers before copying any text,
loads all three factors before publishing them, and retains each profile's
original load/store order. The format routine reconstructs at most 15 digits,
leaves nonpositive signed inputs with no decimal digits, trims only the two
fractional positions of compact layouts, falls back to millions when the
billions suffix is empty, and changes output spaces to the renderer's pipe
placeholder. Capacity checks and diagnostic pointer caching remain as original.
Arithmetic, string and printf services are external dependencies; CASH has no
RNG draws, callbacks or serialized IO of its own.

## CLUB_HISTORY.CPP: club league and cup archives

`CLUB_HISTORY.CPP` has 11 routines and 1,702 indexed instructions in
0x1000cbc0-0x1000e6b0 (`tools/club_history_audit.py`). Its constructor follows
CASH; CLUB_RECORDS starts at the upper bound with a different string pool.
TOC -0x59c8 points at the history pool; -0x59cc is the transition vector for
the final id comparator (0x1000e680), which belongs to this module too.
Every entry is hooked in production through `club_history_adapter`; internal
load/current-league calls are direct C++. The original comparator returns
0 for equal ids and 1 for unequal ids, including when the left id is smaller:
keep this quirk rather than substitute a conventional three-way comparator.
League rows are 24 bytes and cup rows 12; field copies leave the original
padding untouched. The fallback current-league query leaves its provisional
output byte untouched. Archive load/save retain the original short-I/O,
allocation-failure, fatal-exit and close ordering.

Focused comparisons use the existing game-load, game-save2 and ps-newgame
captures, plus `native/club_history_synthetic.txt`. Synthetic cases that stop
at an uncaptured import compare state only to that boundary; they do not
establish completed archival I/O or full branch coverage. Career save/reload
through the live app and performance are not retested by this delivery.

## GER_FIRST.CPP

The [audit](../tools/ger_first_audit.py) closes 11 entries and 3,916
structurally reachable instructions between FRA_THIRD.CPP and GER_SECOND.CPP.
All 11 entries use handwritten C++23 in the normal route, including the
next-season coordinator and geographic regional assignment. Owned direct and
virtual calls remain inside the handwritten implementation; external virtual
overrides and checker selection keep their actual targets.
`CM_GER_FIRST_ORIGINAL=1` keeps the independent original route.

Collection performs its 2001 regional reassignment before allocating the
18-club standings buffer, preserving those partial writes on allocation
failure. It excludes twice the live youth-block count in that first pass,
remembers the original definition at club+0x38 and selects clubs using the
live division ID and count after insertion callbacks.

The year read after allocation selects one of two 34-round calendars. Later
date calls reread the signed year, while retaining the chosen table. Output
flag/count/start writes preserve pointer aliasing. The final assertion compares
literal 34 against 34 with no intervening observable boundary, so its error arm
has no valid path from any audited entry. Configuration invokes the owned
calendar directly after the shared CFM descriptor owner resolves the actual
virtual target; external overrides and checker/original dispatch remain valid.

Construction returns the object after reporting a failed stage, retaining
the partial state and shared failure flag. It reloads the caller's object slot
after callbacks and stores the allocated stage pointer even when its initializer
returns another value. The deleting-destructor flag is a signed short.

Division relationships prepare both main divisions, then recheck the live
regional entry before preparing it and its first child. Promotion filters live
club/reserve state, calls the external selection service once for four slots,
and consumes exactly four slots after any nonempty scan. With fewer candidates,
the selector reports an error but this caller continues into retained list
contents. Empty scans skip relegation as well. Flagged relegation rows are
looked up again before moving the club; both inline getter reads and their
read-before-budget-charge boundary match the generated reference.

Regional-slot filling caps candidates at 200 before advancing the scan. It
rereads the live standings after relegation callbacks before clearing a club's
previous division. A 19-club regional table subtracts one vacancy even when the
result is negative; that signed quota reaches selection unchanged and the
subsequent loop consumes candidates while the remaining vacancy is nonzero.
Reserve coordination retains the incoming or previous destination row when
neither previous regional ID matches. Its real external standing lookup runs
on both comparison routes; notification callbacks can change the live source
count. These quirks preserve the original behavior rather than add defaults.

Geographic assignment requires exactly 36 entrants. It repeatedly widens the
north/south latitude thresholds using ordered double arithmetic and original
FP status, then fills two 18-club lists using previous memberships before a
capacity-only pass. A NaN latitude can leave the threshold loop incomplete;
there is no added timeout or default region. The contiguous scratch lists and
pipelined old-membership copies retain their ordered reads/writes. Pair and
three-way restoration reread prior mutations; a true three-cycle among three
mismatches needs a third proposed region and is exercised with paired memory
observers, since ordinary proposals have only two values.

The next-season coordinator checks regional presence independently before
reserve coordination, after division preparation and after the secondary
division's rollover. It resets existing resources before incrementing the
signed 16-bit year, then configures, collects and ranks clubs, stopping at the
first failed phase. The controlled create/two-rollovers/destroy comparison
reaches every owned operation, with neighboring modules and services modeled
at validated call boundaries. This is controlled integration evidence, not a
new gameplay recording or a performance measurement.

## POR_SECOND.CPP

The [audit](../tools/por_second_audit.py) closes six entries and 1,531
reachable instructions between POR_PRM.CPP and SCO_FIRST.CPP. Construction,
destruction, calendar generation, club collection, configuration and the next
season use the normal C++ owner. `CM_POR_SECOND_ORIGINAL=1` retains the
independent generated reference for comparisons.

The calendar contains 34 rounds and rereads the signed season year before
every date call. Only the low signed byte of its format argument matters;
unsupported formats return null before writing output headers. The optional
flag is written first, then the round count and starting round, including
when those pointers alias. Allocation failure keeps those writes and clears
the shared success flag after the original diagnostic.

Club collection keeps the live division IDs and club count across callbacks,
selects 18 clubs and marks qualifying first-division reserve teams with -4.
Child destruction uses an inclusive last index, reread after each callback.
The deleting-destructor flag is a signed short; season advancement wraps the
signed year through its original 16-bit store. Construction reloads the object
from its caller parameter slot after callbacks. Owned direct and virtual calls
stay in C++, with the shared CFM owner preserving descriptor/TOC semantics.

External services still own base-division and stage lifetimes, standings/date
record updates, reserve lookup, ranking/fixture generation, allocation,
diagnostics and country notification. There is no module-owned save format
or RNG call. The focused checker compares the combined lifecycle against the
original with validated scripted external services; existing daily and January
Continue captures do not reach this competition's lifecycle.

## SIMULATED_STATS.CPP

The module lies between SHORTLIST_MANAGER.CPP and STAFF_CONTRACTS.CPP.
The [audit](../tools/simulated_stats_audit.py) pins seven entries and
2,201 reachable instructions. The range sampler and its staff-contract
counterpart have identical normalized 350-instruction bodies and identical
scale and signed-conversion constants. Their shared implementation retains
each entry's live TOC slots, budget boundaries and reseed call sites.

`simulated_stats.dat` contains a four-byte big-endian count followed by
that many 68-byte rows. A short transfer leaves the original partial data
and clears the shared success flag. Only a complete transfer calls close.
The loader returns a success flag, while the saver leaves the last service's
return value in r3. The squad count scans all 50 slots in order, so a service
can change a slot before the scan reaches it.

The constructor filters live competition and club-service state, orders clubs
by signed reputation with the original pairwise swaps, and shuffles through
three distinct RNG sites. Groups retain the original 15/16-member stopping
rule. For undersized collections this can exhaust the club list before a row
ends; the diagnostic clears the shared flag and processing continues. Failure
to allocate the temporary list exits, while the row allocation is unguarded.
Supplying a file delegates to the owned loader and returns the object even
when loading fails. The constructor and sampler are direct C++ calls in
normal mode and retain independent generated bodies in checker/original mode.

The update exchanges club IDs between rows before processing the day's groups.
Scheduling compares signed remainders modulo seven, then excludes calendar
codes five and six; the second calendar query remains a separate call. Squad
counts are stored as signed bytes alongside an unavailable marker of -1,
ordered by the original swaps, and the positive prefix is shuffled with the
original ordered RNG draws. A positive first count may pair with a zero second
count. The pair index advances only after successful simulation and ordered
publication to the external result services. Allocation and simulation failure
retain that index and recheck the live row and score conditions.

## STAFF_CONTRACTS.CPP: staff terms and offers

The [module audit](../tools/staff_contracts_audit.py) assigns 47 entries and
15,214 reachable exported instructions to the contract string pool, between
`0x103721f0` and `0x10381110` (exclusive). It distinguishes the five original
switch tables from independent entry points and checks for external edges into
the bodies. The production registry is `native/staff_contracts_routines.inc`.
`CM_STAFF_CONTRACTS_ORIGINAL=1` selects the independent generated reference.

The proposal at `0x10374f50` builds the existing contract record in place.
It distinguishes the supplied destination, the primary contract's club and
the incumbent contract. Release decisions read the live competition table and
preserve the order of the CFM calls, valuation calls and record writes.
Duration uses the live role table, current date and shared random pool.
Compare the next cursor with its limit and store it before any reseed, then
read the word and salt from their updated slots. Preserve the additional null-entry read in the
transfer lookup path, including the partial state when that read faults.
Bonus and clause contributions are applied in slot order before capping the
proposal return value. The stored wage and the return value are distinct.

Renewal and terms valuation call the proposal directly in normal execution.
The proposal's reference quote still enters the shortlist observer boundary,
which selects the typed wage owner and retains its original/profile selectors.
The record layout and its readers and writers in CONTRACT_MANAGER.CPP remain
the existing save ABI. The legacy initializer and reference-quote builders
are linked only by their historical component checkers.

## CONTRACT_MANAGER.CPP: ABI and persistence quirks

Global date initialization (`0x100939b0`) now belongs to this module. The
independent database bootstrap checks its actual dispatch/return counters
here and the other nineteen initializers in their existing owner; the
retired engine-initialization counter must stay zero for this entry.

Proposal preparation (`0x10080d30`) saves GPR19..31 at caller SP minus `0x54`,
with FPR29..31 at minus `0x18` through minus `8`. Keep the eight-byte gap
between them; the usual packed-frame default would move the GPR save image.

Pending-contract review (`0x10081bd0`) uses the original FPR28..31 leaf
save/restore helpers without another stack frame. Some scoring paths retain
the incoming `r18` and `f29`; do not initialize them for convenience. Its
relation sampler uses `max(5, 40 - best_score)`, after `best_score > 10`.
The wide integer sampler is unreachable with those constants. Three isolated
fixtures explicitly alter the minimum bound to test that defensive branch;
they are not observations of normal game execution.

Daily processing (`0x10079050`) refreshes date horizons in phase zero and
prunes/scans contracts in phase two; scanning additionally requires the
signed input byte to equal one. Sorting/consuming scheduled work still runs
in every phase. Keep repeated status queries and their ordering. Date
predicates compare unsigned years and signed days, ignoring the leap byte,
while their stack snapshots retain all eight bytes, including padding.

Contract changes copy fields rather than whole records. Primary renewals
retain the old metadata; a new-club replacement or append clears `+0x42`,
while a reused free row retains it. A non-increasing term becomes four
weeks, and the nationality cutoff is 100 weeks before today. A secondary
assignment that reuses a free row returns before the final club refreshes.
Both application paths grow capacity by 100 rows. Allocation failure reports an assertion
and continues into a null-derived append; checks distinguish its matched
fault state from completed returns, retaining the pre-store arguments too.

Offer eligibility (`0x10089ea0`) narrows the stored age to a signed byte and
tests contract flags in the original order. Its owner-date cutoff is 150
days before today. Offer rebalancing (`0x1008a160`) updates only the first
pending row matching both person and club, and only when its kind is three.
It copies proposal terms into that row, copies the row's date back into the
proposal and adjusts the proposal's wage through single-precision steps.
The upper ratio branch tests GT being clear, so a zero/zero assessment
ratio stops without a wage adjustment. Preserve this unordered FP behavior.

Base player value (`0x10084be0`) uses a signed byte for age, signed 16-bit
reputation inputs, but an unsigned comparison for the 5,000 reputation cap.
Its score square wraps to 32 bits before conversion. Club-less valuation
advances the shared random cursor once, including its normal wrap/reseed.
Keep all seven fused sites and the mixed single/double rounding sequence.
After the club-contract query, load the player's attributes before assigning
the returned contract to retained state; a null-person fault distinguishes
those operations. Checks include the complete original unsigned conversion
helper and its stack/FPSCR effects, with injected helper-input boundaries
and enabled-FP stops explicitly separate from ordinary returned cases.

Negotiated player value (`0x10085860`) reuses its age multiplier while
scoring the young-player floor; the division adjustment at `0x100894b8`
rounds to single precision. Its final age table covers 29..45 only, after
subtracting three for goalkeepers: the age-31 and age-33 additions are
3,250,000 and 1,750,000. Ages outside the table bypass that cap. Preserve
the later country adjustment and final contract-clause cap in that order.

Expiry news (`0x1008c4c0`) selects its detailed text by an unsigned period
code. Zero and codes above seven emit the existing shared text without
formatting it again. Display modes use only the signed low byte. The Bosman
departure renderer (`0x1008ca50`) passes its second club-name argument at
the new frame's `+0x38`, beyond the register arguments. The expiry publisher
(`0x1008c270`) reloads both the shortlist row's club ID and the club table
after the division eligibility callback; an injected callback mutation
checks that ordering separately from ordinary game calls.

Loan-return publication (`0x10091630`) refreshes the primary-contract cache
three times on the recalled-player path. If the supplied club equals the
player's current club, delivery uses the final lookup's club; otherwise it
uses the current club retained in `r5`. Assurance-request details
(`0x1008fcf0`) append the previous buffer for role zero or unsigned roles
above seven, even after already emitting the introductory text. Preserve
those appends and distinguish whole-word status flags from byte display modes.

New-contract requests (`0x10090310`/`0x10090560`) pass the low nibble of
contract `+0x60` as the squad role. Their renderer checks raw wage and date,
copies all eight constructor bytes, resets the pending iterator from `+4`
to `+0xc`, and marks message parameter seven through `0x102cd0c0`.
Departure publication (`0x10090a00`) refreshes the global primary cache but
uses the supplied manager, without updating that cache, for its type-six
lookup. Delivery thresholds are signed and strictly greater than 7,000.
Its renderer (`0x10090f20`) tests importance as a whole word equal to one,
uses person-link type `0x7da`, and guards club links by the exact `-1` sentinel.

Offer-result details (`0x1008ab40`) store display mode/output in the caller's
argument area (`new SP +0xe73/+0xe74`). The currency conversion returns a
pair in `r3/r4`; both words feed the formatter. Term years truncate
`days / 365 + 0.5`, including negative spans. Keep the separate long/short
club-name gender fields (`+0x23b/+0x242`) and names (`+0x221/+0x1ee`), plus
the renewal templates' three or four variadic stack arguments.

Transfer-unrest publication (`0x1008cd20`) fetches the next shortlist row
before testing its 30-club cap. Preserve its pairwise reputation swaps:
a stable sort can order tied clubs differently. The reported candidate
count is capped at three; a duplicate in the second/third slot truncates
the count and places the trailing flag immediately after the retained IDs.
Both random advances retain their signed remainders and wrap/reseed order.

Transfer-unrest details (`0x1008d450`) append at most five reasons in the
original priority order. Its extended lead-club template spills six variadic
words through `SP+0x4c`; keep the currency return pair and distinct nation/
nationality grammar fields. The publisher supplies at most three candidate
IDs. Those pointers remain cached across output callbacks, while final
primary-club and person links reread their global record pools.

Transfer-request review (`0x1008e350`) retains an eight-byte gap between
its GPR save area (`caller SP-0x3c`) and `f31` (`caller SP-8`). Automatic
acceptance compares the negotiated amount with 1.5 times base value; the
later reaction change narrows the increment to a signed byte before the
`[-100,100]` clamp. Ordinary sampler bounds are `[-2588,2767]`, or signed
bytes. Three wide recursive arms are preserved and checked only with
explicit retained-bound injections, separately from normal-input coverage.
The news fixture isolates its selected body so internal news calls receive
the same scripted dependency model on both sides.

Transfer-request details (`0x1008ec00`) test reason flag `0x800` twice
for distinct texts. Each test independently respects the five-reason cap:
`0x807` includes both, while `0x907` fills the fifth slot with the first.
The own-club reputation is still loaded when the reason cap is already met.

Pending removals (`0x1007f840`/`0x1007f930`) treat only news ID `-1` as
absent. All other words are queued through `0x102dea50` with the record's
`+0x80/+0x84` payload. Synthetic final-memory expectations require a full
return from both sides and verify the queue count and all three words;
matching interrupted calls cannot satisfy these expectations.

The loader's keyed-record section exists only above save version `0x3e000`.
Its contract rows become 100 bytes above `0x3e032`; older rows are 92 bytes.
Preserve the reference's unusual legacy path: it reads into the main array,
then converts a separate temporary allocation. The conversion leaves padding
untouched and its final bit extraction clears XER/CA. Pending/deferred counts
are signed 16-bit values; only the deferred-list allocation has a null check.
Read/write error returns leave the stream open, as the original does.

The isolated persistence fixtures compare original instructions with C++ for
empty and populated sections, legacy conversion, partial I/O, allocation
failures and post-load secondary-contract repair. Their adapter call hook
intercepts before `run_until`'s native-leaf shortcut; `dispatch_probe` alone
cannot supply a scripted answer for such leaves. Modeled dependency stops
are partial checks, separate from completed returns.

## ENG_PRM.CPP: English Premier Division

All nine routines (0x10529840–0x1052c9d0, 3,155 indexed instructions) use
plain C++23 in `native/eng_prm_*`; `tools/eng_prm_audit.py` pins their source
and production ownership. `cm-eng-prm-checks` compares complete returns with
private hook-free reference pages, including seven December calendar variants
and the assertion fallback. External scripted calls record arguments, preserved
registers, CR fields and continuations; separate cases use real calendar and
club-collection dependencies. Existing ordinary captures have zero module
dispatches and establish integration compatibility only. The original still
promotes a lower-league club when all three remembered league slots are full,
without changing that list; the rewrite preserves this behavior.


## ENG_CONF.CPP: English Conference

The six entries at 0x10524600–0x105267c0 (2,150 indexed instructions) live
in `native/eng_conf_*`. The league calendar has 42 rounds and seven December
variants. Its stage-zero branch also builds three knockout rounds, but normal
Conference construction sets the extra-stage count to zero and last index to
-1. The shared enumerator starts at -1 and continues only below that count;
normal configuration therefore requests only the league calendar. The dormant
knockout branch is preserved without enabling it. Destruction and annual reset
use an inclusive last-stage index and reload the count and pointer after each
virtual destructor, since callbacks can change them.

## ENG_FIRST.CPP: English First Division

The league uses 46 rounds and a two-round playoff calendar. Playoff entrants
retain the original seed order: sixth, third, fifth, fourth. Stage-zero fixture
places 1–3 map to league places 3–5; the league stage collapses places 3–6 to
the third/sixth boundary pair. Event kinds and stage selectors use signed bytes,
while fixture-place comparisons use unsigned bytes. Destruction and annual
reset reload the inclusive stage index and array after virtual callbacks.

## SCRMAN.CPP: screen manager, history and command transport

`native/screen_manager_*` owns all 85 entries in 0x1073c1b0–0x10744fc0,
with 8,946 indexed instructions pinned by `tools/screen_manager_audit.py`.
The first 74 entries are the screen-manager constructor, event loops,
history records, queries, command encoding, controls and global initializer.
The last 11 implement the adjacent sound support called by those routines;
source assertions do not establish its original file name, so the audited
scope includes it conservatively up to TCPIP.CPP. The old heuristic inventory
missed the constructor, destructor and reset. The monitor's eight switch
labels are internal branches, not extra routines.

All entries run direct C++23 in production and supersede 24 builder owners.
Internal calls are direct; `Frame` and named retained values preserve the
original stack image and the registers its callees save. Turbo-page entries
keep their counter boundary and are excluded from the generator's plain
in-place owner calls. The checker and `CM_SCREEN_MANAGER_ORIGINAL=1` retain
access to the original bodies for comparisons.

Command integers are little-endian inside big-endian guest memory. A bounds
assertion skips only its own field: later fields and transport still run.
Queries preserve leftover argument registers because callers' subsequent
variadic routines store them into the guest stack. Sound volume retains the
original signed multiply-high, arithmetic shift and negative increment,
including negative multiples where C++ division would round differently.

`cm-screen-manager-checks` compares every entry with isolated original pages
emitted with all replacement hooks disabled. Its deterministic dependencies
record call order, return addresses, consumed arguments, retained registers
and outgoing control arguments in compared guest memory. 657 valid cases
return and agree, including full-group scans, duplicate IDs and signed
counts, with a separate corrupted-result negative control.
Five real UI captures supply another 3,773 full comparisons and accept their
recorded end states. A paired season-day replay also accepts its recorded
state. Fixtures do not prove every possible interactive or audio-device
path; performance is not established by that single replay pair.

The independent database bootstrap requires one completed SCRMAN global
initializer and zero calls through the older initializer adapter, including
its original/profile paths. The legacy initializer transfer matrix now has
128 cases for its remaining 18 frames; those cases do not claim nonlocal
transfer coverage of the SCRMAN implementation.

The real app also saved a private career through its File menu, restarted
and reloaded that saved file on 10 February 2002 with its afternoon phase
and career commands intact. The signed ARM64 application was installed;
the existing progress spreadsheet records 85/85 production entries with
verified summary formulas. Original saves and installed game data were
preserved.

MAIN's 609 fixture cases run the transferred 0x10743590 dependency live on
both sides. A dispatcher-only fake for a new direct owner is asymmetric:
the generated caller can enter its C++ owner before the dispatcher, while
the rewritten caller's guest call still reaches that fake. The independent
SCRMAN fixtures retain the original-only comparison of that dependency.

SCRMAN's callback-free screen scans now compute their indices and stable
counts locally, then publish the original final argument/scratch registers
and CR0. Counts are not retained across calls; `copy_group_keys` still
rereads its potentially aliased count/output. Screen IDs still select the
first matching group/row, and negative counts skip their rows. Entry routing
uses aligned bitmap membership and prefix/popcount rank instead of binary
search; unlisted, out-of-range and unaligned addresses retain their prior
reject/decline behavior. Original and checker paths use the same public API.

## POL_CUP.CPP: the Polish Cup

Six entries (0x106cd810–0x106ce820, 1,018 indexed instructions) own the
constructor/destructor, seven-round calendar, entrant selection, season reset
and configuration. `pol_cup_*` uses typed guest records and a round table;
`tools/pol_cup_audit.py` pins the entry contracts. The neighboring source pools
and all direct/transition-vector targets close the inventory.

Selection uses the division/position recorded by league finalization
(0x104aa340, club +0x34/+0x23c), distinct from current division (+0x30).
Recorded lower-division clubs that changed division precede unchanged clubs
(one shared 70-candidate cap). Recorded second-division places 13–20 precede
places <=12, followed by lower-division additions up to 36 and recorded
first-division clubs. Each group retains database order before sampling.
The original sampler remains an external call, preserving its RNG order.
Allocation failure returns -1: the constructor tests exactly zero and therefore
continues on -1, while reset tests for a negative result. The configuration
wrapper stores the calendar result and returns 1 even when that result is null.
The constructor's selection==0 error block is unreachable because selection
can return only 1 or -1; it remains in the rewrite.

The differential fixture compares stack bytes too. Native FSRefs contain the
issuing PID, so a separate preparatory child supplies common native identities;
each runner first validates the actual bridge result with FSRefMakePath and
FSCompareFSRefs. Stable guest fork numbers still perform real private IO.
This gives equal external inputs without excluding any comparison bytes.

`CM_POL_CUP_COVERAGE` instruments a separate copy of the owned sources inside
`cm-pol-cup-checks`, with default profile output below the private build tree.
The normal app/world replay retain their uninstrumented library definitions.
Explicit `LLVM_PROFILE_FILE` paths still identify individual verification runs.

## NEWS.CPP: messages, actions and queues

The closed original inventory has 33 entries (16,819 indexed instructions),
from `0x102bdf90` through `0x102ce710`; `tools/news_audit.py` pins the source
pool, neighboring modules, all incoming targets and the one local switch table.
All 33 entries route to handwritten C++23. Message rendering composes 27 local
type heads and forwards 144 external service messages; owned calls use typed
C++ directly. The action-label result gates effects, retaining legacy bodies
whose labels make them unreachable. Queue IO preserves raw item padding, partial
writes and original fatal/close ordering. Module delivery verification is tracked
in `TASKS.md` and the private per-entry ledger.

Items occupy 0xe4 bytes. Their 200-byte payload starts at +8, and payload indices
are signed bytes even when passed in full-width argument registers. Text insertion
always terminates at +0xcf; its optional next index uses the source length rounded
up including the terminator, then narrows to a signed byte before clamping to 50.
The original item-copy operation preserves padding rather than copying all 0xe4
bytes, and uses forward paired-word copies on overlapping records. Queue lookup
retains zero as its loop counter when a negative global item count skips the loop.

The range sampler preserves signed wrapping arithmetic, individual floating-point
operations, draw order and inclusive pool-end handling. Its wide reduction is
bounded to [-32768,32768] by the original signed32 multiply/divide sequence, so
its deeper clone/self-call branches are unreachable. Injury text classification
uses the original 82-record table; class12 contains only codes77..81. Assertion
and missing-record paths retain the original accesses rather than introducing
new recovery behavior.

`CM_NEWS_COVERAGE` instruments only a separate object target linked into the
NEWS checker. The app and world replay use the uninstrumented library. Private
profile defaults prevent root-level artifacts; checker children flush before
`_exit`. Forked LLVM writers can append cumulative records to one cached filename,
so merged branch outcomes are meaningful while raw execution frequencies are not.

## AFRICAN_NATIONS.CPP: calendar and callback boundaries

Calendar stages use the signed low byte. Every stage below three allocates
three group rows; only stages -1, zero, one and two add custom fixture policies.
Stage three builds four knockout rows, writing the final before the third-place
round despite their priorities of 150 and 140. Higher stages return null without
touching the output fields. Date builders reload the live tournament year.

For stages below three, the placement callback maps its second field's values
one/two to one, three to nine and four to thirteen. The first and third fields
instead map one to five, two/three to nine and four to thirteen. Comparisons use
the low byte; other values retain their raw low byte. Later stages bypass this
mapping. The shared stage lookup prefers its cached record without checking
the requested key. Notifications reread the event, manager and competition
definition after the award interpolation call, retaining that call's result.

Construction advances the low signed halfword year to the next even year,
including wraparound. Its object pointer is homed above the frame and reloaded
between writes and calls; these live reads cannot be cached across observers.
Configuration builds its deadline from the previous narrowed year, stores the
calendar result and returns one even when calendar allocation failed. The
constructor therefore still attempts the shared standings allocation on that
path. Assertion version getters are inlined by the reference; preserve their
call-site budget boundary rather than adding another dispatch.
Season rollover keeps the shared standings object and child-pointer array,
rereads the last-child index after each destructor, advances the stored year
by two and rebuilds the calendar through the live configuration slot.
Group creation skips the first four entrant slots, then copies four per group
through live table-pointer reads. Both phase builders retain the raw allocated
stage independently of its constructor's return value, and continue after a
null calendar. They free the temporary entrants and calendar before rereading
the stored child pointer. The knockout bracket preserves the original ordered
first/second-place reads; it does not sort the eight qualifiers again.

Event text dispatches on signed low bytes for the phase and event kind.
The detailed form forwards the original wide arguments and stack bytes.
The plain form selects the original template and reloads club and competition
names for the output annotations after formatting. The repeated equal branches
in the knockout text path consume the same CR and skip the duplicate body.

Final placement sorts a temporary entrant copy with the shared signed
reputation comparator. It publishes exactly sixteen ranks with bands
1, 2, 3, 4, 5, 9 and 13, and reloads the live standings table for each write.

## OCEANIA_NATIONS.CPP

The 15 entries at `0x106e9940–0x106ebdd0` implement the Oceania Nations Cup.
The entrant selector deliberately keeps the original ten-slot pool caps and
untouched spare stack slots, then consumes three RNG-table draws around its
two external shuffle calls. Calendar stages are signed bytes; years are signed
halfwords. The event formatter's repeated equality branches at `0x106eaa10`
make the following duplicate text block unreachable; preserve its effective
message selection rather than treating it as a missing event case.

## INTER_AMER_CUP.CPP

The six entries at `0x106ebdd0–0x106eca50` implement the Inter-American Cup.
The selector uses continental champions when that continent has an active
nation or the cup year is 2001. Otherwise it scans clubs in database order,
stopping at the first club sentinel, with signed reputation thresholds above
1000 for North America and 5000 for South America, and samples one candidate.
The original first-continent empty-pool fallback writes the second entrant
slot, leaving the first untouched; the second selection can overwrite that
fallback. Missing direct champions return zero after reporting the error,
and both constructor and reset continue because they reject only negatives.
Calendar allocation failure likewise remains a null calendar with a successful
configuration result. No serialization is owned by this module.

`inter_amer_cup_native_constructor` checks full construction from the recorded
career against actual native services without a day import transcript. The
captured shared assertion flag is already zero, so the test starts a fresh
canary and requires it to remain set. As in the Polish Cup checker, real FSRef
outputs are validated against common native identities before supplying equal
host inputs to both children; every guest comparison byte remains checked.

The default game deliberately retires this competition after its final 1998
edition (Victor's gameplay change, 2026-10-28). This policy lives separately in
`inter_amer_cup_retirement.cpp`; the six reconstructed entries retain their
original semantics. The new-world builder calls the ruling-body initializer at
`0x10345bb0` and scopes the factory's existing absent-competition branches. The
save loader calls the same initializer at `0x1034a8e4` and restores legacy
objects and statistics. The Competitions sidebar skips the retired object when
building menu items and keeps it registered for historical lookups. Daily and
annual processing skip the retired object.
FIX_MAN's validated row reader omits only post-1998 fixtures whose home score
is -1.
The object at cup+8 owns club/player statistics; it is not the pending-game
manager and must not be destroyed to cancel games.
The separate host-announcement iterator also skips future rows for this cup;
its mixed lists continue publishing news for other competitions.
Played results and historical editions retain the original reader/writer and
0x50-byte record format, including years loaded lazily. A save without a cup
archive may recreate a dormant object on load through the original constructor;
the daily and annual guards still prevent future editions. Creating a career
without this competition intentionally changes the original RNG trajectory.

## EURO_CHAMP_QUAL.CPP: European qualification

The historical initializer inserts its 220 fixtures in original order. Its two
assert expansions reload TOC strings and consequently have different return PCs
from the common assertion helper. The custom record stream keeps 80-byte records
and two scalar bytes; load reconstructs guest pointers from the stored IDs.

The private RNG helper retains wrapping 32-bit multiplication and signed
sampling. Its first wide-range reduction always lies in [-32768, 32768], so the
original's deeper inlined wide-range branches cannot run. The ordinary draw uses
nine groups and a permutation of the immutable indices 0..8. Detailed proofs and
per-entry evidence remain private; the structural inventory is
`tools/euro_champ_qual_audit.py`.

## MATCH_PL.CPP: player state and decisions

The audited module starts at `0x102238a0`, before the parameterized constructor
used by the older inferred inventory. Its default constructor preserves untouched
record bytes, saves registers below the unchanged stack pointer, and reloads
those slots on return. It reads the same floating constant twice; retaining both
loads also preserves the result when the supplied record aliases that storage.
Acquiring possession can first clear the acquiring player itself. Receiver
rewards and subsequent possession writes reload the match after external calls,
so a call that changes the match pointer must affect the later accesses.

Parameterized construction retains its incoming side for the final heading,
while kit selection rereads the live side. Its first post-power home/away
branch uses the already-computed condition field; an extra side read changes
observer-visible behavior. The final probability gate can leave the old
personality byte untouched. Legacy original construction must decline MATCH_PL
for the whole preparation parent, including its direct constructor call.

The match range sampler's nested wide arms are unreachable: after wrapping
the signed bound times ten to 32 bits, division by 65535 produces a correction
bound in [-32768,32768]. Attribute refresh likewise has only narrow draws after
its signed16 condition conversion. Preserve draw order, inclusive pool-end
checks and reseeding while replacing those unrolled arms with direct arithmetic.
PowerPC floating branches use `ble = !GT` and `bge = !LT`, including unordered
comparisons; an explicit `cror EQ,LT,EQ` has different NaN behavior.
Legacy original/profile switches must also decline the new module's hooks;
otherwise a takeover silently turns a reference run into another C++ run.

Primary action selection owns a guest linked list of player copies. The original
population query appends at most two candidates; normal destruction releases
payloads and nodes, clears the head, and retains count, tail and cursor fields.
Keep destruction explicit on normal exits. A nonzero floating sampling bound
can truncate to integer zero: its nontrapping modulo continuation can then make
the following range wide, even when the first range is provably narrow.

Pass completion keeps the action core's mutable flag in the caller's argument
slot, preserving the other bytes of that word. The two completion paths share
their ordered distance sampler without adding a guest frame; the defender's
two distance lookups remain fresh reads. In direct contests, the diagnostic
area test uses the initiating player's side with the opponent's coordinates.
Its final delay bound is `3 - (trait0 > draw0) - (trait1 > draw1)`, with signed
traits and signed remainders; both traits precede the first reseed and the
first draw precedes the second reseed.

The common ball-action core retains one guest activation across contests,
trajectory publication and commentary callbacks. Its weather adjustment
multiplies quality by the integer conversion of scaled quality, rather than
applying a linear scale. Disciplinary descriptors retain their inherited padding
bytes; their side byte belongs to the initiating player. A rejected opponent
still leaves the original signed rating arguments live before later reseeds.

Response and decision sampling can pass a nonzero floating bound that truncates
to integer zero. Their audited quotient-free divisions leave the dividend as
the residual, followed by the original sign correction; introducing a host
division error changes that continuation. Nested wide corrections also retain
the caller's incoming F1 until reseeding, where external code can observe it.

Keeper response captures its skill and attribute bound before the attribute
draw's refill callback, and captures the match's wait byte before that callback
as well. The subsequent waiting draw uses this saved byte even if the callback
changes the player's match pointer, wait byte, skill or reach. Reloading those
fields after the callback would change the original draw sequence and rating.

Action rating reads condition twice and narrows each converted value to a signed
halfword before multiplying. Its resulting fitness bound is always narrow;
the separate geometry and attribute samplers still require their wide paths.
It may replace the action byte before selecting output flags. Those flags are
written second-output first, including cases where a draw occurs between writes.

Shot selection retains its lateral coefficient across reseeding while recomputing
other live floating bounds. Its special distance adjustment rereads the action
after the power call, so a callback can change the subsequent divisor and event.
The final event table gives the lateral flag priority over distance; action 53
consumes another draw when neither flag is set. Caller outputs may alias each other,
player attributes or RNG storage, and retain the original write order.

Parameterized player setup consults the actor's old side until the name-copy
phase. Availability byte 2 changes the position but preserves the condition and
cached-role fields. The optional second eligibility-profile query runs only when
the first result is at least 3; unordered results skip it. Preference comparison
uses signed bytes, but the subsequent subtraction rereads the selected byte unsigned.
The following random preferences use bounds `35 - signed8(trait)` and
`60 - 2 * signed8(trait)`: zero consumes no second draw, and neither can reach
its wide sampler. Personality reputation is narrowed to a signed byte before
the power calculation; a missing competition descriptor replaces both the
fixture ID and its competition pointer, rereading the replacement ID after the write.

Periodic balance consumes its preliminary draw before checking the current balance,
queries the role afresh at each decision, and rereads counters after a reseed.
The keeper's three wide-sampler arms are unreachable: byte bounds are at most
65,025, and the signed-halfword clock gives `90 - trunc(time / 11)` within
`[-2888, 3068]`. Preserve the signed remainder and carry effects of the original
narrow paths. Midfield ratios use single-precision intermediate operations and
separately narrow each contribution to the balance threshold to a signed halfword.

Peer contacts update every matching contact-list record before deciding whether
to append; duplicate matches are observable. Incident responses also retain two
consecutive kind2 attempts, each consuming its own RNG draw. Request, effect and
announcement callbacks can change the match or choice byte, so later phases
reload those fields before applying their consequences.

## DISPUTE.CPP: dispute records and notifications

`dispute.dat` stores a byte-array length, the person flags, a sign-extended
16-bit list count as a word, then raw 20-byte records. Loads before version
`0x3e046` decline; a failed load retains partial allocations for the constructor
to clean up before building empty storage. The original can pass a null list to
append after allocation failure with populated input; the external append then
faults on its first list read. Caller-boundary evidence is recorded separately
from returned comparisons.

Daily expiration compares years unsigned and days signed. It excludes the final
person index (`id < count - 1`) and rereads the live list count after removal,
while still incrementing the iteration counter. Both original quirks remain.
The sampler's wrapped signed `bound * 10 / 65535` correction is always within
`[-32768,32768]`, so the original's deeper wide-range unrolling and recursive
call cannot execute. Headline rendering deliberately reads news slot six twice.

## NATION_AWARDS.CPP: national awards and monthly ratings

The base-class constructor, destructor, daily update and collection methods
precede the first source-pool reference. Three sort callbacks and a one-instruction
virtual method complete the 20-entry boundary. Selection retains the original
threshold relaxation, signed-byte age checks and cumulative count of rejected
older players across passes; it does not reset that count at each threshold.

Serialization calls each child award first, then writes one byte indicating
monthly-rating storage, followed by that external object's existing stream.
Loading invokes the virtual reset before reading the flag. A failed monthly
object allocation still returns success with a null pointer, as the original
does. The global selector also retains its allocation when no candidate survives
the initial scan. Neither quirk is silently changed by reconstruction.

## FIFA_RANKINGS.CPP: international ranking points and history

The 15 original entries (0x104866f0–0x10487ff0, 1,575 indexed instructions)
use 44-byte nation rows and six-byte match records. `fifa_rankings_*` owns
match points, expiry, six-year weighting, sorted queries and persistence;
owned calls are direct C++ and both qsort transition vectors remain original.
The original counts some histories with signed bytes, clamps goals only above
five, applies asymmetric category bonuses, and rounds at individual PowerPC
single/double operations. These rules are retained explicitly.

The save stream contains all 44-byte rows (including original pointer bytes),
then each row's live 16-bit count and match bytes, the founding-month byte and
eight date bytes. Loading replaces the serialized pointer and sets capacity
to the live count. Failed IO preserves transferred prefixes; the original
load/save assertions leave the stream open, as in the original. Append retains
the original unchecked allocation result.

## INTERNATIONAL_AWARDS.CPP: six international prizes

The three entries (0x10440620–0x10440c70, 397 indexed instructions) share
a derived award-manager vtable. Its constructor retains the original virtual
initialization boundary and existing base-class load service. Initialization
keeps six pointer slots even when a child allocation fails; only successful
allocations consume award ordinals, and only the array failure reports an
assertion. The first award receives a stack-resident age/date restriction.

The annual method skips 2001, then checks 6 May, 6 June and 16 June using the
existing zero-based calendar constructor. It compares years unsigned and days
signed. Publication retains the ratings pointer across related calls while
reloading the live award array between them. Serialization, selection, ranking
and publication services belong to the already separate NATION_AWARDS module.

## AWARD_MANAGER.CPP: award groups, histories and news

The 21 manager entries (0x10436080–0x10437cc0) include two predicates
registered by transition vector. The preceding reusable base-award class
has a separate vtable and numeric pool, with no source filename established.
Group construction retains all 29 slots and chooses legacy variants from
the original sentinels. The save and load operations keep the same shared
stream, visiting groups in slot order and then the two histories.

The volume check copies an odd-byte template whose trailing NUL is read
from the guest stack beyond the copied bytes. The ballot context accepts
index 16, which reaches the notification count field. Both behaviors are
preserved as observed in the original.

## AWARD_SHORTLIST.CPP: candidates, ballots and voting

The fourteen entries include three sort callbacks and stop before Belgium's
default constructor at 0x10439fe0. The persisted stream is a four-byte
count/capacity header, sixteen-byte candidates, sixteen two-byte ballots and
an eight-byte date. Eligibility's two regional cases compare the regional ID
against the country field at +0x30, then test +0x34 for the -1 sentinel;
publication and automatic voting filter regions against +0x34 instead.
Preserve that original asymmetry. Automatic voting's signed sixteen-bit prefix
cannot reach its inlined wide random arms. The public sampler's first wide
reduction wraps bound*10 to signed32 before division by 65535; its result is
within [-32768,32768], excluding all deeper wide unrolls and recursion.

## SIPRINT.CPP: printing compatibility

The printer owns a 16-byte guest record: signed availability byte, resource-file
reference at +2, saved port at +4, session at +8 and page format at +12. The
static initializer registers its destructor; resource-file guards and filesystem
templates following that initializer belong to separate dependencies. Page
formats remain opaque `PFor`/1000 resource handles managed by the original
Printing Manager calls. Preserve the original calls that pass the address of
the session field to `PMRelease`, and its absolute line-index vertical positions
across page breaks. The zero lines-per-page remainder follows the generator
proof at `0x1074d174`; no replacement page-layout policy is introduced. Those
absolute positions put every line past the first page off the sheet (on a
QuickDraw port they would be clipped), so the host bridge subtracts the earlier
pages' length when drawing; the routine and the memory it reads are unchanged.

## DB_FILES.CPP: database tables on disk

Eighteen tables, each with a loader, and all but `oldstaff` with a saver; every
loader has a conversion from packed Windows records (little-endian, no padding) to
the Mac layout. Clubs, non-players, players and club competitions also read
version 1 files field by field. `oldstaff`'s loader keeps its values in other
registers and reads the version later, so it has its own register map.

A file ending in `0xf1fffff1` holds Mac records and is read whole. Otherwise the
loader looks for a converted `<name> <header>.dam` in `CM3_DATA`, and failing that
converts the file and writes that copy with the marker. A nonzero flag byte in `r4`
selects the pack-file stream routines and skips the marker test. The version 1
readers always read the original file, leaving an open `.dam` unclosed and a
conversion buffer unfreed. Clubs repair a zero at `+0x1ec` to 1 on every
current-version load, and the version 1 reader stores it times 500. `convert_staff`
copies byte `+0x6d` and then clears it. These are the original's behaviour.

The double-swapping conversions (continents, nations, cities) set r6-r8 before the
loop, so an empty table still leaves them. Their stack frames hold the last record's
doubles, unswapped and swapped. After a failed allocation the original calls `exit`
and has code to continue, which only the fixture's returning `exit` stub reaches.

## AWOL.CPP: staff absence records and the daily review

`AWOL.CPP` owns 21 entries / 6,958 indexed instructions between 0x100031b0 and
0x10009ef0 (`tools/awol_audit.py`), with 49 incoming calls, 48 calls between
entries and no callback vectors or computed transfers. `awol_adapter` replaces
the older `awol_records` builder owner in normal generation; those files stay
as reference tooling only.

The inline random draw appears at 31 sites with an arm for bounds beyond 65535
in size. Every bound there is a sign-extended byte or halfword, or a byte plus
ten (pinned by a test), so the C++ has no such arm, and nothing can reach
`sample` (0x10006080) in play; it is still rewritten and checked as an entry.
Quirks worth knowing: `construct` frees only the flags and the first list after
a failed load; removals XOR the flag bit behind a test of `flag | bit` that is
never zero; `remove_secondary_item` toggles bit 1 for any nonzero flag byte;
`training_event` reads the staff id again after its calls, so a callee changing
it changes which record is written; `fitness_event` retries forever when no
reporter exists and no word gives grade 0, as the original does; `notify` and
`register_absence` read through the club unguarded, so a callee clearing it is a
guest fault in both. The fixture's scripted services never see leaf routines
(0x10738c00, 0x10775550, 0x10768810, 0x1037fa70): `run_until` runs those before
the probe, so a case steers them through the memory they read.


## BRA_REG_CENTRAL.CPP: Brazilian Central regional championship

The 14 entries at 0x106d73f0–0x106d9170 (1,862 indexed instructions) live in
`native/bra_reg_central_*`; `tools/bra_reg_central_audit.py` pins them. A
15-club league of 15 rounds is followed by an eight-club knockout: the first
stage advance copies league rows 0, 7, 2, 5, 6, 1, 4, 3 into the bracket. Unlike
the English modules, destruction releases the stage array even when null and
keeps the pointer. No existing capture reaches these routines (63 censused), so
`bra_reg_central_career_state` builds the competition on a real career's
database. That check runs the real club scan, RNG, calendars and knockout record;
it scripts club history, the fixture builder, the saved-competition load and the
season route because they stop at file or dialog imports, so those calls have
their arguments compared but not their effects. Fixture slots and base-class news
are covered by fixtures only.

## BRA_REG_PARANA.CPP: two turns and an optional final

The 16 entries at 0x106f6a80–0x106f8d30 implement the 12-club Paraná
championship. Calendar stage -1 supplies 22 league rounds; stage 2 supplies a
two-leg final. Fixture round indices below 11 select the first turn, then the
second. Forwarding a fixture temporarily clears the parent's fixture pointer
and restores it after the turn callback. Equal turn winners reduce the stage
count to two, mark the title directly, and leave the original eight-byte
finalists allocation unreleased. These original behaviors are preserved.
The module owns no serialization routine; loading remains in the shared
competition service. Its checker scripts external competition/news boundaries
and also exercises the real date helpers.

## CUP_STAGE.CPP: knockout-stage records and draws

The eight entries cover construction, allocation for load, destruction, entrant
initialization, binary load/save, draw links and the stage-local RNG. The loader
deliberately tests the preceding round read's return value again after reading
entrants; the original does not test the entrant read's return value there.
Saving temporarily replaces club pointers with their database indices and
restores them afterward. Draw links fill the home feed before the away feed;
the host-country swap and RNG consumption retain that order.

`cm-cup-stage-checks` compares every entry against separately generated original
bodies, including partial IO and fatal assertion exits. Its `--production` mode
uses the normal hooks with a memory observer and no instruction budget for owned
code; only the base constructor, CFM trampoline and version-text leaf receive
an external instruction allowance. `CM_CUP_STAGE_ORIGINAL=1` retains the original
route for paired replays and the `--original-control` negative check.

## EUROPEAN_CUP.CPP: the Champions League

The 23 entries at 0x104e9fb0–0x104ef380 (5,320 indexed instructions) live in
`native/european_cup_*`; `tools/european_cup_audit.py` pins them. Thirteen stages:
stage -1 is qualifying, 0–7 and 8–11 the two group phases, 12 the knockout. The
first draw builds 32 pots from the 16 seeds plus qualifiers with status 1, pairs
same-nation clubs and swaps within each half of the draw to separate nations. The
second draw retries itself up to 20 times, then fills any free seat. Group draws
hand qualifiers with status 2 and third-placed clubs to the UEFA Cup object.
Quirks: `collect_qualifiers` reads before the entrants table when the count is 0,
and an allocation failure in a draw carries on through null, so the checker maps
page zero. The first draw's wide-range sample can't run (the bound is a sum of two
signed bytes), so `random_below` is reached only by synthetic calls. In the
checker, `check-%m.profraw` lets forked children merge coverage into one small
file, where `%p` would expand before fork and overwrite.

## UTILS.CPP: text, path and file helpers

The 23 entries at 0x10745e00–0x107475ac (1,488 indexed instructions) live in
`native/utils_*`; `tools/utils_audit.py` pins them. 0x107474a0, which seeds the RNG
and draws the sample-pool cursor and salt, stays routed through
`try_modern_rng_core` for its diagnostics and calls `utils::initialize_random`.
`rng_core_checks` pins the order of every access and the whole register file
there, so that routine writes its prologue by hand instead of using `Frame`.

Quirks worth knowing: `usable_path` never fails on a nonempty string, because its
handle stub 0x10739870 always returns 0x1e. `environment_directory` maps
CM3_TACTICS to `data`, and `-ignoreenv` in the command line skips `getenv`.
`short_name` drops spaces only once a name reaches 31 characters, and then keeps 22
characters plus the name's last eight. In `read_line` a `#` starts a comment
anywhere, also inside a line, whose text then continues on the next line; byte 0xff
reads as end of file. `copy_file` retries a failed 256-byte allocation forever, and
a read returning nothing before the recorded size spins forever. `delete_temporary_files`
lists 100 names at a time and searches again after a full listing.

## FILE_SCREENS.CPP: the file browser, save information and message boxes

The 11 entries at 0x100edf20–0x100f0ff4 (3,109 indexed instructions) live in
`native/file_screens_*`; `tools/file_screens_audit.py` pins them. 0x100f05a0, the game's
message box, has 216 callers. 0x100f1000 and 0x100f1010 sit right after the module but
belong to FINANCE.CPP, the only code reading their vectors.

Quirks worth knowing: a save from before 0x3e023 closes `general.dat` twice in the file
dialog's Info path. The summary screen reads 0xf70 bytes below version 0x3e027 and zeroes
the saved-time field, which it then still displays. Its Addictedness row appears only in
language 0 and past a full day of play. Play time accumulates in `fill_save_summary` by
wall clock, without checking for wrap. The action handlers' sign mask after 0x102a83a0
leaves r3 unchanged, so the C++ omits it.

`cm-file-screens-checks` has a second pass from `ui/game-save/initial.snapshot`: the real screen
manager, controls and text run, with only files, the clock and the error dialog scripted. The
snapshot starts with no group current (16), so the pass makes group 0 current. A screen already
in the history refuses to open again in the real manager, which is how it reaches the openers'
refused branches. The checker's scratch memory must stay off 0x60000000, the game's real heap there.

Its real-save cases read a copy of the reference save through the native file services. The native
FSRef holds the host's process id at +0xc, and `native/file_reference_bridge.cpp` copies it onto the
guest's stack, so two runs of the original differ in those bytes. The checker replaces that exact value
with a marker in the five slots these screens leave it in, so its presence is still compared. The system's file services don't survive a fork after
the parent has used them, so anything reading files in the checker's own process must run last.

`.local/benchmarks/ui/save-info` is the capture for the save information screen, recorded with
`tools/game_driver.py` on a copy of the reference save: load dialog, pick a file, Info, Ok, twice, cancel.

## MATCH_ENG.CPP: the match object and its tick

`CM3_MATCH_ENGINE`, 235 entries at `0x101d16a0..0x101fef60` (`tools/match_eng_audit.py`). All of them
are hooked in the generator from the start, and 79 older builder entries of 19 families left production
at that takeover. Their sources still build for checks but no page calls them. The 98 short routines
in `match_eng_leaves.hpp` run in line at generated call sites. C++ calls into the module go through
`match_eng::call`, which runs the hook in place and reaches the dispatcher only for a routine the module
doesn't list.

A cell has a row at 0x791d (0..8, as a player's +0x105) and a column at 0x791e (0..11, as +0x106), so an
end area is rows 2..6 and columns 0..1 or 10..11. `0x7924` is the side with the ball, -1 while it's loose,
and `0x7921` the side that touched it last. The kick-off flip never leaves 0x7924 at -1, and the
nearest-player search reads before the player table if a kick-off sees -1, so a case claiming that state
proves nothing. A draw of bound N answers 0..N-1 for a positive bound below 0xffff, which makes many of
the original's negative-result branches dead.

Argument registers matter here more than elsewhere: routines leave r3-r10 as scratch, and a caller's
next varargs call stores them on the stack, where a later callee can read them. The coverage cases
found real differences of that kind (r5 at 0x101e8b24 and 0x101eb5dc). The equivalence target for this
module is therefore registers r3-r10 at return and r13-r31, f14-f31 at every call, not just results. A
return address is part of it too, since the callee's prologue saves it in the caller's frame:
`tools/test_match_eng_audit.py` pairs each literal call's target and return address with the listing.

Some paths have no case, since a match snapshot can't produce the state; each lands on a join a covered
branch reaches, proved from the listing: `shot_state` -1 in the kick (only the actor module's 0x1022e3a0
stores it), `extra_time_flag` changing during the review's or the tick's loop, and 0x10281af0 changing
the carrier at +0x3104. Faults kept on purpose fault alike on both sides: 0x101f5350 with player A absent
and B present reads +0x70 of A (the original's quirk), and 0x101f7c10 with no grid reports its assertion
and stores through 0.

`cm-match-eng-checks` runs each entry's cases from a real 600-advance snapshot, then mutates values the
original read, then (`--call-trace`) compares r13-r31 and f14-f31 at each `bl`, and the count and order
of the calls both sides dispatch, which the end state can't show. A match snapshot lacks the discipline manager (the global at -0x7aa4 is null), so the checker answers
its suspension query, the computer's lineup choice and the dialog entry. A case's `answers` only reach a
routine dispatched on both sides, so the checker wants only the module's routines: another module's
hooks decline and its originals run on both sides. A routine wanted by the checker would run in place in
the original, past the answer, while the C++'s dispatched call received it: the difference that gives is
false. `--no-live` runs the cases at three match states, and live runs at one, so a final coverage count
needs both. Leave `LLVM_PROFILE_FILE` set on any coverage run: the default path grows by about 500 MB
per process and filled the disk once.

The module is not faster in wall time: the C++ keeps the stack image and scratch registers, so it does
about what the generated code does. Guest instruction counts drop by 45%, which shows nothing here.

## media.cpp: the press

`media.cpp` owns 75 entries / 99,976 indexed instructions in 0x10646d90-0x106a8a50, twice MATCH_ENG
(`tools/media_audit.py`). Its pool (-0x41f8) and floats (-0x41fc, -0x4200) are loaded only inside the range;
the next unit (pool -0x41e8, the pundit files) has no source name. Most of it is inline RNG sites and
text: 1,890 formatter calls and 508 refills. Those compress well: a text routine is a struct of the story's
variables and decision trees whose leaves are a phrase, a return address and named arguments
(`native/media_text_b_*`: 6,228 instructions in about 1,100 lines).

`cm-media-checks` compares cases and mutated inputs from the real career snapshot. Three things it adds to
MATCH_ENG's checker: `--argument-masks` compares at each call the r3-r10 the callee reads, worked out from
the export's p-code by `tools/call_arguments.py`, because a wrong argument to an answered callee shows nowhere
else; a routine under check is never answered, since its C++ doesn't reach the dispatcher; and `--null-page`
maps low memory, which the original machine could read, so a path through a null pointer returns and is
compared. `answers` can't stand in for a routine another module owns, which the original reaches past the
probe. Normal runs trace the first cases and each 64th: `--mutated-calls 1000000` traces all of them.

Quirks kept on purpose: two history searches have no loop increment and look only at row 0; the managers-list
loop in 0x106487d0 compares a rating with itself plus 750 and never passes; the draw's negative bound always
clamps to n-1; a failed data-file load never closes the file and leaves the object part filled; 0x10699d10 stops
at index 57, and a member weight above 127 reads as a signed byte; 0x10698f50 keeps the previous row's manager
for a person with no club; 0x10649fb0 reads the caller's r30 when the player has no key. The compiler overlaps
pairs of draws (`random(5) - random(5)`): the first remainder is computed before the second advance, and the
second refill sees the first's scratch in r3-r6. Non-volatile registers serve as temporaries inside stories, so
they reach callees' saved images, and the formatter spills r3-r10 including leftovers. Outgoing stack words go
before the register loads, so a wild pointer faults after those stores; the checker compares memory at such a
fault. With a negative player, a tone above 7 and no matching trait, 0x1064cf80 retries forever on both sides. The
call trace doesn't pair formatter calls (the original's page reaches that hook directly): their arguments show
through the text and spills they leave.

## NEWS_SCREENS.CPP: the news screen and the message dialog

`NEWS_SCREENS.CPP` owns 8 entries / 2,249 indexed instructions in 0x102ce710-0x102d0a50 (`tools/news_screens_audit.py`).
Its pool is -0x50f0 and its six transition vectors are the screen manager's callbacks; 0x102d0a50 already belongs to
OFFICIALS_MANAGER. `cm-news-screens-checks` is media's checker with each case in a forked child, so a case can set its
screen up through the game's own routines on their original routes. A snapshot sits outside any routine: r2 is zero, and
the screen manager's current group is 16 until a client runs, so the fixture sets r2 and selects group 0 (the human manager).

Quirks kept on purpose: the title's 57-label jump table sends only the twelve vowels (both cases, Y included) to the
French "d'" format, on the name's signed first byte; `news_command` recomputes the control's action for each test,
addressing through r4 until the last test of each chain, which uses r3; with nothing selected the screen picks the last
listed item that is unread or has an action (the item's priority byte only counts with a null filter, which play never
produces); the send-to-all loop walks the sixteen human managers, the last people in the table.
