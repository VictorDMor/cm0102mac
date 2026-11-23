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
- **Stack leftovers that a reader reads.** The original reads stack bytes it
  never wrote (docs/aot-port.md, "Uninitialised stack reads"). Under the
  per-reader contract (CLAUDE.md, 2026-11-06), a rewrite leaves the bytes that
  a reader listed by `--stack-audit` reads: saved registers, LR, back chain,
  locals or callee residue. Where a reader reads a callee's residue, the
  rewrite calls that callee at the original stack pointer. Any other byte
  below the stack pointer is free. Routines written before 2026-11-06 still
  leave the full image; that is allowed, but no longer required.
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

Since 2026-11-06 a rewrite is native C++ inside: frames and the registers at
calls matter only where the per-reader contract above names a reader. The pieces
below are for those places, and for the ABI adapter at the edge.

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
  game still starts. Since 2026-11-14 the same holds for a document under a
  folder with such a name or whose whole path passes 255 bytes: the original's
  full-path builder (`0x1076ec60`) and its callers keep names and paths in
  64- and 256-byte stack buffers without checking. Listings leave such
  entries out, and a catalog lookup by directory ID refuses such a name. `game_driver.py launch NAME --open DOC` launches that way.
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

## BRA_NAT_SECOND.CPP

The [audit](../tools/bra_nat_second_audit.py) closes 13 entries and 1,916
reachable instructions between BRA_NAT_FIRST.CPP and BRA_REG_NORTH.CPP.
The normal owner covers construction, teardown, both calendars, ordered club
selection, the season transition, group creation and updates, fixture placement,
results and announcements. `CM_BRA_NAT_SECOND_ORIGINAL=1` retains the independent
generated reference for comparisons.

The original calendar writes the optional phase flag and the count/start
headers before allocation. It rereads the signed year before each date call.
Playoff seeding reads the live standings in the order first, fourth, second,
third. The group destructor rereads the inclusive last group index after each
callback. Construction retains the caller's live object home slot. Owned calls
stay in C++ with CFM descriptor and TOC semantics, including checker paths and
the two generated page boundaries.

Base competition persistence, promotion, standings updates, date builders,
group and fixture services, allocation, diagnostics and result/history services
remain external. The module owns no serialized format or RNG entry. The checker
runs the complete owned call graph against the independent reference and
validates the arguments and effects at modeled external boundaries.

New Série B seasons use the existing 27-round, single-leg calendar and four
automatic promotion places. The saved group capacity is zero, so the base
league's completion gate never creates a playoff stage. The end-of-season
callback uses the base league finalizer, whose existing history record stores
the first and second rows as champion and runner-up. The shared league
classification marks the first row as champion and the season exchange moves
the top four to the first division. An active legacy season keeps its saved
group capacity. Rollover tears down any legacy group and
releases its array before selecting the new format. The original competition
rules remain available with `CM_BRA_NAT_SECOND_RULES_ORIGINAL=1` for differential
checks.

## HALL_OF_FAME.CPP

The [audit](../tools/hall_of_fame_audit.py) closes 20 entries and 2,736
reachable instructions. The default C++23 owners cover initialization,
ranking, imports, binary storage, localized categories and text reports.
The engine startup adapter delegates the global table initializer to the
same module owner and retains its existing diagnostics. The independent
generated reference remains available with `CM_HALL_OF_FAME_ORIGINAL=1`.

The binary format uses an eight-byte header, a 324,000-byte encoded table
and a four-byte checksum. A short write after encoding can leave the live
table encoded. A short payload or checksum read clears the primary table.
Import decodes a scratch table and leaves it intact even on checksum failure.
Its silent validation mode leaves the live ranking alone.

Import checks all 120 bytes for duplicates in the first 99 rows. The ordered
field copy leaves the destination's trailing padding in place. The report
walks the original category and slot order, skips rows with zero scores and
preserves the slot number across gaps. Shared date, text, sorting and stream
services remain external.

## The rb_*.cpp rulebooks

43 units, one per ruling body, each one class over RULING_BODY.CPP's base rulebook. In every unit
the routines come in this order: constructor, report of a match's cards, nation text, "next card
bans" query. The class's virtual table names the last three (+0x10, +0x18, +0x1c), the constructor
that stores that table is called from the rulebook factory (0x100e7xxx-0x100e8xxx), and the nation
text's assertion names the unit's source file. A unit therefore **ends** at the routine that loads
its file name; the two routines after it open the next unit.

The delivery of 2026-11-05 took the name the other way round, so `native/rb_greece.cpp`
([audit](../tools/rb_greece_audit.py)) owns two routines of rb_greece.cpp (text 0x106ab7d0, query
0x106ab890) and two of rb_ireland.cpp (constructor 0x106ab8f0, report 0x106ab970). Its files keep
their names; `CM_RB_GREECE_ORIGINAL=1` selects the generated bodies of those four. The Greek query
bans on every third card; the report it was delivered with is the Irish one (bans at 5, 8, 11 and
then every third card from 14, a hearing on the first Thursday after a dismissal).

Units written since are one family: `native/ruling_bodies*`, [audit](../tools/ruling_bodies_audit.py),
`cm-ruling-bodies-checks`, `CM_RULING_BODIES_ORIGINAL=1`. What repeats between units is a function
with a small description per unit (`NationText`, `Rulebook`); a unit's own rules are its own
function. In it so far: rb_ireland.cpp's text and query and the whole of rb_northern_ireland.cpp,
whose class also replaces the base's slot at +8 with a routine that starts every booking record's
points again.

- The queries write their optional outputs before reading the tally, which they may alias.
- A report builds the 0x18-byte ban record in its frame and never initialises all of it. In the
  Northern Irish one the flags byte at +0x14 has bit 6 cleared and bit 5 set from the match, and
  its other six bits, with the bytes +9..+0xb, +0x11 and +0x15..+0x17, stay as an earlier call left
  those stack bytes. The record goes to the discipline manager (0x100dfb30), which keeps
  it. The C++ leaves and reads the same stack bytes; what is there now that callers are native can differ from
  the original (the open risk INJURY.CPP and TRANSFER_OFFER.CPP have).
- The Northern Irish report counts yellow cards with a signed byte against the unsigned count: 128
  or more would never end, in the original too.
- The checker's script must cover the routines' whole ranges, not their entries: the original comes
  back from each call to an address inside the routine, and from there the production page would
  run, with its in-place calls to C++ owners, instead of the reference copy.

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

All 15 audited entries use handwritten C++23 in the normal application.
The January week replay checks 28 actual daily updates. The controlled
tournament-start replay exercises the exact deadline, entrant draws, final
placement callback, group creation and the repeated update on the same date.

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

Entrant selection keeps the seeded qualifiers in place and sorts only the
remaining eligible nations by signed reputation. The fallback scan caps the
pool at fifty. Four groups each draw one club from each of four pots. A draw
of an already selected slot consumes another sample. The cursor advances
before the table read and a wrap refills the salt from the shared RNG.

The daily update compares an unsigned year and a signed day, with the
low-byte mode controlling the date check. On the deadline it runs entrant
selection and publishes final placement through the live virtual slot before
creating the initial standings and three groups. It retains the original
failure handling, including proceeding after individual group failures and
skipping a second draw after the competition has started.

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

The recursive shot event at `0x102576e0` owns a 432-byte frame, all seven
child call sites and their continuations, and the common return. Both the
ordinary caller and the awarded-goal debugger use the same C++ phases. The
debugger observes the setup halfword and statistics word at their original
guest PCs, runs commentary to its exclusive stop, and resumes at score
publication. The module-wide original override and guest PC sampling still
select the generated shot body. The paired shot replay holds all other
routes constant and requires the original and C++ to consume the same number
of shot activations.

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

## BRA_REG_GAUCHO.CPP: league and knockout

The 14 entries build a 17-club league followed by a four-club knockout. The
semifinal seeds use league rows 0, 3, 1, 2. The season rollover destroys the
old knockout and reuses the stage pointer array. Both third and fourth
positions receive seed 3 in the fixture list. Calendar stage -1 supplies the
league schedule and stage 0 supplies the semifinal and final.

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

Authorized correction (2026-11-15): the injury counts in 0x10649fb0 and 0x106620b0 query each occupied
squad member instead of repeatedly querying the story's subject. This corrects the news count and the
reaction selected at the existing four-injury threshold. `cm-media-checks --injury-roster` exercises real injury updates and compares
ordered RNG writes with the original amended only at those two query arguments; `--raw-original` retains
the uncorrected oracle for diagnosis.

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

News (`0x102cea30`) opts into New Screens through shared Yoga list/reading tracks. The builder computes its visible
headline capacity before selection and scroll restoration, and reflows the original rich text at the available width;
its links therefore retain the original measured spans. The article reserves a scrollbar gutter. Extra height favors
reading, headline growth is bounded, and both capacities stay within AutoGrid's 30-row limit. The postpass keeps dates
compact and stretches the original four top/four bottom tabs; it preserves guest registers, stack, commands and history.
The original localized labels/fonts/colors and acknowledgement rules remain authoritative. Off/800x600, secondary
groups and network sessions retain the legacy layout; message composition is outside this recipe. Focused checks use
disposable existing captures to compare real inbox records/RNG and original tab, selection, filter and unread commands.

## DATABASE.CPP: records, lookups and queries (rewritten by layer)

`DATABASE.CPP` is 189 routines / 64,495 instructions in 0x1009efc0-0x100de410 (`tools/database_audit.py`; the test
re-derives every entry and body from the export). It is the first module rewritten by routine layer rather than whole
(Victor, 2026-11-06): `tools/game_inventory.py` lists the routines that call no generated code, and those go first. The
22 routines after the last DATABASE.CPP source-name load have no evidence of their own; the unit's static initializer
0x100de3b0 (already C++ under engine_initialization) closes it, as initializers close CONTRACT_MANAGER.CPP and
HALL_OF_FAME.CPP.

The first block is nine leaves: six three-slot preference lists (a club's two lists at +0x5c/+0x68; a person's
preferences record at +0x18 with lists at +0x4, +0x10, +0x1c, +0x28; the meanings are inferred from the callers' score)
and three attribute ratings that turn a signed byte into 1-20 through a 16-bit value of the same record. Called from
generated code, a leaf leaves what the original leaves: the ratings store the int-to-double words and the `fctiwz`
result below r1, set f1-f7 and the FPSCR through `guest::Fpu`, and leave cr0 from whichever `fcmpo` ran last; the lists
leave the record pointer in the register each slot reload used (r5, then r4 or r3) and cr0 from the last compare,
with XER[SO]. The ratings add 0.5 again after each compare instead of reusing it, so the FPSCR sees each add.
`cm-database-checks` compares every byte value with 17 values of the 16-bit field, and every hit position, miss,
duplicate, null record and logical extreme of the lists, with `strict_volatile` and, in process, the whole register
file (f0-f31, r0, XER, CR, LR, CTR) and FPSCR, then runs every case through the production page with no instruction
budget. A C++ variant without the leftovers waits for the first C++ caller.

Block 2 adds four routines. The contract-flag queries 0x100d3150, 0x100d3230 and 0x100d3300 share one body: with no
club they ask CONTRACT_MANAGER for the primary contract and store its answer, null included, in the shared "last
primary contract" word (TOC -0x7b78) that SHORTLIST_MANAGER reads too, then derive the club from the table (index x
0x248, 32-bit wrap); they call CONTRACT_MANAGER's C++ in place, at the original return addresses, with r0 holding the
caller's LR as `mfspr` leaves it. The checker scripts both lookups and records every call's registers in guest memory.
0x100db1b0 builds a position mask from the player's ratings: "natural" tests compare the unsigned byte with 20, the
"above 10" and "at least 15" tests the sign-extended byte, and the last test of the second pass loads into r3.

The rest of the module is written in parallel blocks (lookups, records, text, generation, search, people, the loading
parts), each owning `database_<block>*.{hpp,cpp}`, `database_routines_<block>.inc` and
`database_checks_<block>.cpp`; `cm-database-checks --only` runs one block. Its ownership pass fails when the production
route enters DATABASE.CPP's code anywhere but at a hooked entry (an instruction budget can't be used: other modules'
leaves run through `run_leaf` before any probe). Four attribute copies (0x100c5ab0, 0x100c6150, 0x100c63c0, 0x100c65d0)
were also register-promoted leaves (`native_record_fields`), whose hook ran first: a DATABASE.CPP owner now supersedes
that lowering, and the audit requires the owner's hook to be the first at its entry. Fixture stubs that compare `s.pc`
with a literal give that address a run of its own at generation, so pages change around them.

All 189 routines are C++ and hooked (2026-11-07). The whole-file loader 0x100a55a0, the saver 0x100abfb0, the index
resolver 0x100af8a0, the language loader 0x100b6050 and player initialisation 0x100b8cf0 have no capture that replays
through them except world/database (load and resolution): their evidence is the fixtures. The saver writes 0x1cc bytes
of stack it never wrote into index.dat (a stale-stack reader: keep the stack image before it). Quirks kept: the
virtual-staff "clamp" at 0x1009fd98 sets the count to count + 0x52d + kept rather than clamping; two resolve clears hit
the neighbouring field; the attribute-settling loops hang when fewer than twelve slots qualify. Checker runs need
CM_UTILS/DB_FILES/SCREEN_MANAGER/SHORTLIST_MANAGER_ORIGINAL (set by the ctest).

## MATCH_MAN.CPP: lineups, player ratings, team selection

`MATCH_MAN.CPP` is 23 routines / 21,056 instructions in 0x102065e0-0x1021b010 (`tools/match_man_audit.py`; the test
re-derives every entry, body and switch table from the export). Ten routines had no source-name evidence; the bounds come
from the unit's TOC slots (-0x5294..-0x5274, between MATCH_EVENTS.CPP's pool and MATCH_OFFICIAL.CPP's slots), the
function-pointer vectors (each unit's group in unit order puts the lone `blr` 0x102065e0 in this unit) and MATCH_OFFICIAL.CPP
opening with its referee constructors 0x1021b010/0x1021b020. One switch (bctr 0x10210944) keeps its bound 13 instructions
back, out of `check_jump_tables.py`'s reach, so the audit pins it; the other three tables list their cases in reverse address
order (case 0 is the last label). 0x1020e8f0 was actor_construction's builder; the MATCH_MAN.CPP hook replaces it and the
audit requires it to be the only owner hook at that entry.

The module straddles the day and the match, so its rewrites keep the full stack image. The hottest are the judged ability
0x10219e40 (about 2 million calls on same-save-continue) and the two player appraisals 0x10216830 / 0x10211e70, which share
their date window helpers (`match_man_rating_b_parts.hpp`). `cm-match-man-checks` runs cases on the season-day career
(`--world`) and the 2262 match (`--match`, with live advances), mutations, the call trace with argument masks, and the
production proof; it fills r13-r31 and f14-f31 with distinct values before each case, because snapshot registers of 0 hid
a rewrite that cleared f28-f31 on entry. A trace that reaches its 2,000,000-call limit is still paired call by call, but its
counts are not compared. Answers only reach routines the original dispatches: generated pages call other modules' C++
owners in place, so answering one gives a false difference. A routine under check is never answered either, so the cases that answer
the classifier, the arranger or the pick run under `--only 0x10206ea0,0x102104d0` (`match_man_equivalence_answered`).
The game's random draws (halfword + seed word) are divided signed (mulhw), so remainders are taken on the int32 value.

Quirks kept: 0x10209d10's second and third levels and its self-call are unreachable (the second bound is 10n/65535 of a
32-bit n); 0x102102d0 rolls the staff pointer itself as a number; 0x10210c50 doesn't reset the best score between places and
rates the occupant with the candidate's side record; 0x1020d7d0 copies the lineup record piecewise around padding bytes.

Intentional simulation correction (2026-11-15, authorized by Victor): substitution comparators 0x1020ea00 / 0x1020ee00
now form a consistent ordering. The scaled comparator uses each player's own role, with weights 1.35 for role bits 2/4,
1.20 for 8 and 1.10 for 16 (the remaining conversion branches are unchanged). Both comparators tie players in the same
special-priority bucket and compare signed score keys without overflowing subtraction. This changes selection, call/stack
residues and volatile floating-point status where the corrected path differs; it is not claimed equivalent to the original.
`CM_SUBSTITUTION_ORDER_ORIGINAL=1` restores the original policy for diagnostics, and `cm-match-man-checks` explicitly selects
it for reconstruction checks. `cm-substitution-order-checks` instead tests the corrected production routes with ordering laws,
original-defect controls, ABI/record-write checks, numeric extremes and all 720 permutations of six candidates for each
comparator through the game's qsort. Equal keys have no guaranteed stable identity order. The save format is unchanged.

## MANAGER_MANAGER.CPP: manager records, jobs and the result tables

`MANAGER_MANAGER.CPP` is 105 routines / 49,690 instructions in 0x10193630-0x101c42f0 (`tools/manager_manager_audit.py`;
the test re-derives every body). KEY_NATION.CPP closes before it with its static initializer 0x101935b0 and the
destructor 0x101935f0, which sits in KEY_NATION.CPP's transition-vector group (each unit's group runs in descending
address order); MANAGER_SCREENS.CPP opens at 0x101c42f0, whose unit slots adjoin its pool -0x5310. The module's slots are
-0x5360..-0x5420, its source pool -0x5368. Nine jump-table cases of 0x101b3c90 and 0x101b83f0 are also discovery roots;
the audit folds them into those routines.

The list search 0x101b1620 (2.8 million calls on season-day) keeps the native lowering `list_search.cpp` as its fast
path, which skips the cursor's intermediate writes; where that steps aside (observers, aliasing writes, a null element)
the owner follows the original, `list_next` 0x10738e00 inline. Written as plain C++ without the fast path, it cost 2%
of the day (5 pairs). An epilogue's `lmw` leaves Ghidra's `tea` temporary past the restored words (`lmw_leaves`); the
season replay no longer compares `tea` at day boundaries, since no game code reads it before writing it.

Every search of a day comes from SHORTLIST's candidate filter (0x1035f368), always on the same 53-element list, and
finds almost nothing (season-day: 2.84 million searches, 150 million steps, 1,990 found). The fast path answers from
a derived index (`list_search_index.hpp`): each key's first node and position, and the last node for an absent key,
so registers, the condition field and the charged instructions stay the walk's. That list is MANAGER_MANAGER's object
(`*TOC[-0x79f0]`) +8, its 0x28-byte offer records keyed by person id at +8. Its writers: the object's constructor
0x10193630 and destructor 0x10193a20, the load 0x10193b10 (appends), the daily update 0x10196390 (removes; it also
writes byte +1, not the key), 0x101aaa90 and 0x101ac280 (append, and remove then append to change a record); the cell
itself is set at 0x103463c8 and 0x1034b0f4 and cleared at 0x1034df8c. Nothing writes a linked record's key. Every
append and removal runs the utility-object node routines, which note a mutation on the State; the index is rebuilt
after one, or when the container, header, count or head differ (4,593 rebuilds on season-day). It is on in the app and
`cm-world-replay` only (`set_native_list_search_index`); `CM_LIST_SEARCH_VERIFY=1` checks every answer against the
walk, `CM_LIST_SEARCH_INDEX=0` turns it off.

The job assessment 0x101ad080 (`manager_manager_assessment.cpp`) keeps its whole 0x1f0 frame: the stack audit finds
later readers of its int-to-double and `fctiwz` slots. Quirks it reproduces: one conversion (0x101af7cc) is `fsubs`, so
that rule rounds the score to single before its `fmuls`; a path with a null person+0xc reads the caller's r26, never
written there; the three later null tests of the staff record (+0x1c) are dead, since the entry already rejects it
with -10000 and nothing in between writes it.

The manager review 0x101a8ad0, its departure check 0x101c1690 and the module's draw 0x10195e10
(`manager_manager_review.cpp`) also keep their full frames: this module's checker compares every stack byte a call
writes. The draw walks a halfword table (cursor `*TOC[-0x7b84]`, wrapping at `*TOC[-0x7b88]` with a new base from
0x1077b770). A bound beyond ±0xffff splits through floating point into a share and a next bound `bound*10/0xffff`, but
the product wraps to 32 bits, so the next bound is within ±32768: the original's third and fourth stages and its
recursive call (0x101961ec) never run. The review inlines the same draw only with byte-sized bounds, so its inlined
floating-point paths are dead too. The review reads club+0x90's record unchecked (0x101a8bc4), so callers' clubs always
have a manager; its later null tests of +0x90 are compared with `--null-page` cases.

The post-match update (0x1019e8f0, its per-club 0x1019e990 and 0x101c15f0; `manager_manager_update.cpp`) is the
module's hook in the post-match broadcast 0x101cd880. Its own inlined draw has a signed byte plus one as its bound, so
its floating-point branch (0x1019f404..0x1019f4d4, with the call of 0x10195e10) never runs. Its cases use real fixtures
found in the season-day world; where they let the review run to its end they empty the people table, since the offer
value (0x101abda0) then scans every person through nested job assessments, past the call trace's limit.

The manager news text 0x101b9b50 and its comment 0x101bfd10 (`manager_manager_news.cpp`) build the item in text
buffers of a 0xd70 frame through 105 formatter calls; the event-code dispatch is a table of 46 rules (code pairs, format
offsets, argument layout). Dead in the original: compose's paths at 0x101bb558 and 0x101bb598..0x101bb664 (they need
the actual score above the expected after it was found at most expected - 4) and the comment's cases for 2 and 3
(0x101bfe2c, 0x101bfe84, 0x101bff68, 0x101bffc4; those values are only ever 0 or 1). With its flag set, the comment
always ends with an empty text.

The board's reaction 0x101a2eb0 and its judgement 0x101a05c0 (`manager_manager_board.cpp`): the judgement fills two
halfword flag sets in the caller's frame from the person's standing against the club's reputation (single-precision
`fsubs`/`fdivs`), and the reaction moves the club's board row by a table of flag bit → steps. The judgement's first
`lfs` fallback (0x101a0794) is dead: the valuation is forced to at least 1 at 0x101a06fc. The stack audit lists a reader
(0x10bf7bbc) of bytes the judgement leaves, so its frame is kept whole.

The board's verdict on a transfer deal 0x101a0f50 (`manager_manager_deal.cpp`, called from another module): its six
inlined draws take bounds made of sign-extended bytes or a halfword, so their floating-point stages (the calls of
0x10195e10 at 0x101a111c, 0x101a131c, 0x101a1478, 0x101a161c, 0x101a19dc, 0x101a1b98) never run; only the glue number's
draw (0x101a184c) can be wide. The flag argument is stored at 0xfb(r1), in the caller's parameter area, and reloaded
before the price; the rewrite branches on the reloaded byte too. `ble`/`bge` after `fcmpo` branch when GT/LT is clear,
so an unordered compare takes them: testing only GT or LT is exact (a review suggested otherwise twice; the checker
proved the suggestion wrong).

The daily update 0x10196390 reads a stack slot it never writes (sp+0xb4 at 0x101964fc, left by its caller's earlier
callee 0x10371b2c; the stack audit lists it), so its rewrite reads that slot from guest memory in the original's frame.

The text about a club 0x101980a0 (`manager_manager_people_text.cpp`; helpers 0x10197e00, 0x10197e80, 0x10197f00) picks
one sentence by the days (under 30, under 60, more), the relation 0x100f55b0 gives, two bands of the club's row
(halfwords +6 and +0x12, seven bands each) and the kind of the club's next dated league entry: 25 jump tables of 7
cases, 336 sentences, kept as tables read from the listing. Its fallbacks for a band outside 0..6 are dead (the
helpers return 0..6). It is a leftover-stack reader that no capture reaches: for a club 0x100c7d00 flags whose
nation record (+0x2c) is null it formats sp+0x3c as a name without having written it (0x1019c28c on).

The weekly pass 0x101a3350 (`manager_manager_weekly.cpp` and its part files; called by the daily update every seventh
day) walks every club: the board row's value moves toward its target, and events, hires and the club's standing with
its manager are decided by up to 78 draws per club. Two frame bytes (sp+0xdc, sp+0xdd) are cleared once by the
prologue and set by a club's hire, so they stop the same rules for every later club of that pass. r13 (a level byte)
is read at 0x101a3f1c, 0x101a4508 and 0x101a4658 on paths where the club has not set it: it holds what an earlier club
left, or the caller's r13 for the first club to get there, and the rewrite keeps the register for that. The
floating-point stages of six inlined draws (0x101a38c0; 0x101a6a64 and four copies) are dead: their bounds are a
squared byte over 100 plus one, or a sign-extended byte; the rewrite throws if one were ever needed. It was written
in parts by handing the rest of the call to the original: a hook may set the PC to any instruction address of its
routine and run the dispatcher until the routine returns, so a long routine can be checked section by section.

The board's reply to a manager's request (`manager_manager_standing.cpp`: 0x101bb9f0 decides one of six request kinds
from the club's row, the manager's and owner's records, the employer's funds, a date window and draws; 0x101bd320
files the 0x1786 message; 0x101bd460 writes its text) is the module's first native-first block: native locals, one
`guest::Frame` per entry, registers only as the arguments of calls out of the block. The floating-point stage of its
four inlined byte draws is dead (each bound is an `extsb` byte). 0x100f5770 returns a second word in r4, which
0x101bb9f0 adds. The text routine is the one place that sets non-argument registers: 0x1074bab0 appends a 0xd0-byte
row built in its own frame, and the bytes after the name's terminator are the save area (`stmw r22`) of the
previous callee, so r22-r31 hold the original's values at the formatter calls before it.

The assessment of a manager offered to a club (0x101c3220 with its verdict 0x101c1e20 and its report 0x101c3860;
`manager_manager_assess*.cpp`) and the module's tail (`manager_manager_tail*.cpp`: the application a club weighs
0x101c0230, the queue of due moves 0x101c0fc0 and their news and texts) are native-first too. 0x101c0230 takes four of
its arguments from the caller's parameter area (sp+0x3b, +0x3e, +0x43, +0x44) and re-reads the person's word +0x1c after
a call at 0x101c0910 rather than reusing the earlier load; the rewrite does the same. Under the native contract this
module's checker leaves the dead stack below the entry free for every routine except the five whose leftovers the stack
audit lists readers for (0x10196390, 0x1019d320, 0x101a2eb0, 0x101ad080, 0x101b1620), and does not compare
non-volatile registers at calls made from native code, whichever routine is being checked.

The vacancy block (`manager_manager_vacancy*.cpp`: the board's answer to a manager's request 0x101bdf80, the news it
files and their texts, and the vacancy event 0x101b9130) is native-first. The original reads the interest table's base
(TOC -0x7ab4) and the person's index again after every call rather than keeping them; the rewrite does the same. Three
of its calls leave argument registers the original sets but the callee never reads (a format with fewer `%s` than
registers, an array constructor that reads r3-r7): `manager_manager_trace_accepted.inc` lists them with the reason.
The module's message helpers (`start_message`, `set_field`, `sext8`) live in `manager_manager_detail.hpp`.

## The sort family: qsort4x.c, qsort8x.c, the byte sort and their entry

Every sort in the game goes through 0x10768300, which picks one of three heapsorts by the row width: pairs of words
(0x10768660, qsort8x.c) for a multiple of 8, words (0x107684c0, qsort4x.c) for a multiple of 4, bytes (0x10768340)
otherwise. Nothing else calls the three, and no data word points at any of them (`tools/test_qsort4x_audit.py`). All
four are C++ in `native/qsort4x.cpp`: one heapsort over the three swaps.

What its callers observe besides the rows, and so what the rewrite keeps:

- The order of comparator calls and swaps. Comparators are game routines with frames of their own, and the heapsort is
  not stable, so ties end where the original's do only with the same sequence.
- The prologue image (saved r21-r31, LR, back chain). The stack contract lists fifteen readers of what 0x107684c0
  leaves, five of which change results when they read zero instead (0x101de39c in the match engine, 0x1029fa38,
  0x1043355c, 0x1076b760, the import bridge 0x10bf7bbc). Sorting changes a later leftover-stack read, so a sort cannot
  be replaced by a host sort even where the rows would come out the same.
- r21-r31 at each comparator call (a comparator's prologue saves them below that image), and r3-r8, cr0 and the carry
  as the last swap leaves them. The fixture checker compares these with `strict_volatile`, at every comparator call.

The word and pair sorts assert that the width is a multiple of their unit (0x107755c0, line 120) and then sort anyway,
moving whole units and leaving the remainder of each row where it was. The entry never sends them such a width.

The fused 8-byte sort (`tools/native_sort8.py`) stays the first hook at 0x10768660 and answers for the three comparators
it inlines; the entry's C++ calls the pair sort through its page so that it still does. A typed C++ comparator called
directly from the heapsort would replace that fusion; nothing does that yet.

## DISCIPLINE.CPP: bans, suspensions and their history

37 entries / 11,122 instructions at 0x100de410..0x100e92d0 (`tools/discipline_audit.py`). The unit has no source
name in its own last routines: its TOC slots are -0x5610..-0x5600, and the conversion constants at -0x5610 close its
literal data, which places 0x100e8780, 0x100e8d10 and the comparator 0x100e9290 here; 0x100e92d0 on belongs to the
unnamed scout and transfer-rumour unit. All 37 entries are native-first C++ (`native/discipline_*`).

The manager (TOC -0x7aa4) holds a *pointer to* the array of 42 ruling bodies at +0, a 0x2c-byte row per person at
+0x14 (five 8-byte slots of a body index and a word, the history position at +0x20/+0x24, a pending count at +0x28),
the history file at +0x18 and the pending list at +0x1c. 0x100e7040, the slot lookup, takes most of the module's day
time.

The one contract that reaches records: the ruling bodies build a 0x18-byte ban struct in their frames without writing
bytes +0xd..+0xf and part of +0x14, and 0x100dfb30 copies it into a ban record that goes into discipline.dat (the
BlockMoveData reader 0x10bf7bbc). Those bytes are the register save areas of earlier generated callees at that depth
(0x105bc2b0, bsearch 0x10775600, 0x10717890, the history file), so the routines on that path (0x100dfb30, 0x100e0090,
0x100e0250, 0x100e0410, 0x100e05c0, 0x100e0700, 0x100e8780) keep the original's r13-r31 at every call; the file
routines 0x100de760 and 0x100deda0 do too, as the strict replay compares them at the file imports. The case checker
cannot see this (the native contract leaves the dead stack free); the live check and the paired replays of
season-day and same-save-continue do, and found it.

0x100e8d10 is this unit's copy of the range sampler; every bound it gets is a sum of two signed bytes, so its wide
arm and self-call are dead (as in NEWS.CPP). 0x100df500 copies frame bytes it never writes into pending items: the
C++ keeps the original's frame and calls so they are the same leftovers. The temp-folder name 0x100de410 interleaves
from 17- and 16-byte frame copies; with the game's two constant strings (16 and 15 characters) it never reads past them.

The ban-news renderers (NEWS types 2002 and 2003: 0x100e0b70, 0x100e59b0) choose among about 160 formats; the C++
keeps them as tables of (pool offset, return address) per decision. Their link rows (0x1074bab0) copy the renderer's
own stack, so they and 0x100e7490, which they call, keep the original's r13-r31 and the formatter's spilled r5-r10 at
every call. 0x100e59b0 reads its caller's r28 as a format argument for a ban by date (the call that would set it,
0x100e67f8, is on a dead path); its only caller, NEWS 0x102be040, always holds the TOC cell address -0x7be8 there
(0x102be070), and NEWS is C++, so the rewrite takes that constant. 0x100e0b70 reads frame words it never writes when a
message has no fixture or an unknown kind; the C++ reads the same slots, so those texts depend on leftovers as in the
original.

## DATE.CPP: dates, their arithmetic and their texts

32 entries / 3,320 instructions at 0x107156b0..0x10718b38 (`tools/date_audit.py`): the unit's 31 routines and
0x10718af0, an 18-instruction leaf between it and DISPLAY.CPP that has no data of its own. The bounds are the unit's
data: its pool (TOC -0x3fa0, "DATE.CPP"), month starts, static dates, text buffers and jump tables (-0x3fe8..-0x3fa0)
and the guard bytes of the static dates (r2-0x35c0..-0x35bb) are used by these routines and by nothing else. All are
C++ in `native/date.cpp` (building, moving and measuring dates) and `native/date_text.cpp` (nine formatters and the
names they use); they replace the register-level owners of the earlier process (`date_subsystem_adapter`,
`calendar_adapter`), which nothing in the game calls any more for these entries.

A date is a day of the year from 0, the year, and a leap byte. Nearly every unit calls these routines (2,472 call
sites), and what they leave is read afterwards: 104 reader sites of leftover stack in the captures, among them the
import bridge copying it into game memory, and callers that go on to a varargs call with r4-r10 as a date routine
left them. So this unit is not written to the native contract: every routine leaves its prologue image, the
registers at each of its calls and every register at its return as the original does, and `cm-date-checks` compares
with `strict_volatile` (r0, r11, r12, XER and CTR too) over all of memory. The logic is plain C++; the registers are
set where a call or the return exposes them.

- Each routine that moves or measures a date works on a function-local static date, built as 1972 day 40 on first
  use and flagged by a guard byte. The static dates are game memory and are written as the original writes them.
- Adding a negative count of days calls the subtraction with the magnitude and the other way round; -32768 is its
  own magnitude, so the original recurses until the stack ends. Nothing passes it.
- Authorized correction (2026-11-15): signed day differences now use the actual year crossed, making them
  Gregorian and antisymmetric. The old ascending loop classified the preceding year instead.
- The interval routine (0x10717dd0) keeps the ordinal-year/day-remainder representation used by age records
  and contracts, but now borrows the year before the later date when its day remainder is negative. The
  original tested the later year, producing zero or two days at some one-day year boundaries. Existing leap
  calls remain to preserve unrelated registers/stack. The checker amends only the authorized result fields
  of the raw reference and separately verifies calendar boundaries and rejection of the old arithmetic.
  This interval is not a total elapsed-day count: callers needing that use `calendar::absolute_days`, as
  national call-up injury eligibility now does. The branch at 0x10717f18..0x10717ffc still needs each date
  to be later than the other from the same values: dead, and the C++ throws there.
- A calendar date with a day outside 1..31 or a month outside 0..11 is reported by the assert macro and then built
  with day 1 or month 0.
- The formatters find the month by a search that leaves the month register as the caller had it when the day is
  before every month start; the C++ leaves it alone as well, so such a date prints from the caller's register.
- Weekday 0 is Monday. The name routines switch through tables of code addresses in game memory; the C++ reads the
  table and finds the case by its address (`tools/test_date_audit.py` pins the tables).
- The 200-day cutoff cache that the shortlist scan had for 0x10717b20 went with the old owner: the scan calls the
  C++ routine directly (`date::call`).

## GAME_CONFIG.CPP: the preferences record and its file

12 entries / 1,444 instructions at 0x1013f030..0x10140724 (`tools/game_config_audit.py`), in `native/game_config.cpp`
(the record, its options and setters) and `native/game_config_file.cpp` (building the game's record from the file and
the command line; applying a record). They replace the register-level owner of the earlier process (`preferences_*`),
which no page calls any more; its sources stay compiled because the old SETUP.CPP screen checks use its reference. The
bounds: the unit's string pool (TOC -0x54e8, "GAME_CONFIG.CPP") is loaded by these routines only; the static
initializer 0x1013f000 before it closes GAME.CPP and HALL_OF_FAME.CPP starts at 0x10140730; the last two routines
(0x101406e0, 0x101406f0) load no pool and belong by the record fields they read and write. 40 outside call sites, all
calls of an entry; no pointer to the unit.

No capture that replays reaches the unit (it runs at start-up, from the options screens and from the menu bar), so
`tools/stack_contract.py` has no reader for it and the paired replays say nothing about it. The routines are compared
under the native contract, and still keep the original's frame and its non-volatile registers at every call: the
routines below reach file and display imports with them, and that costs a line per routine. `cm-game-config-checks`
scripts every routine outside the unit and traces each call with the arguments it reads and r13-r31.

- The record is 0x150 bytes: a link to the record a working copy was made from, the options word, language and
  currency bytes, a number of days (7..365, 30 at first), the save folder, a 0x1f-byte text this unit only copies,
  a "stored" byte, and the font's identifier and name. The identifier field and its two routines were called "video
  mode" by the earlier process; the callee that names it (0x10750b60) and the `-defaultfont` switch say font.
- An option is one bit, and selecting it clears the rest of its group (twelve groups). The original is a tree of
  equality tests, so a value that is not exactly one listed bit is reported by the assert macro.
- The file "CM3 Config" starts with the word 0x1553117c. Reading stops at the first short item and what was read by
  then stays; a file that is short or has another first word is deleted. The stored save folder is read into the
  frame and dropped: the record keeps the "games" folder of this run. A stored font identifier whose name is no
  longer the stored name goes back to the default (-2).
- A stored language that is not installed is skipped; a stored currency outside 0..24 is skipped.
- Applying a working copy writes its source and applies that. Applying the game's own record opens the display
  again when the font or the mode changed, and passes the option's bit itself (8 or 0) as the full-screen flag; when
  that fails it selects the other mode and tries it, and when that fails too it reports and leaves the program.
- Three sites start the sound player. The one in the constructor does not look at the cell first, and only the one
  in the option routine plays the intro. After a player that did not start, "sound off" is selected, which frees
  it; the test for a player still in the cell that follows can never succeed (the only outcome the checker's
  coverage does not reach).
- The exit routine (0x1076f7d0) is called and, were it to return, the code after it runs; the checker's scripted
  exit returns, so those continuations are compared too.

## TRANSFER_OFFER.CPP: the offer record, its terms and its value

17 entries / 5,545 instructions at 0x10411e60..0x10417590 (`tools/transfer_offer_unit_audit.py`). The files are named
`transfer_offer_unit*` because `native/transfer_offer_*` is an older owner of TRANSFER_MANAGER.CPP routines. The bounds:
the unit's three pools (TOC -0x4b88 strings starting "TRANSFER_OFFER.CPP", -0x4b8c doubles, -0x4b90 floats) are loaded
by these routines only, from 0x10411fa0 to 0x104172f0; the three routines before the first load build and destroy the
record the others work on; TRANSFER_MANAGER.CPP's audit ends where the unit starts and TRANSFER_SCREENS.CPP's starts
where it ends. 118 outside call sites, all calls of an entry, and one pointer: a transition vector of the destructor
0x10411f10. They replace the instruction transcriptions of the earlier process (`transfer_record`,
`transfer_valuation`, `transfer_synthesis`), which no page calls any more.

`native/transfer_offer_unit_record*.cpp` has the record (building, destroying, its form in a saved game, starting an
offer, its total), `_value*.cpp` what an offer is worth and how a fee is reshaped, and `_build*.cpp` the routine that
builds a club's offer for a player (0x104126d0, 3,551 instructions).

**Contract.** Native inside, compared under the native contract, with one rule the captures forced: every routine
keeps the original's frame and holds the original's non-volatile registers (r13-r31, f14-f31) at every call it makes.
Its callees save those registers in their frames, and code that runs later reads those frames as stale stack: with
r18-r28 left native in 0x104126d0, or r30/r31 in the destructor, every checked call matched and the replays of
`season-day` and `same-save-continue` still ended in another state. The routine checker cannot see this (the bytes are
dead stack at the routine's return), so `cm-transfer-offer-unit-checks` compares r13-r31 and f14-f31 at each call in
its call trace, and the paired replays are the end-to-end check. 0x104172f0 also writes its int-to-double words to
its frame as the original does (a reader in TRANSFER_MANAGER, 0x103f88c4).

- The record is 0x54 bytes, written to a saved game whole, followed by a count and the rows of its two lists. The
  reader's club rows are indices into the manager's offer slots: an offer whose row names an earlier offer shares
  that offer's club list, and the list it had built is reset and destroyed.
- 0x104126d0 reads two bytes of the record it is given before writing them (+0x38 and +0x3c keep their low
  nibbles). The record is usually on the caller's stack and not initialised; the C++ reads the same bytes.
- On the human buyer's path the 8-byte exchange row appended to the new offer's list includes frame byte sp+0x53,
  which nothing writes after entry: stale stack ends up in the list and, through the writer, in the row's padding
  in a saved game. The C++ leaves the byte alone, but what earlier frames left there can differ from the original
  (this unit's own conversion temporaries are native locals now). No capture reaches the path; open.
- 0x10415e50 (a draw below a limit) contains a call of itself that cannot run: the inner limit is the limit times
  ten over 0xffff, at most 32768 in magnitude, so the inner level always takes the small branch. The C++ has no
  recursion and throws at the dead branch.
- 0x104172f0 compares twice the fee with a 64-bit result of 0x100f5770 (r3:r4), signed, through a carry chain.
  When the buyer's contract is above 5000 and the person's profile has been cleared in between, the original reads
  through the null profile; nothing guards it and the C++ does not either.
- Two of 0x104170f0's and the reader's paths free or reset a list through routines the pages run in place
  (0x10738c20, 0x10738ca0); the checker cannot answer those, so they run for real in the cases that reach them.

## INJURY.CPP: injuries, their daily course, news and file form

47 entries / 12,259 instructions at 0x101840e0..0x10190180 (`tools/injury_unit_audit.py`). The files are named
`injury_unit*` because `native/injury_*` and `actor_attributes` are the instruction transcriptions of the earlier
process, which no page calls for these entries any more (`actor_attributes` keeps six entries of another unit). The
bounds: the unit's private TOC slots, -0x5478..-0x5430 (pools, eight jump tables, three comparator vectors, three
statics) and -0x64d4/-0x64d0 (the injury tables), which no other code loads; the string pool holds "INJURY.CPP". 302
outside call sites, all calls of an entry, and three pointers: the transition vectors of the sort comparators
0x1018ff10, 0x1018ffe0 and 0x101900b0.

`native/injury_unit_status*.cpp` has the questions asked about a person (stage, fitness, sharpness, the current
injury) and the pending update 0x10187a40 every one of them runs first; `_onset*.cpp` how a person gets an injury
(0x10188cc0) and the daily pass that hands them out; `_course*.cpp` recovery, relapse and the club's queue of
injury events; `_file.cpp` the manager's construction, its form in a saved game and a new game's starting
injuries; `_text.cpp` names, descriptions and news.

**Contract.** TRANSFER_OFFER.CPP's: native inside, the original's frame, and r13-r31/f14-f31 as the original holds
them at every call (`cm-injury-unit-checks` compares them in its call trace). On top of that, the frame stores the
stack contract lists for the unit (`tools/stack_contract.py module INJURY.CPP`), and for the eight routines the
match engine reaches (0x10186cd0, 0x10187650, 0x10187680, 0x101878d0, 0x10187a40, 0x1018c180, 0x1018c270,
0x1018c340) every store the original makes into its frame, since the match replays have no stack audit. The three
comparators are frameless and leave copies of the row's date below the stack pointer; the C++ leaves them too.
Calls between routines of the unit are direct C++ calls, so the 47 entries are routed together: a draft among
them would still run where a routed one calls it.

- A date is five bytes: a signed day halfword, an unsigned year halfword and a byte. Words read from a date in a
  frame carry three stale bytes (0x101885b0 in 0x10188490, 0x1018be38 in 0x1018bdd0); they are read from the real
  frame, and only reach registers that callees save.
- Every division and remainder in the unit is signed (the listings hold no `divwu` or `mulhwu`). A draw is the
  sample table's halfword plus the base cell's word, taken signed; three unsigned remainders were found by mutated
  runs and the review.
- The "draw below a limit" helper 0x101864a0 has four inlined levels and a call of itself for limits past
  0xffff. The limit of the next level is the limit times ten over 0xffff with the product wrapping in a word, at
  most 32768 in magnitude, so no more than two levels run; the C++ throws at the third. The same expansion is
  inlined in 0x10187a40, 0x10186a20, 0x10188930, 0x10188cc0 and 0x1018dcb0 with limits that fit a halfword, and
  those throw at the scaled branch. Only the daily pass reaches the scaled draw: its limit is the people count.
- 0x10187a40 has first-use guards (statics at TOC -0x5448..-0x5450): its first call builds the window dates and
  clears the cached club. A snapshot taken before any call runs that code on every checker case, so cases that set
  the windows also set the guards.
- 0x10186cd0 appends an 8-byte row to the manager's notes list from its frame (+0x38) after writing six of the
  bytes; the last two are stale stack. The save writer 0x10184e60 writes all eight bytes of each row, so stale
  stack reaches a saved game as row padding in the original too. The C++ copies the real frame bytes; what earlier
  frames left there can differ from the original now that callers are native. Open, as TRANSFER_OFFER.CPP's
  exchange row.
- 0x10186fc0 passes its caller's r18 to 0x10188930 as the severity when a note's strength is 0x32 or less (it
  never sets it on that path). One caller holds 0x10000 there (severity 0); the other passes on its own caller's.
  The C++ reads the register.
- The save writer writes the capacity words of the event and report tables (+0x18, +0x24) where the reader reads
  counts (+0x14, +0x20). A saved game without injury records makes the reader's zero-size allocation return null,
  which it reports as a failure, and the constructor then gives starting injuries as for a new game.
- When an allocation of the notes list fails (reader, 0x10186cd0), the original goes on and reads through the
  null list; the C++ does the same.
- 0x1018fdf0 with no person reports, calls exit, and would report and exit again; nothing guards the read through
  null that follows.

## FINANCE.CPP: club accounts, budgets, the daily and seasonal passes, finance news

58 entries / 18,706 instructions at 0x100f1000..0x10103600 (`tools/finance_audit.py`). The bounds: the unit's private
TOC slots, -0x55d0..-0x5598 (string, float and double pools, two scrambled names, eight data tables, two jump
tables), which no other code loads; the string pool starts "FINANCE.CPP"; FILE_SCREENS.CPP's audit ends where the
unit starts. 332 outside call sites, all calls of an entry, and two pointers: the transition vectors of 0x100f1000
and 0x100f1010, the constructor and destructor of an account as an array element, which only 0x100f84e0 and
0x100f8720 load.

The finance manager (the word in the cell at TOC -0x7ab8) holds one 0x168-byte account per club, indexed by the
club's index: a signed 64-bit balance at +0, the club's index at +8, budget figures at +0xc..+0x14, a ledger of
income and expense lines at +0x70..+0xbc kept twice (the second copy 0xa0 further on), and state bytes at
+0x160..+0x166. `native/finance_ledger*` has the account lookup 0x10103510, the wealth band 0x100f55b0 (-2..2 by
the balance against the club's reputation; many proofs in the unit rest on that range) and the booking routines;
`_setup*` the manager's construction, a new game's opening balances, the accounts' form in a saved game and the
draw below a limit 0x100f2100; `_value*` what a club can pay (0x100f2680, 0x100f4790, 0x100f4f80); `_bid*` the
bid limit 0x100f5770 and two bookings; `_matchday*` takings, the split of a fixture's income and ground
expansion; `_daily*` the daily pass 0x100fa6e0 and the season roll; `_season*` the club review and donations;
`_board*` the board's review with gifts, cover and bail-outs; `_news*` the nine news items and their texts.

**Contract.** INJURY.CPP's: native inside, the original's frame, r13-r31/f14-f31 as the original holds them at
every call (`cm-finance-checks` compares them in its call trace), and the frame stores the stack contract lists
(`tools/stack_contract.py module FINANCE.CPP`: 0x100f2680, 0x100f4790, 0x100f55b0, 0x100f5770 and 0x10103510
leave bytes later readers read). No routine is called by the match engine. Calls between blocks go through the
adapter (`invoke_unit`), calls inside a block are direct, so the 58 entries are routed together.

- 0x100f5770 returns 64 bits in r3:r4; every caller reads r4, some also r3. Its line's kind is `R3 | R4`.
- 0x100fd1a0 ends its pass at the first inactive club (the `bne` after 0x100c7d00 goes to the epilogue): the clubs
  after it are not refreshed. The decompiled C reads as a skip; only the call trace showed the difference, since
  the refresh was answered in the cases.
- A case's answer for a routine of the unit fires only while that routine is not under check. A run of every
  entry therefore takes other paths than a block's own `--only` run: equivalence is the run of all entries plus
  the nine block runs, and coverage is measured over both (`.local/rewrites/finance/covrun.sh`). A routine the
  original calls inside its own generated page is not dispatched and cannot be answered or poked at all.
- The daily pass has a fixed day on which two clubs move to grounds named in the unit's pool ("Falmer" and another):
  their attendance is reset by reputation tier and the fixture list's entries are relinked (0x1048b180 and the list
  routines). The two walks are one template with different registers and return addresses; each needs its own case.
- 0x100fa7ac reads eight bytes of its frame that this call did not write (after 0x107156f0, which returns its own
  buffer); the original copies the second word and never uses it.
- 0x100f84e0 merges two scrambled names for the CD's folder check. The second has 16 characters and 17 bytes are
  copied to the frame, so the merge loop stops on whatever the frame holds at +0x81 onward. The merged name's
  length byte is fixed, so the name asked for does not change; the count of merged bytes and r22 at the following
  calls do. The C++ reads the real frame bytes, as the generated body did. No capture reaches this routine.
- 0x101030c0 can read address 0x10 two ways: through a null ground row (0x1010311c; the test before it only skips
  the read for a non-null row) and through a row whose word at +0x10 is null (0x10103134). The original machine
  could read page zero; here the game stopped. The C++ posts no item then and says so once in the run log
  ("Stadium news for a club without a named ground"); the generated body is not guarded. Neither happens with the
  game's own data: both callers store the row just before the call, and the two rows they use have the word.
  `cm-finance-checks --production` checks both outcomes (the original faults at 0x10, the C++ returns).
- 0x10102660 leaves the flag byte in r5 when the stadium index is -1 and the flag is set, so the seating text
  prints the flag where the capacity would go.
- Limits of the inline draws below a limit are a signed byte, a halfword or a small product at every site outside
  0x100f2100, so their branch for limits past 0xffff throws; 0x100f2100 itself runs two levels (the third's range
  is at most 32768). The draw and its division helpers are written once per block (for the native-first pass: one
  shared draw with a per-site register callback).
- The 250,000 cap at the end of 0x100f4790 cannot act (every kind's range ends at 35,000 or below) and throws.

## HUMAN_MANAGER.CPP: the human managers' records, tactic slots, lists and file

62 routines, 0x10145af0..0x1014d560 (`tools/human_manager_audit.py`, `native/human_manager*.cpp`,
`cm-human-manager-checks`). The table at TOC -0x7a54 holds `{records, count}`: one 0xa65c-byte record per human,
indexed by `staff id - staff count + 16` (humans hold the last sixteen staff ids). A null staff argument means the
human whose turn it is (game record at TOC -0x7bb0: halfword +0x2c58, 0x2c0-byte rows, staff word at +4).

- 0x1014c2e0 (whether a human runs a club) is the hottest routine of the module (418 call sites, millions of calls a
  day). Its saved r24-r31 and those of the routines it calls (its own option readers and DATABASE.CPP's range
  tests) are read later as stale stack, so it holds the prologue's values in r24-r31 throughout and calls those
  routines directly at the original stack pointer. 0x1014c490 asks the same of the human whose turn it is and,
  unlike 0x1014c2e0, never checks that the club's manager has a human's id.
- 0x1014bf80 (is a key in a human's list) leaves r4-r8 as the original does on every path: callers read r5-r10
  after it, and the original holds r8 = record + 0x10000 across the binary search, which leaves r8 alone. Without
  it the same-save-continue capture ended with a different stack byte. The native contract's checker does not
  compare r4-r10, so only a pair finds this; `tools/register_contract.py --after` lists the callers that read them.
- 0x1014b690 (remove a key from every list) returns from inside its loop at the first record with no list, and
  after the loop sorts the list of whatever record r29 last held: when the loop never runs, the caller's r29.
- The six squad routines (0x1014c890..0x1014d300) read the tactic-slot byte unsigned and use slot 0 for slot 1.
- 0x1014a300/0x1014b0a0 read and write the records in the saved game (`human_manager.dat` inside the save): the
  saved game's version (TOC -0x7b74, through its absolute value) selects older layouts below 0x3e021 (tactic slots
  as 0x1e5c-byte images), 0x3e005 (the list as sixteen words) and 0x3e001 (the humans' staff byte +0x65 set to 20).
  They hold r13-r31 as the original at each of their ~40 file calls per human.
- A case's answer for a routine of the unit fires only while that routine is not under check, so the run of all
  entries and the six block runs take different paths (as in FINANCE.CPP); coverage is measured over both
  (`.local/rewrites/human_manager/covrun.sh`). The realloc failure of 0x1014b810 is reached by answering the heap's
  own resize 0x10774f40, which the realloc facade dispatches; the facade itself runs in place in the original.

## DISPLAY.CPP: the display table, its input, drawing and network form

32 routines, 0x10718b40..0x10725a00 (`tools/display_audit.py`, `native/display_*.cpp`, `cm-display-checks`). The main
object (TOC -0x7bb0) owns two display tables (+0x2c68, +0x12d6b0) and, per human row of 0x2c0 bytes, the one in use
(+0x24). A table addresses its fields as `addis rX,table,0x13` plus a negative displacement (`display::table` names them).

- Takes three entries over from older builder owners: the table's reset and destructor 0x10718b40 / 0x10718ba0 from
  aggregate_object and the unit's static initializer 0x107259a0 from engine_initialization (`DISPLAY_TAKEOVERS` in the
  generator, as SCRMAN took its constructor). Page 0x1071a is a turbo page: its hooks are the counter's semantic
  boundary, and the audit accepts either form.
- The adaptive interface's pins in 0x10718f60, 0x10718fc0 and 0x1071afc0 are carried by the C++: `screen_right` /
  `screen_bottom` stand where the original has 799 / 599, and 0x10718fc0 calls `draw_adaptive_background` at 0x10719120
  with r24 holding the table. The checker compares in 800x600; the adaptive checks exercise the rest.
- Search Filters (`0x10329fd0`) opt into New Screens using `form_layout.cpp` (Yoga v3.2.2's public C API) and
  `search_filter_layout.cpp`. Yoga only lays out the shell; original AutoGrid refresh publishes both drawing and
  input bounds. Its seven overlay grids share the field rectangle, while pitch decorations on the footer grid
  retain explicit overrides. Text, colors, fonts, popup ownership and `0x103308c0`'s draft/apply/cancel handlers
  remain original. Only local filter histories with a valid engine, draft and General/Attributes tab qualify.
  Yoga is isolated in static targets; upstream headers/build flags do not propagate to the simulation. Its MIT
  notice ships in the app. The focused checker compares original/adaptive transactions in disposable captures.
- Transfer proposals (`0x10417b60`) use the same Yoga layer for bounded section/column tracks, measured
  introduction rows, exchange lists and compact footers. `transfer_proposal_layout.cpp` preserves the original
  control order, history indices, rich-text links and amount-menu ownership; explicit column bounds survive
  the original in-place value refresh. New Screens off and 800x600 remain untouched. No proposal builder,
  command, negotiation or draft calculation is replaced. The focused capture check covers new/existing,
  pending and incoming footers, five-player exchanges, original edits/menus, discard and confirmation entry,
  and unchanged guest ABI, draft/records/history and RNG. Negotiated reset and competing-bid menus retain
  their original handlers; their specific flows are source-reviewed rather than exercised by this fixture.
- Playing-role contract negotiation (`0x10096c60`) reuses Yoga form tracks for Basic, Bonuses and Clauses.
  `contract_negotiation_layout.cpp` keeps original controls, indices, requested/draft terms and measured
  rich-text links; the two clause grids share an inner row rectangle despite their different insets.
  Field tracks reserve AutoGrid's scrollbar gutter from populated-row metadata, including before its
  first refresh. Visible-row capacity and scroll positions stay original. Local playing roles 11–13/15
  and tabs 1–3 opt in; staff forms retain their existing layout, while confirmations use the recipe below. New Screens
  off and 800x600 are unchanged. Original builders and negotiation rules remain responsible for terms,
  restrictions and actions; no new localization strings or generator hooks are needed.
- Loan terms (0x1042ca40) and immediate proposal/contract/loan-trial confirmations
  (0x1041eb10/0x1009dc00/0x1042efa0) reuse `negotiation_dialog_layout.cpp` behind New Screens.
  Bounded Yoga tracks align loan fields and restore buttons; a shared confirmation recipe preserves
  original text wrapping and measured hyperlink offsets in a centered column. Standalone titles and
  the contract's rich title grid retain their original styles. Scroll positions and disabled controls
  remain original; narrow overflowing confirmations retain legacy geometry when the original text
  column cannot fit beside the scrollbar. Gates distinguish person pointers, offer links (including
  the proposal's history-2 fallback), and contract roles. The message
  screens (offer decision from the news 0x10421fb0, transfer completion 0x10423270, transfer-list
  confirmation 0x1042a810, offer restriction 0x10420b50 and feedback 0x10421070, staff release
  0x1009e8f0) use their own dialog layout, `message_screen_layout.cpp`, described below. Existing-capture checks compare eight original/adaptive states, six window sizes,
  scrolling, loan edits/restoration, confirmation entry and read-only dismissal; final submission is
  hit-tested without dispatching a persistent transaction. No original builder or handler changed.
- Message screens (the six builders above) use `message_screen_layout.cpp`: their builders put a short
  message in the middle row of nineteen and size the buttons by a column specification, so the confirmation
  recipe gave a tall empty panel, and a four-button footer (completion: Back, No, Postpone, Yes) kept the
  classic geometry. The layout makes a dialog: a panel up to 800 wide centered under the title bars, as
  tall as the message rows plus one row of padding each side (at least five rows), the message rows
  compacted into it (the controls' rows keep their order and spacing; the grid's first/last row and row
  count follow), the text and its links shifted together to the new center, and the buttons (a grid
  footer of one to six columns or one standalone control) as one row of equal 190px cells under the panel,
  right-aligned to it, the block in the upper third of the content area. It declines on a scrollbar, a
  missing or duplicated grid, or a message too tall for the canvas. Gate: a career is open.
  `--message-screens` opens all six through their original openers (the loan listing too) on real records
  and checks, besides the usual ABI/records/RNG/picker/redraw/Off checks, the dialog geometry above.
  Unexercised: feedback reasons other than 1, restriction reasons other than 5, longer messages.
- Player transfer status (0x10428ab0) uses `transfer_status_layout.cpp`: the seven-column price grid and
  the two-column choices grid share one rectangle and one row table, so both get the same bounded panel
  (up to 960 wide, row height up to 28px). Only the widest label column grows; the minus, amount, plus
  and automatic cells keep their width, the intro paragraph and its links move together, the footer
  buttons are equal 190px cells right-aligned to the panel, and the price menu still opens at its
  control. Title and subtitle already follow the shell. Previous offers (0x10422600) uses
  `previous_offers_layout.cpp`: the same panel for its message grid, the subtitle grid and the single
  standalone footer button, with each centered text and its links moved by one shift. Gates: a career,
  a person in range (status) or an offer record with person and bidder in range. `--transfer-status`
  (eight price/choice states, price menu, edits, redraw) and `--previous-offers` (six message states)
  use the original openers. Unexercised: non-player persons, the Set action (hit-tested only), a
  previous offer whose person has no contract.
- Board Confidence (0x101c8fb0, MANAGER_SCREENS.CPP, opener 0x101c8ed0 from the Manager menu), Manager Statistics
  (0x101c4460, opener 0x101c42f0) and the FIFA World Rankings (0x100745f0, COMP_SCREENS.CPP, opener 0x10074550 from
  the Competitions menu) share `paged_screen_layout.hpp` and have a layout file each: `board_confidence_layout.cpp`,
  `manager_statistics_layout.cpp`, `world_rankings_layout.cpp`. All three build one content panel at the classic
  rectangle (a 16-row message grid at 110,155-780,540; a table at 110,170-780,535), optional standalone controls
  above it, and the Back/Next footer from the footer helper 0x1074b0a0, which already follows the canvas (100,H-45 to
  W-10,H-10) as one wide cell and one narrow one. The panel becomes up to 960 wide, centered in the content area, with
  row height up to 28px. Only the widest column of a table (managers, nations) takes the extra width: rank, club,
  status and points keep theirs, each cell with its original insets in its track; a one-column "no statistics"
  table keeps its measured message, shifted to the panel's center. Board lines keep their wrapping and move by
  the panel's left edge (their right bound by its right edge). The standalone controls above the panel keep their
  side: the left half (the board's two request links, the chosen-nation control of Manager Statistics) moves with the
  panel's left edge, the right half (the pager "." "*" "Pg n/m" "+" "/") with its right edge, text bounds with
  them; Manager Statistics' subtitle, which its builder lays at the classic 100,80-790,125, follows the shell like
  the others. The footer is one 340px block right-aligned to the panel, two equal cells (AutoGrid's equal-width
  flags at +0xbb2). The nation menu keeps the original popup, placed when it opens. The layout declines (classic
  geometry) on a missing, duplicated or scrolling grid; no page of these three builds a scrollbar, and the
  fixtures show the layout ran for every state the gates admit. Gates: a career for the two tables; for
  Board Confidence also a history club inside the database's club array. `--board-confidence` (the managed
  club and two others whose messages wrap differently), `--manager-statistics` (first page, a nation without
  statistics, two later pages) and `--world-rankings` (first page, a confederation with no nations, a middle and
  the last page) open them through their original openers on `tactics-views/initial.snapshot` and compare, at six
  canvases, panel centering/height, footer equality and alignment, toolbar sides, column widths (one column
  grows), text bounds, picker hits, in-place redraw, Off, and the original Back result and RNG, and each runs
  three mutants through the same checker (footer drift, classic panel, classic toolbar control). Unexercised:
  the other MANAGER_SCREENS builders and screens reached with other nations' histories in play.
- UEFA Coefficients (0x10074d80, COMP_SCREENS.CPP, opener 0x10074d00 from the Competitions menu), Hall of Fame
  (0x1033fcf0, opener 0x1033fc60 from the Game menu), Manager Status (0x10338b40, opener 0x10338a90, dispatcher
  action 0x3fc) and Job Information (0x101c74e0, opener 0x101c7370 from the Manager menu) are one more table family on `paged_screen_layout.hpp`: its `layout_table_screen` takes the table's
  classic rectangle and column count (`uefa_coefficients_layout.cpp` 110,145-780,535 with six columns;
  `hall_of_fame_layout.cpp` 110,170-780,535 with five; `manager_status_layout.cpp` 110,145-780,535 with three;
  `job_information_layout.cpp` 110,170-780,535 with five, or one column holding a centered message when there are no
  jobs) and does the same pass as the rankings: a centered panel up to 960 wide with rows up to 28px, only the widest column
  (nation, manager) taking the extra width, standalone controls above the table kept on their side (the Hall of Fame's
  year-range control on the left edge and its nation control on the right) and the compact Back/Next footer. The UEFA
  table always has a scrollbar (52 nations, 17 rows with the header as the first scrolled row; the scroll offset is
  history slot 0). The original grid keeps 24px for it at the right edge, and the layout leaves it: the last
  column's track runs to the grid's edge, so its cells stop 24px short, and the rows, last row, offset and flag are
  not touched. Manager Status owns an invisible link per row with explicit bounds spanning the row; they span the
  panel. Job Information lists the clubs of the chosen nation whose manager's job is insecure: a club enters
  the list when a short at +6 of its 0x4c-byte statistics row is below 4050, so a capture without vacancies is
  given some by writing that short for clubs the original "has a manager" test accepts; the table then grows a
  scrollbar from the seventeenth job (history slot 1 is its offset). `--uefa-coefficients` (top, offset 17, offset 35), `--hall-of-fame` (an empty table, and twenty or seven
  managers written into the live ranking table of the disposable capture copy, then read by the original builder;
  the 324,000 bytes of the table are compared before and after the layout) `--manager-status` (the capture's one
  human manager) and `--job-information` (no jobs, five, sixteen, twenty and twenty scrolled, the written rows
  compared before and after) check, at six canvases, centering, height, footer equality and alignment, toolbar sides, column
  widths, the scrollbar gutter and scroll state, text bounds, picker hits, in-place redraw, Off and the original Back
  result and RNG, with mutants (footer drift, classic panel, classic toolbar control or subtitle, a cell under the
  scrollbar, a row link at its classic span). Unexercised: the nation chosen through the Hall of Fame's and Job Information's menus (the popups
  open at 0,0 and are placed when they open), the Hall of Fame's "Gravar como texto"/"Carregar" file commands,
  a Manager Status with several managers (a capture has one), UEFA coefficients with real values (a new world
  has zeros) and the sort links' effect. The Hall of Fame's date column shows 1970 for the fixture's synthetic
  rows: only the geometry is compared.
- The manager's job confirmations and the holiday dialog (Manager menu). Applying for a job (builder 0x101c8730,
  opener 0x101c8650, called from a CLUB_SCREENS routine at 0x1005ea84; the menu path was not traced), rescinding the contract (0x101c8b60, opener 0x101c8a80 through the wrapper 0x101aa650, dispatcher
  actions 0x41c/0x41d, argument the club) and retiring (0x101c98c0, opener 0x101c9880, action 0x3f2) are message
  screens with their panel at 110,145-780,535 instead of 110,140-780,545 (a question, a Yes/No footer of two
  columns), so they share `message_screen_layout.cpp`, which now accepts either panel rectangle; the dialog, the
  text carried to the new center and the two 190px buttons under the panel are as described above. Holiday
  (0x101c9eb0, opener 0x101c9e70, action 0x3ef) has its own `holiday_layout.cpp`: an intro paragraph above a
  thirteen-row list of seven settings, each a text with a link measured to its current value, and a Yes/No
  footer. The panel is centered and at most 800 wide, keeps its classic height (the rows keep theirs), the
  settings' text and links move with the list's left edge (a full line also follows its right edge, a link keeps
  its measured width), the intro spans the panel and the buttons are equal 190px cells right-aligned under
  it; the layout declines on a scrollbar, a missing or duplicated grid or a canvas too short for the buttons.
  Gate: a career is open. At a width of exactly 800 the bounded panel is the classic one; the checker then
  tells that the layout ran by the footer. `--message-screens` gained the three confirmations (opened through
  their openers, answered No) and now runs mutants (classic panel, a button drifted, a button inside the
  panel, text off the panel's center) through the same checker; `--holiday` opens the dialog and flips its
  settings through the original handler (first, second, third, all), at six canvases, and compares records,
  RNG, the link texts and the No result with the original, with mutants (classic panel, footer drift, footer
  at the bottom row, a setting at classic bounds, the intro at its classic span). Unexercised: Yes (it starts
  the holiday), the confirmations' Yes (they change club or end the career), the other languages' longer
  texts, confirmation texts that wrap to more lines than the three clubs of the fixture give.
- The transfer dialogs of TRANSFER_SCREENS (all eight builders from 0x1041f8f0). Their openers' arguments, read from the
  rewritten openers in `transfer_screens_confirmations.cpp` and the callers: bid rejection 0x1041f8f0 (opener
  0x1041f830(link, link): the offer link is history 0, and history 1 when 0 is empty; history 2 holds the chosen reason; opened
  from the offer decision's Reject and the proposal screen's refused offer), remove the exchange player from a
  proposal 0x10422b50 (opener 0x10422ac0(link, person ID)), add to the shortlist 0x10424f80 (0x104248a0(person, mode)),
  set a player as the first or second to compare 0x104249e0 (0x10424930(person, _, which)), withdraw a bid 0x104265c0
  (0x10426550(link)), release a player 0x1042ae90 (0x1042ae20(person)), end a loan early 0x1042f7c0 (0x1042f670(person); it
  opens only when the original's eligibility test passes, otherwise the original shows an error dialog and no
  builder runs) and the trial period selection 0x1042b580 (0x1042b500(person); history 1 is the weeks, 1 to 4). Six
  are message screens (one-column panel at 110,140-780,545, a No/Yes footer of two columns), so they only
  joined `ScreenTemplate::message_screen`: the dialog, text carried to the new center and 190px buttons are as above.
  The trial period is the same panel as a grid of seven columns holding one row (label, minus, weeks, plus) with
  no explicit bounds, so `message_screen_layout.cpp` now accepts a seven-column panel: the cells come from the
  columns AutoGrid measured for the classic rectangle (`paged_screen::ColumnMap`, only the widest column, the
  label, takes the extra width) and are written as explicit bounds. Bid rejection has its own
  `bid_rejection_layout.cpp`: a two-column panel (a narrow marker column and a wide one) with two full-width lines
  (explicit bounds), seven choices (the first three always offered, four restricted reasons either buttons or
  disabled labels) and a footer of two buttons (No/Yes) or three when more than one offer is pending (No/All/Yes).
  The panel is centered, at most 800 wide and as tall as its last used row plus one (trailing empty rows are
  dropped through `set_rows`/`resize_rows`, keeping the used-row range and scroll), full lines follow both
  edges, links keep their measured width, the choices follow the wide column, and the footer's buttons are
  equal 190px cells right-aligned under the panel; it declines on a scrollbar, a missing or duplicated
  grid, a footer of another size or a canvas too short. Gates: a career is open; bid rejection also needs the
  offer (history 0, else 1) to name a person inside the database. `--message-screens` gained scenarios 10-21 (the
  six builders, shortlist add and remove, compare first and second, a release with and without compensation, the trial period at 2, 1 and 4
  weeks, with a cell-order/width check for the seven-column panel and a mutant that moves the plus button out of the
  panel) answered through their No or Back; `--bid-rejection` (new, `adaptive_bid_rejection_checks.hpp`) builds one
  offer, two chosen reasons, two pending offers (the manager's offer array and the first offer's chain list written
  in the scratch copy, which gives the three-button footer), and the offer in history 1, at six canvases, with
  mutants (classic panel, a drifted button, the footer at the bottom row, a line at classic bounds, overlapping choices) and the
  gate's refusals. Unexercised: the Yes and All buttons (they send offers, shortlist or release players, withdraw
  bids), the screens' text in the other ten languages, bid rejections with more than two pending offers or with reasons
  other than those the fixture's offers allow (its choices are a mix of buttons and disabled labels).
- Arranging a friendly tour, Credits, Web Sites and the text entry dialogs (a fifth batch of the legacy screens, all
  layout-only). The friendly screens are CLUB_SCREENS builders reached from the club screens' handler (control type 0xb,
  opener 0x10045800(club)): Choose Country (0x10045930; history 1 is the chosen nation, 4 the "available only" filter) is
  a four-column table, two nations per row (name, confederation, name, confederation), sixteen rows and a scrollbar, a
  Filtrar control above its right edge and Cancel(3)/Next(4); Choose Dates (0x10043b00; history 1 is the page, 1 to 20)
  is a six-column table of seventeen rows, a pager of five small controls in the subtitle's band (632-785 x 85-105) and
  Back/Next (Next becomes action 10 once an available date, action 8, is chosen). Both use
  `paged_screen::layout_table_screen` (`friendly_layout.cpp`), which gained two options: `grow_mask` (the two name
  columns of the countries share the extra width equally; every other table still grows only its widest column) and
  `pager_band` (the pager keeps its offsets and its right end follows the panel's right edge; the other chrome rules
  are unchanged). Credits (0x105e9ad0, opener 0x105e9a40, Game menu action 0x42b and the start menu) and Web Sites
  (0x1062c380, opener 0x1062c300, action 0x42c) are one-column text tables of measured text areas (`info_pages_layout.cpp`;
  Credits has a section menu at the left above a sixteen-row scrolling table, whose measured lines reach four pixels into
  the 24px scrollbar gutter in the original and keep that), with Back/Next; their gate needs no career, because the start menu opens
  them. The text entry dialogs (`text_entry_layout.cpp`) are the player's nickname (builder 0x103ac1c0, opener
  0x103ac0d0(person), Cancel(52)/Remove(53)/OK(51)) and Send Message (builder 0x102d0500, opener 0x102d02b0(person or
  instance), Cancel(7)/Send): a title, a subtitle, one edit control at 150,310-740,370 and a footer grid at 100,555-790,590 of
  two or three columns. The edit control becomes at most 800 wide, centered under the title bars and in the upper middle
  of the content area (rectangle and text bounds move together; its height stays 60), with equal 190px buttons
  right-aligned under it; any other arrangement stays classic. Gates: a career is open (friendly screens, text entry);
  none beyond the local non-modal screen for Credits and Web Sites. The checkers are `--friendly-countries`,
  `--friendly-dates`, `--credits`, `--web-sites` (the shared `paged_screen_fixture`, now with `PagedOptions`: columns that
  may grow, the pager band, the footer's back action) and `--text-entry` (`adaptive_text_entry_checks.hpp`): the countries
  in available and all-nations states, the dates on the first, fifth and last page and with a date chosen, Credits on the
  first section, the last section, scrolled and with no career, Web Sites with and without a career, the dialogs for the
  nickname (three buttons), a message to a person and to the manager (two buttons), each at six canvases (the layouts
  ran at every one: the footer is no longer at the classic strip), comparing controls, actions, records and RNG with
  the original, answering Cancel in both runs, and mutants (panel at classic coordinates, a drifted footer button, a
  toolbar or pager left behind, a cell under the scrollbar, the edit control at its classic rectangle or its text bounds left
  behind, the footer at the classic strip). Unexercised: the screens after Choose Dates (Next from the dates screen did not
  open another builder in the fixture), typing into the edit control (the original picker does not report it either,
  so only its rectangle and bounds are checked), the nickname's Remove/OK and Send (they change records or send mail), the
  other ten languages' texts, and the Credits and Web Sites text lines' look at 3200 pixels (the lines keep their measured
  extent and sit in the panel's middle).
- Eight more builders joined `ScreenTemplate::message_screen` (sixth round): have a player scouted 0x103ac810 (opener
  0x103ac780(person, flag), Yes 55 / No 54; from the person screens' action menu), the "choose two players" notice 0x103acc50
  (opener 0x103acc10(person); the Compare action 1076 opens it when fewer than two players are chosen; one OK, 56), and six
  screens opened from **news items**: the Intertoto place 0x10075410 (opener 0x10075380), a job offer 0x101c94e0 (0x101c9350),
  a player's transfer request 0x10093bd0 (0x10093ae0, three buttons, Back 34 first), the two "invalid squad" notices of the
  national teams 0x102bd090 / 0x102bd920 (0x102bcec0 / 0x102bd7f0; lines pre-wrapped at 115..775 and one Ok button
  at 110,555-780,590, which the layout now also accepts as its standalone button) and the friendly tournament invitation
  0x10049550 (0x100493c0, three buttons). A news-driven opener takes (the manager's person, the item's ID): the screen's
  history holds that pair and the builder fetches the item again through 0x102ccdd0 and reads its payload words
  (0x102cb570). Fixtures can therefore publish one with the original constructor and queue insertion (`TransferScene::news_item`:
  0x102bdf90, 0x102cb4b0 per word, 0x102cbc10; the ID the queue gives is its +0x10 field) instead of needing a real
  event; the answer handlers of these screens may mark the item or change the person, so `--message-screens` compares those
  records between the original and the adaptive run instead of requiring them unchanged. Where the club sits in the payload
  differs per screen: word 0 (job offer, the first squad notice, transfer request with the player ID in word 2), word 1
  (Intertoto, invitation) and word 4 (the second squad notice); the probed openers do not check the item type. Unexercised: the confirming buttons (they sign, answer
  or remove news), a real item of each type, other languages.
- Seventh round of New Screens: nine more builders and two new layouts. Seven joined `ScreenTemplate::message_screen`
  (`--message-screens` scenarios 30-36): the "disk full" and "not enough memory" notices 0x10340db0 / 0x10341120 (openers
  0x10340d40 / 0x103410b0 without arguments; two pre-wrapped lines and one Ok at 100,555-790,590; Ok 51 / 52), "take control of
  a club" 0x1033bdc0 (opener 0x1033bd50(club); No 32) and "reassume control" 0x1033c970 (0x1033c900(instance); No 37), and three
  news-driven questions: entering the English cup in a later round 0x104c65a0 (opener 0x104c6510(instance, item ID); the
  competition name comes from a global the news handler sets), a player's appeal against his punishment 0x106fcbb0
  (0x106fc7f0, which reads fourteen payload words: person index in value 4) and the ultimatum to the board 0x10702ba0
  (0x10702a40). `disciplinary_layout.cpp` lays out two grids that share one rectangle at 110,140-780,545 as one panel: the
  disciplinary action on a player 0x1010ff90 (opener 0x1010fef0(person), from the person action handler at 0x103a7edc /
  0x103a9e1c; a seven-column grid with the intro line, the action row "label - level +" and the reason heading, a two-column
  grid with the five reasons, Cancelar 4 / Confirmar 5), the press question 0x1067bea0 (opener 0x1067b920(instance, item ID);
  three explicit lines, three replies 5253, Cancel 4252 / Ok 4251; its seven-column grid has an empty column the column map
  would refuse, so it is only read when a control needs it) and the role in the club 0x100952b0 (opener
  0x10094ae0(person, 0, 0, 1) as the transfer-request flow calls it; two two-column grids and the Back/Choose footer). The panel
  is centered, at most 800 wide, as tall as its last used row plus one, the wide column of each grid takes the extra width, the
  intro line and name link follow the edges, and the buttons become equal 190px cells under the panel. `head_to_head_layout.cpp`
  lays out the club screen's "Last meetings" 0x10041550 (opener 0x100413b0(club, club), from the club handler at 0x100399b4):
  the eleven-column summary (one column measured empty: `ColumnMap::read` takes `allow_empty` and up to sixteen columns) and
  the meetings table become one panel of at most 960, the summary at its height and the meetings as tall as 28px rows allow, the
  View and Filter menus on their sides, the Back/Next buttons right-aligned. Checks: `--disciplinary` (six states: three levels
  and reasons, two press states, the role screen; answers compared with the original, the press ones with the news queue too;
  a reason is highlighted under the mouse, so colors are not compared between runs), `--head-to-head` (three club pairs; the world
  holds no earlier meetings, so the cells are empty) and the new message scenarios, each at six canvases with mutants refused.
  Unexercised: the confirming buttons, a real news item for the four news screens, the history cells of the head-to-head table,
  other languages. Still classic: 0x10048e00 / 0x1004a4a0 (their openers look the friendly up in a table that is empty in the
  capture), awards, competition 0x10073d00, national team 0x102bc400, the transfer-rules article 0x106ff680.
- Eighth round of New Screens: twelve more builders, three new layout files and one new test per family. The national team squad
  screen 0x102bc400 (opener 0x102bc340(person, squad, limit-notice flag) from the person action handler at 0x103a8da4 / 0x103a8e4c;
  squad 0 main, 1 reserve, 2 Under-21; it needs the manager's international side in the instance at +0x20, which the check sets on the
  disposable copy, else the builder raises its own error dialog) is a message screen: a confirmation ("remove X from the squad?",
  No 1 / Yes 2) or the "only N players may be selected" notice (Ok 1), so it joined `message_screen` (`--message-screens` scenarios
  37-40), as did the confirmation 0x10702580 (opener 0x10702470(person, flag) from the person action handler at 0x103a7b6c / 0x103a7bd8 /
  0x103a9aac: flag 1 grants the player a special two-week holiday, flag 0 sends him to a spiritual counsellor; No 2 / Yes 3; scenarios 41-42).
  The Manager menu's meetings with the board 0x105d6ac0 (opener 0x105d6a20(club), from the handler at 0x101c92a8) and with the league
  0x106ee9a0 (0x106ee900(club), 0x101c9304) have the disciplinary dialog's two grids (an intro line over seven columns, the options in two)
  and a Cancel/Confirm footer: they joined `ScreenTemplate::disciplinary` (`--disciplinary` states 6-7). `paged_table_layout.cpp` hands a table at the classic content rectangle to the shared table recipe: the awards of a competition, the award's list over the seasons 0x10000180
  (opener 0x10000070(subject), where the subject is the `+8` word of an entry of the awards table `*(toc -0x7bb4)`, 29 categories; with
  the Ver menu above the table or without it; empty in the January 2002 capture, which shows the one centered message) and the nominees
  of an award 0x100026d0 (opener 0x100025b0(entry): entry `+0x24` is {record pointer, short count} and each 16-byte record names a
  person index and a club index; the check writes 0, 3, 16 and 30 records there), and the friendly tour's invited clubs 0x10048450
  (opener 0x10048330(link to a club, 0) from the club handler at 0x1003a1d4 and the dispatcher at 0x102a880c: four columns, the managed
  club's own tournament at 170 with a disabled "invite another" button, another club's at 145; the capture invites nobody, so the cells are empty). `tabbed_page_layout.cpp` is the shape of a page
  with a tab bar (two grids over 100,T-790,T+35 plus the body at 110,<top>-780,545, optionally a second bar under the body and a
  filter pair, page controls or a summary line above it): the transfer rules article of the news 0x106ff680 (opener
  0x106ff550(instance, item ID), four tabs at 140, lines pre-wrapped at 115..775), the Historial 0x10145290 (opener 0x101451d0(tab 0-4))
  and the game database 0x10106840 (opener 0x10106530(tab 0-3): nations, clubs and staff as a three-column list of 16 rows with a
  filter pair, page controls and a summary line), and a club's Historial 0x1003b7f0 (opener 0x1003b3a0(club); tabs above, four under
  the body). The page becomes one centered panel of at most 960: the tab bar spans it with equal tabs, the body is as tall as its rows
  can use (28px each), the second bar sits under it, the filter pair and page controls keep their side, a summary line spans it and the
  Back/Next footer is right-aligned. `centered_dialog_layout.cpp` moves a fixed dialog whole to the middle of the canvas (every grid,
  standalone control and measured text area by half of the extra width and height, sizes unchanged): the friendly tour's tournament
  setup 0x10046de0 (opener 0x10046c80(club); a frame at 170,137-719,461 with the option rows and Cancelar/Ok). Checks:
  `--awards` (two lists, four nominee states), `--invited-clubs` (two states), `--tabbed-pages` (four article tabs, four Historial tabs, four database tabs, two clubs),
  `--centered-dialog` (two clubs; the options' commands are compared, Cancel and Ok leave through the screens that opened the dialog,
  which the fixture does not have), each at six canvases with mutants refused. Unexercised: populated invited clubs and a populated award list (nobody has won an
  award in the capture; the layout reads the grid's column count and hands any table of up to eight columns to the recipe), the
  Historial and club Historial bodies (a tab's sub-builder fills them from text files that the check platform does not have: the
  bodies are one empty row), real news for the article, how a player reaches the tournament setup, other languages. Still classic:
  the competition team comparison 0x10073d00, the club Historial's file-backed sibling 0x101434a0 (needs its text file), the
  compare-players screen 0x1038e350, 0x100465c0 / 0x10048450 / 0x10048e00 / 0x1004a4a0 / 0x10054810 and the rest of the list in
  `.local/rewrites/screens/NOTES.md`.
- Club information (section 5, builder 0x1002f1e0) uses `club_information_layout.cpp`.
  Shared Yoga tracks bound table width and row spacing. Finance/Income/Expenditure (history27 low bits
  2/4/8) retain their backing/text layers, 10px inset, five column weights and 14-row capacity. Their
  read-only eligibility keeps the original human-control and holiday/away veto. General Info (1) and
  Stats (16) are public for normal and national clubs: metadata remains compact, staff gains up to 30
  visible rows with the original scroll clamp, and statistics retains its original 17-row capacity.
  Growing staff capacity also initializes the newly exposed row weights at +0xbd0; the original
  constructor initializes only its first seven weights, so changing capacity alone creates empty hit rows.
  Staff heading/underline keep their measured width; nation/person links remain whole-cell actions.
  The existing club-bottom hook preserves original construction bounds; the postpass recognizes actual
  grid shapes and moves lower tabs, heading/menu and explicit text coordinates. Original commands,
  accounting queries, records and RNG remain authoritative; network groups keep legacy geometry.
- Club Next Match (section 3, 0x1002a930) uses `next_match_layout.cpp`. The common bottom pin
  preserves original construction at 545; a postpass bounds width to 1600 and grows the original
  3:5 body up to 30 rows, initializing new row weights and using the original scroll clamp. Both
  original 500/545 bottoms and the one-column no-fixture message retain their own structure.
  Rich text emits a kind-1 base before its kind-2 links: left and value groups can share a row,
  so each base establishes the shift inherited by its links. Value bases use their actual 355/362
  origin rather than inferring the original pre-count's gutter. Typography, measured widths,
  commands and fixture queries remain original. Focused checks reuse a captured fixture and
  original date commands; missing player-history archives are temporarily marked absent through
  the reader's existing size=-1 path and restored before layout/record comparisons. Current-season
  data remains captured; archived-history rendering is outside that fixture's evidence.
- Standard league standings (COMP_SCREENS.CPP builder 0x10065340, section 1) use
  `league_standings_layout.cpp`: bounded shared tracks, original eleven column weights,
  up to 30 rows including headings and separators, and aligned tabs/View/Print/stage controls.
  Overall/Home/Away retain the original sorting and commands. Separator X overrides follow
  the actual scrollbar gutter; newly visible rows receive unit weights before the first refresh.
  Future/unavailable messages keep their original measured text, wrapping and row capacity.
  Eligibility reads the selected stage directly; unresolved stages opt in only when every
  possible stage is supported. Never call 0x10450a50 from layout: its virtual predicate can
  finalize league records. Compact groups keep their eleven-column standings at the original height. The six-column
  fixture list below uses the original font-2 row height and no frame inset, and gains up to
  30 rows. Its scroll position is independent of the standings and their qualification lines.
  Captured short/28-club leagues cover menus, scroll transitions, links, records/RNG and ABI;
  controlled fixtures cover message states, indexed-stage eligibility and stage-command reset.
  Section 1 also adapts the nine-column knockout draw built by 0x10067d50. The list starts
  below the original optional stage selector, round title and live draw toolbar. The original
  builder still formats both legs and assigns team and score actions. The Copa do Brasil
  draw-date message, friendly cup live controls and European qualifying playoff with a real
  switch back to its group stage are covered by the existing disposable UI capture.
- The competition screen's Results, Calendar and Match Dates tabs (sections
  2–4 of `0x10065340`) use `competition_fixtures_layout.cpp`. The layout
  preserves the original fixture cache, column weights and link payloads and
  expands the visible rows to 30 on taller canvases. Explicit date-group
  labels and underlines follow the list inset, while empty messages retain
  their measured width and center. Original date navigation, competition
  tabs, Print and Back/Next share the adaptive bounds. The local eligibility
  check does not query or resolve a competition stage.
- Games of the Day (MATCH_SCREENS.CPP 0x1028ae80) uses `daily_fixtures_layout.cpp`.
  Its nine-column list shares bounded tracks, retains original weights, initializes newly
  exposed rows (up to 30), and keeps its 20px navigation gap. Back/Next become equal 170px
  buttons; the title's invisible competition link, subtitle and Competition/Debugging toolbar
  follow their displayed panels. Stage headings retain measured widths; neutral venues move
  by the home-column delta, including the original scrollbar inset. AutoGrid column caches
  are relative to the panel, not absolute screen coordinates. Existing left/right-click cells
  keep their commands, fixture ownership and localized labels. Eligibility only reads the
  live match-day queue and selected sublist; no match/date processing is added.
  Existing queue checks cover normal, grouped/neutral and human fixtures, original debug
  result menus and footer commands, plus controlled GUI overflow, friendly layout and result
  variants. Scores with an archived report are source-reviewed; report navigation is not
  dispatched. Post-layout checks retain queue/fixture/club/history/date/RNG and guest ABI.
- Initial Prepare Game (SETUP.CPP builder `0x10334550`) uses `startup_layout.cpp`
  after its original builder. The shared Yoga tracks bound the original nine
  choices to two columns/five rows, preserve row-major order and update every
  explicit hit rectangle, including the odd centered Web Sites item. The
  two-cell footer stays disabled where the original disables it, with equal
  compact widths. Existing header/sidebar hooks supply the remaining chrome.
  The read-only gate requires local group zero, matching history and no
  first-run settings overlay (`history0 == 0`); it deliberately needs no career
  instance. New Screens Off and first-run/network screens retain classic
  geometry. No new exact-PC hooks or SETUP workflow reconstruction. Existing
  initialized startup captures verify choices, missing-file states, input,
  classic fallback, config/history/RNG and guest ABI preservation.
- Load Saved Game (FILE_SCREENS builder `0x100ee220`) uses `load_browser_layout.cpp`
  after the existing native builder. Shared Yoga tracks align the filename field
  and list in a bounded 1120px panel; taller canvases expose up to 30 rows with
  original font metrics. Cancel/Info/OK retain their original actions with equal
  170px widths. Expanded lists explicitly clear AutoGrid's stale scrollbar flag
  and clamp scroll without rewriting history or filename buffers. The read-only
  gate requires a local saved-career chooser (`.sav`, the original filter, no
  folders-only or plain-cancel flag): Load, Save and Delete (modes 0-2, since
  2026-11-20), from any parent, with the three-button footer or the two-button
  one without Info. Quick Start, folder and network choosers retain classic
  geometry. The values a real Save or Delete passes (parent, Info) were not
  read from a capture: the fixture opens them with the Load call's arguments. No file I/O, save-format or loader
  semantics are replaced. Controlled catalogs exercise empty/short/overflow lists,
  editing and the original selection/Info/OK/Cancel routes without opening a career;
  layout checks preserve styles, localized text, buffers, history, RNG and guest ABI.
- Nations & Clubs (FIND_SCREENS.CPP builder `0x10103850`) uses
  `nations_and_clubs_layout.cpp` for the six tabs and restricted browser modes.
  Each tab's history stores the selection, scroll position and list index in
  consecutive slots. The layout uses that index after construction, expands the
  original two-column groupings to up to 30 visible rows, and keeps the two
  tab strips, filter, heading and Back/Next buttons aligned to the bounded panel.
  List membership, localized labels and original AutoGrid input stay in the
  existing callbacks. Classic, network and secondary screen groups keep their
  original geometry. No change to the generated FIND_SCREENS routines.
- The global Transfers browser (`0x10426aa0`) lays out the existing native
  five-column table before emitting controls. The bounded Yoga track provides
  both the panel height and the page size, so the page label, original pager
  commands and displayed records stay in sync. Enlarging the window clamps an
  unavailable page to the new last page. Date controls, nation filter, empty
  message and footer follow the same panel. Monthly/yearly selection and the
  transfer cache remain owned by the existing handlers; the classic builder
  path keeps its original capacity and coordinates.
- Career sidebar (0x102a1070) and progress (0x1076e5f0) have audited geometry pins in
  `tools/adaptive_gui_audit.py`. Sidebar background/menu grids follow canvas height, and all eight
  bottom status/action branches follow its bottom edge. Progress text, fill, frame and partial repaint
  share the committed canvas, including Continue's history-free processing interval. Widths/font styles
  of the sidebar remain original; the network-only client's waiting redraw remains legacy.
- 0x1071de80 interprets screen commands (SCRMAN calls it 37 times): one dispatcher and 26 command functions
  (`display_interpreter.hpp`), two jump tables (27 and 11 entries). Every exit returns -1 except commands 4 and 6. The
  stream's halfwords and words are little-endian inside big-endian memory; a failed read reports ("network.h" as the
  file name) and leaves the destination, register or frame slot as it was, so later code reads stale values.
  Command 8 sub 1 (a control's rectangle) passes its bounds through `live_match_update_rect`: with New Screens on
  and the live match displayed, the possession bars (0x10291470, 0x10293fb0) and the Latest Scores headings
  (0x102988f0) follow the laid-out screen. Otherwise, and for every other caller, the bounds are the original's.
- The table destructor 0x10718ba0 re-reads -0x56dc after clearing it, as the original; nothing between can store there.
- In the run of all entries the interpreter's cases answer the outside routines its answered unit routines would reach
  (network send, file lookups, graph_carbon drawing): `interpreter_outside_answers` in the checker.

## COMP.CPP: the base competition class

53 routines (0x1044fed0..0x10453394, `tools/comp_audit.py`), native-first: `native/comp_*.cpp`. The audit's docstring
holds the evidence for the fifteen entries outside the span of the module's string pool (TOC -0x49c0): the object
file's run of function descriptors, the class vtable 0x108cf608 and the contiguity of an object file's code.

- 20 entries are the base class's one-instruction virtual defaults. The comparators 0x104532c0 and 0x10453300 have no
  direct caller: ARG_PRM.CPP and ARG_SECOND.CPP pass their descriptors to qsort.
- 0x10453190 keeps its 0x70 frame and the original r29-r31 at its call at 0x10453280: the BlockMoveData bridge later
  reads 417 bytes that `delete[]` leaves under that call (`same-save-continue` reaches it; the case checker cannot
  see this reader, only the paired replay).
- 0x10452ed0 is called from MATCH_ENG, MATCH_MAN and MATCH_PL, so it keeps the original frame until the match replays
  have a stack audit.
- 0x10452ed0 and 0x10451f80 call 0x10459b50, which reads r7-r10 that neither wrote: they are leftovers of the earlier
  callees 0x104abf40 and 0x10494d20 (or the caller's own). The rewrite leaves them alone, which is equal only while
  those callees run generated code; revisit when they are rewritten.
- The news texts call 0x1074bab0, which copies register save areas into the news link row; the copy reaches r13-r24,
  so those hold the original's values before each such call.
- 0x10450ee0 (the fixture calendar) reads address 0 when its date allocation fails and never bounds its 1000-row
  block, as the original. Its resize-failure assertion (line 0x495) is written from the listing and has no case: the
  checker cannot force it (the original runs `realloc` in place), and the original cannot reach it without first
  storing rows beyond the block, since the allocator's resize only fails when it must grow
  (`comp_routines_calendar.inc`).
- Loop counters over stages are `char`/`short` and wrap as in the original.
- No RNG draw and no save access in the module. The pool's "%s.tmp", "%s_%d.tmp", "CM3_TEMP" and "r" are read by no
  instruction.
- `same-save-continue` (13 entries), `season-day` (0x10450900, 0x10450b60, 0x10452ed0; it needs
  `CM_ENGLISH_RULES_ORIGINAL=1`, as every replay of that capture does since the English rule changes) and the
  nation-awards `original-save` (one) reach the module among the captures that still replay; the rest of the
  evidence is controlled cases on the season-day snapshot.

## LEAGUE.CPP: the base league class

67 routines (0x1049e4f0..0x104aac14, 12,587 instructions, `tools/league_audit.py`), native-first: `native/league_base_*.cpp`
(`league_*.cpp` already held two screen layouts), all in production.
- The allocation failure of 0x1049f030 is checked with a negative club count: the element count is a signed halfword,
  so the 32-bit size wraps past what the allocator accepts and the real allocator returns null on both sides. No
  `answers` entry can do that, because the generated original runs the allocator in place.
- Blocks: the save reader 0x104a6850 and writer 0x104a8530, fixture generation 0x104a4970 (draws `rand`, reseeds with
  `srand` twice), season setup and schedule rounds, standings and results, the daily update and season end, news.
- `english_rules::premier_bench` used to be a generator hook at 0x104a8518 inside the loader; the rewrite calls it
  after the `fclose`, before the success return.
- Generated callers read r4-r10 after 26 of these entries (`league_base_volatile_readers.inc`); the checker compares
  them at return. Four cross-block differences of that kind appeared only when all blocks ran together (a comparator's
  r5 handed up by qsort, r9 of 0x104a44f0, the pilot comparators' r4/r5, r4-r7 of 0x104aa340).
- Stack readers: 0x104a1540 keeps every frame store (GAME.CPP 0x1011d1d4 and the BlockMoveData bridge read what it
  leaves); 0x104a28d0 builds its frame at the original sp; the writer keeps its locals at sp+0x38..0x40 (MAIN.CPP
  0x107795d8) and 0x104a60e0 its frame (STADIUM.CPP 0x104affc0). 0x104a26b0 is called from MATCH_ENG.
- Left unwritten on proofs, in the RNG code: draws of 0x104a28d0 with a bound beyond +-65535 (its bounds are an int16
  rating and that plus 1500), the deeper levels of 0x104a3350 (the second level's bound is at most 32768), and a
  division by n-1 in 0x104a00a0 where n is always even (the rewrite throws there).
- A typed COMP routine called directly needs its argument registers set: `comp::news_competition_year` now puts r4/r5
  itself (the league news calls it through the dispatcher; the direct call faulted before that change and was not
  retried after it).
- `season-day` and `same-save-continue` reach 21 and 25 entries, the nation-awards `original-save` the writer; no
  capture reaches the loader.

## CUP.CPP: the base cup class

47 routines (0x10478f20..0x104866f0, 13,700 instructions, `tools/cup_audit.py`), native-first: `native/cup_base_*.cpp`
(`cup_stage*` is another module), all in production.

- Blocks: the fixture tree 0x10479100 and the draws (RNG through the buffered stream, scaled with 65535.0), a fixture's
  result 0x1047bfb0 (who goes through a tie; MATCH_DAY calls it through vt +0xc), the round's draw and fixtures, the
  save reader 0x104831a0 and writer 0x10484420, the daily update 0x10482ca0, news, prizes and ranks.
- The Inter-American Cup retirement is tested twice: the generator hook `inter_amer_cup_skip_daily` stays ahead of
  the module's hook at 0x10482ca0, and the C++ body begins with the same test, because a dispatch probe or a direct
  call does not pass the generated entry. Change the policy in both.
- Series ties (fixture flags 0x2000): the decider 0x10482640 takes 3 from the winner's mark at fixture +0x3d/+0x3e and
  answers 4 until a mark is below -5. The postponed path of 0x1047bfb0 hands the fixture copy to the fixture manager
  with those marks intact; a first version zeroed them, which only a review caught, since no case had nonzero marks.
  The checker now captures the 0x50 bytes handed to 0x104888f0 and 0x10489900 (`Case::Capture`).
- 0x1047bfb0 passes on bytes of its fixture copy at sp+0x360 that it never writes on outcomes 1 and 2 (0x38e, 0x39c,
  0x3ad-0x3af, 0x364/0x378 unless the derived cup's vt +0x84 fills them). They are whatever earlier code left at that
  depth and reach fixture records; no stack reader is listed for it and only four captured calls reach the routine.
- Stack readers: 0x1047f3a0 keeps every frame store (the BlockMoveData bridge reads them, and it reads its own stale
  byte at sp+0xe7); the daily routine calls DATE 0x107156b0 at the original stack pointer (GAME.CPP 0x1011d1d4 reads
  what it leaves). Both rest on the paired replays.
- Left unwritten on proofs the review confirmed: the floating-point arm of the tree's draw and the recursion of
  0x1047a9e0 (bounds are sign-extended halfwords; the finer bound is at most 32768), the dead assertion
  0x10481f6c..0x10481fcc, the repeated compare at 0x10482228, the dead `or r29,r3,r3` at 0x104859f4.
- Several outcomes return only with the checker's `--null-page` (a null competition record, a failed `new(8)`, a null
  fixture read before its null test): the original reads address 0 there.
- `season-day` reaches 3 entries and `same-save-continue` 14; the nation-awards `original-save` the writer. No capture
  reaches the loader, the tree or three of the four draws.

## fog_of_war.cpp: what each human manager knows of every person

24 routines (0x106fd120..0x106ff550, `tools/fog_of_war_audit.py`), native-first: `native/fog_of_war*.cpp`,
`cm-fog-of-war-checks`. The game's table of sixteen slots (TOC -0x7978) holds one knowledge object per human
manager (slot `people - 1 - id`); an object points at one 12-byte entry per person (level byte 0-3, the date last
seen) and at its manager. "fog.dat" saves the preference byte and each present slot's manager index and entries.

- Quirks kept: the reader accepts a manager index equal to the number of people (`index > count`); a failed
  allocation while reading leaves the file open, and after any read failure the slots keep pointing at the
  objects just destroyed; a failed write leaves the file open. In the daily floor (0x106fe820) a manager with no
  club knows fully everyone without a contract: the contracted club (0) equals the manager's club (0).
- `sight` (0x106fec70) draws its roll inline below a sign-extended byte, so its wide branch and the unit's copy of
  random(n) (0x106fef10, called from there only) never run in the game; 0x106fef10 itself is checked by direct calls.
- The file and lifecycle routines and `initial_level` hold the original's r13-r31 at their calls (CallRegisters):
  a capture's replay compares every non-volatile register at each import (`observed_by_import`), and the file
  layer passes most of them through to `PBWriteForkSync`. The daily rules make no call that reaches an import.
- The generated reader inlines `labs` (0x10775550) of the saved game's version: a check sets the version cell
  rather than answering the call. The stack audit of the 20 contract captures shows no reader of this unit's frames.

## TRANSFER_MANAGER.CPP: the native-first family

The module's first 87 routines (`native/transfer_manager_*.cpp`, `transfer_manager_routines.inc`) were transcribed
instruction by instruction before 2026-11-06. Routines written since are native-first and live apart:
`native/transfer_manager_native_*`, lines `TRANSFER_MANAGER_NATIVE` (production) or `_DRAFT` (checkers only) in
`transfer_manager_native_routines_<block>.inc`, their own adapter and `cm-transfer-manager-native-checks` (cloned from
TRANSFER_OFFER.CPP's: world-state cases, mutations, call trace). `tools/transfer_manager_audit.py` reads both tables;
the generated hook `try_modern_transfer_manager` hands entries it doesn't list to the native table. Both tables share
one declining flag, so an original run of either side (or `CM_TRANSFER_MANAGER_ORIGINAL=1`) runs every routine of
the module as generated code.

- Same contract as TRANSFER_OFFER.CPP: native inside, the original's frame, and the original's r13-r31/f14-f31 at
  every call (`keep`), which the checker compares at each call. Calls between native routines, a block's own
  included, go through `invoke_native` so the call trace sees them on both sides.
- `tools/call_arguments.py` used to stop at the prologue's `bl` to the FPR-save millicode (0x107737f0...) and
  report mask 00 for every routine that saves FPRs, so no argument of theirs was compared. A leaf callee now
  clobbers only what it writes; the corrected masks also show many routines passing r8-r10 on unread (0x103fb840
  hands them to 0x10362ef0), which the rewrites set as the original holds them.
- Stale frame reads kept: 0x103fa8e0 (sp+0x4c) and 0x103fa290 (sp+0x50) after their 0x107156f0 call, 0x103f8cc0
  (sp+0x68), 0x103fa290's sp+0x40 on the 0x21 path, and 0x103e8d30's 50-word list at frame+0x38 for a human club,
  which it never fills (ids past the people limit there are skipped; a case writes them).
- Unreachable in the original, proved from the listing: 0x103ee1a0 is `li r3,1; blr`, so the tests of it for 0 in
  0x103e8d30 and 0x103ed520 never take; 0x103f83c0 never stores reason 0x13; the second level of 0x103e3860
  (as in 0x10415e50) can't see a range past 0xffff; 0x103e8500's draws after a count > 1 test never get range 0.
- Several news routines read through a null club or contract (a field from guest address 0). The game stops there
  (page zero is unmapped); the checker's `--null-page` maps it so the paths beyond are compared too.
- 2026-11-16: the other 74 routines (the offer pipeline from GAME.CPP's daily pass down, and 30 news-message
  renderers called from NEWS.CPP's `render_message` 0x102be040) joined the family; the module is all C++, 127 of its
  routines native-first. Some renderers are not only text: 0x10405ca0, 0x10401c90, 0x10402e70, 0x1040d0d0,
  0x1040d820, 0x10410a60 and 0x103fe1a0 write a word of the stored news item through 0x102cd0c0.
- A leftover reader no audit listed: the date words 0x103ea840, 0x103f3100 and 0x103ec0f0 copy into their frames are
  read later as stale stack; same-save-continue diverged until the rewrites stored them at the original offsets.
  0x10401c90 and 0x1040c580 copy a 400-byte text template into the frame, 0x10407a50 a 200-byte one (read by
  MAIN.CPP); all are kept although later code overwrites them.
- Checking: `Case::answers` reaches any routine, value 0 included, except those under check in the run (the `--only`
  set), so cases that steer a native callee run with `--only` limited to their block ("answered: ..." labels). An
  allocation failure is forced through a count or capacity that makes the request enormous; the growth
  allocations of 0x103f45a0, the 16-byte list headers of 0x103e25d0/0x103e3170/0x103e3410 and 0x103f3670's growth
  realloc can't be made to fail short of exhausting the heap. 0x103f3670's yearly history files are read through
  the C library's buffer, so a case supplies a file record whose buffer holds the rows. `--null-page` is part of
  the equivalence test: some case lists read page zero while building.
- The call trace caught two wrong return addresses (0x1040e2d0's field 5 call, in production since ea39c099, and
  0x103e70f0's call of 0x104ae570), and mutated cases a load order (0x103e76c0 faulted at another address); no
  replay showed any of them.

## COMP_UTIL.CPP: competition utilities

35 entries (0x10462cd0..0x10477d54, 21,480 instructions; `tools/comp_util_audit.py`), files `native/comp_util*`,
checker `cm-comp-util-checks`. The neighbour 0x10462b80 has no proof of owner and stays pinned as ambiguous.

- **Three routines are tables compiled as code** and land as generated data with a small typed function around
  it, not as transcribed branches. `tools/comp_util_tables.py` reads the jump tables of 0x1046f1f0 (a digit per
  round code, language and kind: inferred, the grammatical gender class of the round's name) and 0x10469bb0 (one
  localized format per round code) by emulating their range checks over every halfword code;
  `tools/comp_util_headline.py` walks the decision tree of 0x10464d00 (the result sentence of a news item)
  symbolically into 173 rows over 22 named facts. Both tools fail on a shape they cannot name, check that every
  table entry is reached (or that exactly one row matches each reachable combination), and have a `check` mode
  that fails when the committed `.inc` is stale. The `.inc` files hold numbers and string-pool offsets, never text.
- The language these routines switch on is the preferences record's byte +8; a case sets it through the byte at
  TOC -0x35ac.
- An error arm of 0x1046f1f0 does not return: 1,667 of them fall through into the next range test, which is why a
  code outside one stretch still answers from a later one.
- With language 8 and a kind no caller passes, 0x10469bb0 translates text it never initialised (leftover stack)
  and faults; the rewrite faults alike.
- r4-r10 are left as the original except for the entries listed, with their proof, in
  `comp_util_volatile_free.inc`. 0x10463990 was removed from that list: two callers reach it through a vtable.
- Captures reach only 0x10464010, 0x10464880, 0x104646d0, 0x10464160, 0x104647e0 and 0x10462cd0; everything else
  rests on fixtures built from the season-day world state, and no stack audit exists for those entries: the
  stores a callee could read were reproduced from the listing. The season-day pair needs
  `CM_ENGLISH_RULES_ORIGINAL=1`.

## PLAYER_REGEN.CPP: retirements, newgen players and staff

47 entries (0x102d8230..0x102eb6f8, 19,660 instructions; `tools/player_regen_audit.py`), files `native/player_regen*`, checker
`cm-player-regen-checks`, switch `CM_PLAYER_REGEN_ORIGINAL`. Pilot blocks routed: `regen_queries` (the person class, the list A
search and queued test, the two qsort comparators) and `candidate_score` (the pooled Random 0x102da630, the candidate score
0x102dc410 with 808,000 calls a season day, the pick 0x102dbf00).

- The module's draws are inline copies of the pooled Random (cursor at TOC -0x7b84, refill `bl 0x1077b770`). The checker copies the
  pool's cursor, salt, end, the generator's state word and the sample under the cursor into the compared memory after every call
  (`--corrupt-draw` is its control), and a refill is called through `c.call` with the original's return address (the call trace pairs
  calls by it) with the link register restored afterwards, because a routine finishes through it.
- The scaled form of Random (bound of 0x10000 or more in magnitude) has a second bound of `n * 10 / 65535` with the product wrapping in
  a word, so at most 32768 in magnitude: only two of the original's four inlined levels run. The scaled draw leaves r4..r9 as the
  last executed level does (r10 never); small draws leave them alone. The cubed term of 0x102dc410 reaches the scaled form (a byte
  +0x2f of 82 or more halves to 41, whose cube exceeds 65535).
- `pow` is the host's `std::pow`, as `carbon_bridge.cpp` computes it, called directly: the glue 0x10780fc0 only saves and reloads r2.
  FPSCR bits follow from the same operations in the same order (including a repeated `fctiwz` of the pow result around the scaled draw).
- No stack audit reader reads bytes these entries leave (season-day and same-save-continue: the only readers of this module's bytes are
  0x101d1900 and 0x10371b2c, of 0x102d8920's writes). 0x102dc410 and 0x102dbf00 leave r4-r10 free (`player_regen_volatile_free.inc`).
- Blocks `regen_lists`, `club_placement` and `detach` are routed too (27 of 47 entries). The regen object's lists are
  `{entries, count, capacity}` at +4/+8/+0xc (A, 0x1c-byte retirement entries), +0x10/+0x14/+0x18 (B, 0xc), +0x1c/+0x20/+0x24 (C, 0x10)
  and +0x3c/+0x40/+0x44 (D, 0x10); a full list (count = capacity - 1) reallocates 250 entries more (0x107751b0) and an absent block
  reports the assertion and ends the game. Quirks kept: the appender of C tests list D's count; list A's appender copies only some
  fields (+0 +4 +8 +0xa +0x10 +0x12 +0x18 +0x19) and its removal clears the vacated entry's id, byte +0x18 and four halfwords but not
  +0x19; the registration 0x102df1a0 accepts an id equal to the people count, and its D growth tests the list's COUNT (not its block)
  for the assertion.
- The two club scores keep their 16-bit values in 32-bit registers and truncate (extsh) only where the original does; their draw bounds
  are bytes or halfwords, so no scaled draw is reachable. 0x102e21c0 and 0x102e1f20 dispatch through TOC jump tables (-0x50b4 and
  -0x50b8 for the staff, -0x50ac and -0x50b0 for the player; read from the executable, the cases are in `player_regen_detach.cpp`).
- A routine that calls out runs inside a `Frame` of the original's size (its callees keep the original stack depth: a mutated run
  that changes a stale stack word a callee reads differs otherwise) with the callee-saved registers set as the original holds them.
  Entries whose callers read only r9 and r10 (0x102e99f0, 0x102ea3c0, 0x102dc000: `register_contract.py --after`) are compared on those
  registers (`player_regen_volatile_read.inc`, a mask the checker applies), which no routine of the block writes.

- Blocks `retire_decision` (0x102dcb20, 0x102dd910, 0x102de6e0, 0x102de470) and `persistence` (`retire.dat`: 0x102ea870, 0x102ead70) are
  routed too (33 of 47). No capture reaches the three decisions, so their cases start from the real people (season-day) and edit the fields
  over every threshold, with the contract manager's, date, competition and protect-list callees answered where a path needs a value.
  The decisions read the callee-saved r19 of their caller (a staff member with no job byte): the hooks hand it over (`incoming_r19`); the
  callers never write it. 0x102dcb20 values a player in single precision (fsubs, a fused fnmsub, frsp, fmuls) with 1.0 - 0.075 * (age -
  limit): single against double precision changes the truncated value at 1500, 3500 and 5500 (the cases of the search), while fused against
  separate never does after the frsp. 0x102dd910 truncates the fnmsub 1.0 - 0.1 * gap in double precision: fused against separate differs
  only in FPSCR (gap 10: -5.55e-17 against 0, sticky XX), so those cases clear FPSCR first (`clear_fpscr`). That truncation is 0 for every
  gap the routine can reach (1..15), so its score is always 0 and the branches after it (score >= 5000, the draw of the player with no club)
  are dead code the C++ keeps.
- The persistence file is, for lists A, B, C and D, the 4-byte count and `count` entries of 0x1c, 0xc, 0x10 and 0x10 bytes. The writer
  carries on writing after a failed call (the handle variable by address goes to the close routine); the reader fails with the alert when
  retire.dat is absent. The checker's `Case::Archive` is an in-memory stand-in for the zip entry functions, whose file bytes, call log and
  counts end in scratch memory; `Case::verify` checks the final state against an independent serialization (and a load against the image).

- Blocks `news_posting` (0x102df780, 0x102e0090, 0x102e13c0) and `news_texts` (0x102dfbc0, 0x102e0510, 0x102e15b0) are routed too (39 of 47;
  `player_regen_news*.cpp`, `player_regen_news_detail.hpp`, cases in `player_regen_checks_news.cpp`). The posting routines build a news item on their
  own frame (sp+0x38 or sp+0xb0, so its address, an argument every callee sees, is the original's), set its words one call of 0x102cb4b0 at a time
  and hand it to the news manager (0x102cc030, or by prominence 0x102cc8e0, 0x102cc400, 0x102cc280, 0x102cc140 to the club or to each interested manager
  0x103fc710 lists at sp+0x38, up to 30). The text builders read the words back (0x102cb570), build the name into a frame buffer (0x100cb590) and format
  one sentence (0x10727890). The formatter's argument list is positional: a sentence passes the gender and name values in the orders the languages need
  (up to 13 values: r5-r10 and the stack words from sp+0x38), so a site is one `say` line (pool offset, address of the original's `bl`, values); a
  site with fewer than six values leaves r6-r10 as the last callee left them. The title routines 0x100c6bf0 and 0x100c6af0 write their kind byte through
  a frame pointer only for some inputs, and the sentence reads that byte back whether or not it was written: the rewrite reads the same frame address.
- 0x102df780 draws once per coach it looks at (assistant manager +0x94, coaches +0x160...; the pooled generator, one refill site) and computes the score
  with the one `fmadd` of the news: the constant at +0x98 of the unit's table is 12.5 (0x4029000000000000), the kind byte of the club (signed) times it plus half
  the reputation plus the noise (a draw modulo 2500): all of them integers or half integers far below 2^53, so the product and the sum are exact and
  fused against separate rounding cannot differ (FPSCR included: the cases with `clear_fpscr` agree; the rewrite uses the builder's multiply-add all the
  same). The scores come from 0x10219e40 (answered per call in the scripted cases, real in the others) and are compared as signed halfwords with
  reputation / 50 + 75 (great) and reputation / 100 + 50 (good).
- Registers: every routine of the two blocks is compared on all of r4-r10 at return (no exemption, no mask). The posting routines return through the last
  callee's leftovers except where the original sets r4, r5 or r6 with no call after (the early exits of 0x102e0090 leave the club in r4; its dispatch
  leaves r4 = the player record, r5 = the club's +0x30 or +0x2c word, r6 = the club or the staff record when the readers' loop runs no call); r13-r31 are set
  as the original holds them at each call (the callees save them). The text builders end with the name appenders, so r4-r10 are theirs.
- 0x102e15b0 reads the club row's +0x23b and +0x221 for a club that is null (word -1) in the sentences of the non-human branch: the original reads the
  addresses 0x23b and 0x221, which the checker's cases fault on (both sides alike) unless `--null-page` maps low memory; both modes agree. Whether the game
  can post a staff retirement text with no club in that branch is not established.

- Block `newgen_staff` (0x102e27e0, `player_regen_newgen_staff.cpp`, cases in `player_regen_checks_newgen_staff.cpp`) regenerates a retired non-player staff
  person in place, with 61 inline draw sites whose order depends on the data. The draws in the original's order: a bound of trunc(0.75 * the staff record's
  halfword +0x26) (none when it is 0; double precision, and `|bound| <= 24576` so the scaled form of Random can never run), then below 750, 200 and 50; the age by
  the job (one draw, the jump table at TOC -0x50bc: jobs 0, 5, 9 and everything outside 0..10 take the default arm; a table of {modulus, offset, refill site});
  for a manager (job 1) with a club and a staff record the attribute bytes by the club's reputation (halfword +0x1ec: above 6250 with the flag byte +0x241 nonzero,
  above 6250 with it zero, above 3250, the rest: four constexpr tables of plain / difference (two draws below 100, the first remainder kept across the second) /
  tested (a draw below 5, 10 or 100 chooses between two draws) attributes); then below 365 (the birth day), below 125 (the personality tier by ten thresholds)
  and, with a club, a staff record and a job the contract manager's test (0x1007e4f0) passes, below 28 and below 6 (the contract end).
  One single-precision step: +0x24 is replaced by trunc(0.5f * +0x26) when it is below it (fsubs, fsubs, fmuls, fcmpo, fctiwz).
- The job byte is the club side's (+0x62), else the national side's (+0x60), else the callee-saved r20 the caller held: 0x102d8920 loads it from TOC -0x7b84
  (the address of the pool's cursor cell, 0x10be8284 in this executable) at 0x102d893c and never writes it again, so for a person with neither link the job is
  that address's low byte as a signed byte (-124: above 10 unsigned, the default arm). The hook reads r20 before the frame. r4-r10 are compared in full (no mask):
  the last callee (0x100c3870) leaves them; every call gets the registers the original sets just before it (the callee-saved r19-r22 are mirrored for the callees'
  prologues), and the path that skips the contract block sets r4 (the last draw's sum) and r5 (the cursor) as the original's arithmetic leaves them (not
  observable: the callee overwrites both). No capture reaches the entry: ~4,300 cases from the real people (the job byte over 0..255 on each of its three sources,
  reputations around 3250 and 6250, potentials around the bound's thresholds, the contract test's answers), the pool refilling at every draw (the pool's end cell set
  to its start) and at each draw position in turn, forced remainders at the tested attributes, all 61 refill sites paired by the call trace, every `if` outcome of the
  source hit (`contract != 0` after the clear only with the clear answered: a real contract is cleared by it), 15 source-level controls found. The cases with a
  null staff record run with `--null-page` (the original reads addresses 0x24..0x43 otherwise: both sides fault).

- Block `newgen_player` (0x102e4530, `player_regen_newgen_player.cpp`, cases in `player_regen_checks_newgen_player.cpp`) creates a new young player in place of a
  retired one (the caller 0x102dabb0 reads no register after it; no callee-saved register of the caller is read). 131 inline draw sites, none in a loop, the
  order data dependent. The order: the draw below 4 that may replace the person's nation (a wide Random below the nations' count; the second nation by the
  plan's third index with draws below 100 and 3), then one of two branches. A nation with no flag byte (+0x120) and a reputation word (+0x6c) under 350
  (`modest_nation`) draws the age first (a wide Random below the reputation against a draw below 1000, then below 5 plus 23 or below 7 plus 17), the ability
  (+4 of the player record = trunc(factor * potential) in one multiply-add with a wide draw below trunc(factor' * potential)) by the age (above 20: factors
  0.55 and 0.25, else 0.6 and 0.25), then +8, +0xc, +0xa and the age tier's clamps; any other nation (`strong_nation`) draws the ability by the position ratings
  (four arms: +0x15, +0x11/+0x10, +0xf = 20, else), +8, +0xc, +0xa and then the age by a draw below 10000 with the ability's cap. Both then take the birth day
  (below 365), the personality bytes (+0x1b, +0x36, +0x1d, +0x2e with a wide draw below 3 times a bonus of -5, 0 or 5, +0x34, +0x44, +0x3c, +0x3d, then +0x1e,
  +0x24, +0x2c, +0x2f by age) and the placement (a draw below 4, the club pick 0x102e99f0, the signing 0x102dc000, the youth news and a release; else draws below 15
  and 10 and the move 0x102ea3c0). The wide Random takes the scaled form only for the nations' count and the nation's reputation word (every other bound is
  below 24576 in magnitude); its second draw is a real call of 0x102da630, and the two inline copies allocate their registers differently (the registers the
  callee leaves, which the next callee reads, are set per copy). No capture reaches the entry: the checks start from the real people, player records and nations
  of season-day, edited over every byte the routine branches on, with the pool refilling at every draw, at chosen draws and with forced remainders, negative
  salts, and the callees that open files answered (0x10186ea0) or the nation count restored for the callees that loop over it. The default run is 727 cases
  (7.9 minutes serial: every site paired by the call trace with `--argument-masks`, findings 0; every operand of every `if` seen both ways but one); CM_NEWGEN_PLAYER_FULL=1
  (155,832 cases: every value of each rating and personality byte, every potential, 12,000 random setups) was run by hand in label groups on the final code, with every 8th potential
  (CM_NEWGEN_PLAYER_STRIDE=8): 35,000 cases, no difference. The routine's r7..r10 leftovers are read by
  0x1014b690, 0x10309850 and 0x100c2800: r7 and r8 are set where the original's `li` are, r9 and r10 pass through. 3 of the 131 refill sites are not
  written: the original's age-tier tree asks twice about the age (`age <= 20` then `> 26`, `> 23`, `> 20`), so three of its five ability arms (refills
  0x102e50e4, 0x102e52d0, 0x102e54bc) are dead code (proved from the listing: each entry has the one predecessor, the age register is not written between).

- Block `retirement_processor` (0x102d8920, `player_regen_retirement_processor.cpp`, cases in `player_regen_checks_retirement_processor.cpp`) is the daily walk of list A
  of the regen object: it sorts the list, then for each entry in order either drops it (a person under 30 with a club or a blocking class, one the contract manager or
  the transfer manager holds back, one on a short contract: 210 days or more away or age 38 are the limits), retires the person (announce, release the contract, shortlist,
  national and dispute bookkeeping, detach a player, then by kind: a pure player may be given a staff record from list C by three draws (the byte +0x67, % 100, a bound that is
  the cube of the ability byte plus the player's halfword, wide above 0xffff, and % 50000 with a second chance; the staff record's halfwords are then derived by pow in three
  variants and a position byte of 20 swaps two bytes), a person with both records goes to the player pool, anyone else is registered for regeneration or regenerated at once
  by job) or, for the first entry not yet due, posts the announcement once and ends the walk; last it sorts list B. The routine calls this module's routines directly
  (`remove_retirement`, `announce_retirement`, `detach_player`, `regenerate_staff` with its own r20: the address of the pool's cursor cell, which never selects a job here
  because it is called only for a person with a club, ...) and the other modules' through the dispatcher at the original's return addresses; r13-r31 are mirrored for the
  callees' prologues; r7/r8 are set where the inline arithmetic leaves them for a later callee (r8-r10 are read by 0x102df020 and 0x10365590, r7-r10 by the sorts; the caller
  reads r6-r10). 12 inline refill sites, all paired by the call trace, plus the call of 0x102da630 of the wide draws; the wide forms at the bytes' draws (+0x67, +0x63, signed
  bytes) and the first draw's wide copy at 0x102d8e10 and 0x102d980c are unreachable (a signed byte bound), and the cap at 199999 of the second chance cannot be exceeded (the
  scaled part is at most 199996 and the tenth of a draw below 30 at most 3). Real captures reach it (season-day once, 18 retirements, 8 conversions; same-save-continue 7 times). The default run is about 2,180 cases (3 minutes: real people of season-day with list A and B rewritten per case,
  dates on both sides of both thresholds, every filter, hold and short-contract combination with the deciding outside routines answered, controlled draw scripts for the
  conversion and its equalities, negative salts, a refill at each draw position, the tails by person kind and job byte): all 12 inline refill sites paired by the call trace with
  `--argument-masks`, findings 0, every operand of every `if` seen both ways but the unreachable cap; CM_RETIREMENT_FULL=12000 (random sweep, run once by hand in nine label groups): 11,436
  cases returned, no difference (563 end at a callee that reaches the file system, on both sides). 30 source-level controls were found (thresholds, moduli, refill addresses, the
  weights and limits of the three ability variants); not found, and why: the weak variant's third factor (its limit 1.0 always binds), the cap, the r20 handed to `regenerate_staff`
  (never selects a job here), and the import glue's stack word. The live check (`--check-routines`) on season-day (1 call) and same-save-continue (7 calls) and the paired
  replay (MATCH=2) agree; the stack audit lists two readers (0x10371b2c, 0x101d1900) of bytes the dispute callee leaves at this frame's depth, which every callee reached at the
  original stack pointer reproduces. With this block all 47 entries of the module are routed.


- Blocks `regen_pending_processor` (0x102dabb0, `player_regen_pending.cpp`) and `lifecycle_and_drivers` (the constructor 0x102d8230, the destructor 0x102d85c0 and
  the scans 0x102d86c0 and 0x102d8830, `player_regen_lifecycle.cpp`) are written and hooked. 0x102dabb0 regenerates the due entries of the queue (list B: its year
  halfword unsigned against the game's, then the day halfword signed, with the compiler's xor/shift compare evaluated as the original does), each by 0x102e4530 and
  0x102df6d0 (an id outside 0..the people count reports the assertion of line 0x36e and ends; the count itself is accepted), while the player pool (list D) is
  not empty; then, with more than 2 entries in list C, up to count - 2 times, the best candidate (0x102dbf00) of the nation the first entry names takes that
  entry's 0x44-byte staff record (table TOC -0x79f8). A person with no player record draws +0x28, +0x2a (below 1500) and +0x2c (below 500), each plus 1. One with a
  record may swap the staff bytes +0x30/+0x31 (signed, with a draw below 4 and one below 50), runs 0x100c3870, and takes the club's coach or assistant manager post
  by the reputation against the player's halfword +0xa (0.5 then 0.75; the coach loop does not stop at the first empty slot: every empty one of the five gets him);
  then +0x2a, +0x28, +0x2c are trunc(min(cap, factor * max(pow(role byte, 2), halfword))) (the third compares 0.75 * pow), with factor/cap 0.9/7500 for a halfword
  +0xa above 7500, else 0.75/6000 after a draw below the signed byte +0x63 (none for 0) that is nonzero, 0.5/4500 when it is zero. The maximum and the capped
  product evaluate pow and the conversions again where the original does, so FPSCR is the original's; the scaled form of the draw below the byte is unreachable
  (a signed byte). r20 and r21 are mirrored across iterations (the date's address, then the byte and the draw). No stack reader reads anything these routines
  leave (season-day and same-save-continue, `--stack-audit-all` with the originals running).
- The module's routines are called as C++ with the registers the original's `bl` sets (`Run::direct`): 0x102e4530's first callee (0x102b8180) reads r4 as a leftover
  argument, so a direct call that left r4 alone faulted (found by the first creation case; the same call through the dispatcher passed because the checker
  compares the callee in isolation from the same registers). The constructor builds a Pascal name from two obfuscated pieces of the unit's data and asks for it
  (FSMakeFSSpec through the glue 0x1077fee0) until it is found, alerting each time; `Case::lookups` (player_regen_checks.hpp) answers that import per call (a refusal
  once, twice, an OSErr whose low halfword is zero but not the word). The scans keep the retirement entry on their frame (+0x44, its dates begun by 0x10715880 at
  +0x4c/+0x54) so the decisions see the original's addresses; the daily scan reads the caller's r19 (the decisions' job byte of a staff member with no job bytes).
  The destructor frees the four blocks that exist, the per-nation club lists (read count each time) and, for a positive halfword flag, the object.
- Checks: `regen_pending` 920 cases (the queue's date and id outcomes, creation for real, a window of the real people table [base cell and count rewritten] with one edited
  candidate: player record or none, club or none, the reputation thresholds, the answered human/busy/free routines, the swap draws, every role byte class against the
  caps of the three variants), `lifecycle` 301 (scans over windows of the first real people: age, link, id, class, queued, contract flag, retirements, the staff r19 family;
  the destructor on the real object and copies; the constructor with the file refused, each allocation refused, loading retire.dat from the in-memory archive). Findings 0;
  all 6 refill sites of 0x102dabb0 and every dispatcher callee site of both blocks paired (the module's routines called as C++ and the pow glue are not traced); every operand of every `if` of both sources seen both ways; 16 + 11 source controls, all found but one
  (the r21 mirror: callee-saved registers are not compared by a native-first trace, and no stack reader reads them), `--corrupt` and `--corrupt-draw`. Two constructor cases
  fault on both sides (allocation 1 and 3 refused in a new game write through the null block): `--null-page` runs them to the assertions (301 returned, the four assertion sites
  paired). CM_REGEN_PENDING_FULL=1 (every role byte, 6,144 ability cases): no difference. Live: season-day 1 call and same-save-continue 7 calls of 0x102dabb0 checked,
  0 differ; the pair replay MATCH=2.

## The font engine: T2K under the game's text (tools/font_engine_audit.py)

263 routines at 0x10751350..0x10768080 (22,909 instructions, fourteen compilation units) are the font scaler the game links:
Bitstream's T2K with its TrueType interpreter and autogridder, under FONT.CPP (the game's wrapper, not part of this audit) and
reached from the assertion box through graph_carbon.cpp. The audit pins the entries, ends and digests, the units (the pool
slots and source names AUTOGRID.C .. TSIMEM.C, link order; "FNT", "STYLE" and "UTIL" are three units without a string, named
by guess), the API (eleven entries FONT.CPP calls), the 127 entries only descriptors name and the three jump tables.
Block files: `native/font_engine_*.cpp` (`util`: the fixed-point helpers; `stream`: the input stream readers), the checker
`cm-font-engine-checks`, the brief for the next blocks in `.local/rewrites/font_engine/brief.md`.

- **Errors unwind by setjmp/longjmp**: every API entry runs `setjmp` (0x10773eb0) on the tsiMemObject's jump buffer at +0x10 and
  stores the code in its `errCode`; error sites call `longjmp` (0x10773f30) with a T2K error code (10009 = 0x2719 bad block, 0x2718
  allocation). Assertions are not errors: the game's reporter 0x107755c0 (text, file, line) returns and the engine goes on.
- **t2ktt_head and truetype_tables** (`native/font_engine_t2ktt_head.cpp`, `font_engine_truetype_tables.cpp`; 27 routines): the input stream's constructors and delete,
  the hint font (the TrueType interpreter's state: `new_hint_font`, `set_hint_size`, `load_glyph_program`, `delete_hint_font`; the last two sizes never run in the game),
  the cmap lookups, the table readers (cmap, kern, hhea, head, maxp, loca), the glyph reader and the directory lookups. The three API entries that take an error pointer call the
  generated setjmp from their C++ frame (r26-r31 and cr0 as the original holds them: the jump buffer keeps them and the checker compares it), catch the longjmp's
  `GuestContextTransfer` themselves and free the object's blocks; the checker's `font_engine_run_rewrite` runs inside a `GuestDispatchScope` so the transfer has an owner.
  The callers' reads of r9/r10 pass through chains `register_contract.py --after` does not follow: every entry is compared on r9/r10 and the routines whose original writes them
  (unrolled loops, pass and flag counters) leave them as it does.
- **Fixed-point helpers are exact transcriptions**: 16.16 multiply/divide from 16-bit halves of the magnitudes (INT_MIN keeps its sign
  through `neg`; `srawi` then mask is a logical shift; a divisor shifted in the divide loop shifts as signed). They are in
  `font_engine_util.cpp` with the registers they leave (callers' indirect calls can read r4-r8): `Leftovers`.
- **The checker's UI state needs r2**: a UI snapshot (`.local/benchmarks/ui/save-info`) is taken at an import boundary and has r2 = 0
  in its register file; the cases set the game's TOC (0x108bba90) as any call has it.
- **End to end** (`cm-font-engine-checks --world .../ui/save-info/initial.snapshot --end-to-end`, about a minute; `CM_FONT_ENGINE_EXHAUSTIVE=1`
  every style 0..0x1f and every size 6..72, minutes a file, `CM_FONT_ENGINE_E2E_FONT=n` one file): the game's own font code (FONT.CPP,
  generated in both runs) drives the engine over each font file of the snapshot with a fresh font record, renders every character
  0x20..0xff at each style and size through its glyph cache and every cmap code through the engine's character entry, logs each
  glyph's metrics and bitmap hash into guest memory, and the dirty pages (the engine's heap, the caches, the log) and the allocator
  tables are compared between a run with every hook declining and the production routing. Three font files ship (small.t2k and
  medium.t2k are byte-identical); the game requests cmd = 2 only, so AUTOGRID.C, GHINTS.C and HSTRAT97.C do not run in it.
- **TSIMEM.C** (`font_engine_tsimem.cpp`, 8 routines): the allocator over the game's malloc. Each routine builds the original's frame and sets the registers
  the original holds at every call out, because the callee's prologue saves them: 0x10767bb0 (`tsi_DeAllocMem`) leaves its saved r28-r31, link register and back
  chain where the game's printf reads them. Error sites are the original's own steps (`raise`: r3 = mem + 0x10, r4 = code, the stamp 0x5500aaff at mem + 0x174, then `c.call`
  of the generated longjmp at the routine's sp); the generated longjmp owner throws `GuestContextTransfer` to the dispatch scope of the generated API entry's setjmp, and
  `guest::Frame` runs no epilogue on the way. The checks answer the longjmp (stamp, arguments, following code compared) and an end-to-end **error sweep**
  (`CM_FONT_ENGINE_E2E_FAULTS=oom:1-185,header:1-185,...`) injects allocator failures and corrupted blocks at every allocation of a short run so the
  real transfers through the generated entries run on both sides. `font_engine_stack_exact.inc` lists the entries whose dead stack is compared byte for byte
  (`routine_check_dead_stack_exact`, off elsewhere). The stack contract itself is the checker's **leaver check**: at every entry
  into one of the five leavers (original run and C++ run, through the dispatcher or by a direct call) it records the stack pointer relative to the checked
  entry's, the link register and the registers the leaver's prologue saves, and compares the two sequences in order. `font_engine_stack_exact.inc` is only an extra,
  stricter comparison, kept for entries whose engine callees are all generated or listed.
- **GLYPH.C** (`font_engine_glyph.cpp`, 6 routines): the glyph record the outlines are built in (`glyph_record`): the compact point format of the .t2k files (0x1075b970),
  a glyph read from a stream (0x1075bc90: contours and points, or a composite's component list in a buffer that grows by half), the empty glyph, the merge of a component into
  a composite (0x1075c0f0) and the release (0x1075c8c0). 0x1075c830 (the point arrays) is a stack-contract leaver: its frame is built as the original's, and the stack audit
  (`cm-world-replay CAPTURE 1 --timing --stack-audit-all`; `CM_FONT_ENGINE_ONLY=0x..,0x..` runs only those routed entries as C++, to isolate a block in a replay) finds the same values at the printf readers.
- **TRUETYPE.C, the sfnt** (`font_engine_truetype_glyph.cpp`, 12 routines): the sfnt object (a font's tables over a stream), its creation (0x10766ab0) and deletion (0x10767190) with the setjmp
  the API entries run on the memory object (the C++ catches the allocator's longjmp transfer at the setjmp's return, as main_program does, then frees what the memory object holds; setjmp stores the
  condition register, so cr0 is set as the original's last compare left it), the table loader (0x10765810), the cmap lookups, the glyph by index (T2K compact or TrueType data) and the metrics.
  The checks run on the snapshot's three real fonts, one field of the sfnt or one byte of the font file changed per case, and the end-to-end error sweep runs both catch paths for real.
- **STYLE** (`font_engine_style.cpp`): the bold style the sfnt layer calls through two function pointers (advance widths and glyph outline) and the corner
  intersection the outline routine uses; the outline routine allocates and releases a copy through `tsi_AllocMem`/`tsi_DeAllocMem` at its own frame's sp with
  the original's r14-r31 (mirrored by its loop variables). Both pointer-called routines leave r3-r10 as the original on every path and are compared in full.
  **T2KEXTRA** (`font_engine_t2kextra.cpp`): the colour (sub-pixel) bitmap conversion for command bit 0x80, dormant for the game: cases only.
- **FNT, first third** (`font_engine_fnt_a.cpp`, 62 routines): the TrueType interpreter's entry (0x10756310: the execution context on its frame, the size checks, a setjmp the illegal-opcode handler and a bad opcode table longjmp to with
  0x1101 and 0x1102, the loop called through the context's `run` word), the two loops that fetch the opcode and call `table[opcode]` through the glue, the rounding, move, project and cvt functions the context and the graphic
  state point at, and the opcodes that set vectors, reference points and zone pointers, work the stack and shift or interpolate points (ISECT, MDAP, MIAP, IUP, SHP, SHC, SHZ). Handlers are called by pointer, so each is checked from tiny
  states built around a real hint font and by programs of opcodes run through the entry against the original; every routine with a frame builds it (SHP, SHC and SHZ read stack their helper may not have written, which reaches r8/r9 and the
  entry's r9/r10), and the entry sets cr0 as the original's last compare leaves it because setjmp returns the condition register in r6. The game runs only the entry and the plain loop (symbol.ttf's fpgm, at load): the rest is cases with
  100% line coverage (an instrumented copy of the library, `.local/rewrites/font_engine/fnt_a/coverage.sh`) and a source control per routine; the end-to-end run is the equivalence for what the game runs.
- **T2KSC.C, the scan converter** (`font_engine_t2ksc.cpp`, 9 routines, the stack leavers 0x107617b0, 0x10761d50 and 0x10762e30): the converter object (`scan::`, 0x2228 bytes, the
  allocator's scratch slot 4) holds sorted edge lists by pixel row (keyed by the x of each crossing) and, for the grey bitmap or the dropout controls, by pixel column; the outline's
  lines (0x107617b0) and quadratic curves flattened by halving (0x10761d50, its halves on the frame's own stack) put nodes {key, next} into them (chunks of nodes from the allocator),
  and 0x107623c0 fills the bitmap: 8-bit coverage (0x7e a full pixel, a weighted blend along y) or one bit a pixel with the two dropout passes. Every routine builds the original's frame and
  mirrors the original's non-volatile registers in a register array (`Regs`), set before each call that saves them, so the leaver check sees the original's r15-r31 / r29-r31 at each entry
  into a leaver; r9 and r10 are left as the original (the callers read them: `font_engine_volatile_read.inc`). The checks build the converter by running the ORIGINAL constructor inside each
  compared run (`Case::prepare`, new in the checker; the mutation of such a case is applied after the preparation).
- **FNT, the TrueType interpreter's opcode handlers** (`font_engine_fnt_b.cpp`: graphic state and point movement; `font_engine_fnt_c.cpp`: flow control, definitions, jump-table
  dispatchers, MDRP/MIRP, deltas, the opcode table filler 0x1075a500 and the state tables builder 0x1075a970; the loops and the stack/arithmetic opcodes are `fnt_a`). The
  interpreter's entry (0x10756310) builds an execution context on its stack (`interpreter::` in `font_engine_detail.hpp`) and its loops call `table[opcode]` through the
  indirect glue with r3 = the context; the context's function words (move, project, dual project, run, cvt scaling, single width) and the state's (round, scale callback) are
  descriptors called the same way, never switched on in place. Each handler keeps a shadow of r3-r10 (what it leaves goes to the next handler and, at the end, to the entry's
  callers) and is compared in full. Dormant for the game except FDEF (symbol.ttf's fpgm at load), the two builders and the operand skipper: the cases run on a real hint
  font's state in the scratch area (`font_engine_checks_fnt.hpp`), and the end-to-end sweep also runs synthetic programs through the real entry on the hint font it made of
  symbol.ttf (`font_engine_checks_fnt_e2e.cpp`, off with `CM_FONT_ENGINE_E2E_NO_INTERPRETER`). Not in `font_engine_stack_exact.inc`: the generated callees save the
  caller's non-volatile registers, which these handlers do not mirror (the per-reader contract leaves those bytes free).
- **T2K.C, the scaler object** (`font_engine_t2k_core.cpp`, 9 routines; `t2k_object::` in `font_engine_detail.hpp`): the object FONT.CPP keeps per font (0x5f8 bytes: the 2x2 matrix and its identity flag, the sizes and their 16.16 scales, the glyph cache record,
  the font's metrics rotated through the matrix, the render result) and the five API entries on it: `new_t2k` 0x1075e960, `set_transformation` 0x1075eef0 (size, unit-column matrix, metrics, the hinter's setup when asked), `purge` 0x1075f980,
  `render_character` 0x10760e30 (the game's: cmd = 2 for the characters 0x20..0xff) and `delete_t2k` 0x10760f50, each under the setjmp of the memory object with the longjmp's transfer caught at its return (cr0 as the original's last compare left it).
  `render_glyph` 0x1075fa10 is the renderer: the glyph by number or by character, a composite's components rendered one level down (at the composite's own depth: the original passes the caller's r29 before its increment) and merged by
  0x1075c0f0, the points scaled to 26.6, the origin rounded to the grid, the matrix applied, then the scan converter (still its own block) and the cache of rendered bitmaps. It keeps the original's r13-r31 in an array set before every call that can
  reach a leaver. The callers read r9 and r10 of every entry, so the caret routine and the component merge leave them as their originals do and are compared on them. The hinter (dormant for the game:
  cmd = 2 asks for none) is called as the original calls it; its hint data buffer on the stack is copied unwritten for a font without a T2KG table (the original copies the stack's leftovers: not reproducible, dormant). Cases run on the snapshot's three
  real scalers with poked fields, real longjmps through each entry's setjmp and the hinter's callees answered; the end-to-end run, its error sweep and nineteen source controls are in `.local/rewrites/font_engine/brief.md`.
- **GHINTS.C, the global hint data** (`font_engine_ghints.cpp`, 5 routines: the highest and lowest y of a glyph's contours 0x1075ac40/0x1075acd0, the T2KG reader 0x1075ad60, the median height of a letter list
  0x1075ae30 and the analysis that measures the 0x6c-byte data from the font's own glyphs 0x1075af60): the font code never asks for hinting (hint = 0, cmd = 2), so they run only under the hinted end-to-end
  mode (`cm-font-engine-checks --end-to-end-hinted`, test `font_engine_end_to_end_hinted`: the same original-against-production comparison with the hinter driven through `T2K_NewTransformation` with hint = 1 and the
  grid commands of `render_glyph`; symbol.ttf has no letters to measure, so the original's own size setup divides by zero there and the run compares up to the same fault). The leaves and the reader leave r3-r10 as the
  original; the median and the analysis are compared on r9/r10 (their glyph-reader callees leave r4-r8 their own way). The analysis's two sample arrays stay in the guest frame (with no sample at all the original
  reads their first entries as the stack left them). Cases: 1,641 (stale-stack medians, random letter lists, glyph limits, answered analyses with real result arrays), mutated runs and the call trace with no finding.
- **HSTRAT97.C, the hint strategy** (`font_engine_hstrat97.cpp`, all three routines, in production, dormant for the game): the heap sift 0x1075c980 and the score polynomial 0x1075ca60 (both exact in r3-r10, 9,260 cases) and the
  strategy 0x1075cbc0 (1,338 instructions): per axis the link edges and the extremum points of the glyph become 14-byte records (a block and an array of pointers from the memory object, heap-sorted by coordinate through the
  sift, grouped, merged, kept), then the stems are placed record by record through AUTOGRID.C's callbacks (seven by pointer through the indirect glue, the rest by `bl`), the bisection of the stem width restarting the records after
  the last bisected one when it moves. `Strategy` keeps the original's r13-r31 (`g`) and r3-r10 (`v`) as its instructions leave them: the callbacks read all of r3-r10 (the glue's targets are unknown) and the release leaver is
  called with r28-r31 as the loops left them. Cases: 1,144 on synthetic handles in the scratch area (callees answered, the bisection real, all real; empty and negative last index, point -1, pairs in one group, 60 large shapes,
  flags poked away at a callee's dispatch, a glue-target marker poke), `--mutate 4 --call-trace --argument-masks`: findings 0; branch coverage 332 branches, every outcome hit but the ones proved unreachable (comment in the
  source). The hinted end-to-end run (real glyphs of the two .t2k fonts, 772,240 strategy entries) is identical and 13 of 20 source controls make it differ.

`autogrid_a` (AUTOGRID.C's first half, 9 entries, in `native/font_engine_autogrid_a.cpp`; dormant for the game): the autogridder's height and stem
analysis (the size scaling of the font's heights and stems, the band map, the nearest height, the direction normalisation, the per-point directions, the
extrema, the inflections, the point classification and the edge partners) on the object `autogrid::` of the detail header. Every routine leaves r9 and
r10 as the original does (the chain up to the hint size entry passes them on); the cases start from objects the original engine builds for a real scaler
and real glyphs (`autogrid_a/` under `.local/rewrites/font_engine`), the hinted end-to-end mode runs all nine, and 118 one-expression source controls in a
private copy of the shared library found 102 changes (the 16 not found are equivalent, unobservable or an unreached boundary: see the brief).
- **AUTOGRID.C, second half** (`font_engine_autogrid_b.cpp`, 37 routines; `autogrid::` and `hint_input::` in the detail header): the hint handle's creation (nine allocations in
  the original's order), per-glyph run (`hint_glyph`: the glyph's words copied to the originals, the flavor/data assertions, then the run of the passes: rings, edge records
  sorted by priority through the game's byte move, the strategy or the stem widths) and deletion (nine `tsi_DeAllocMem` calls at its own frame's sp), and the small
  callbacks the strategy calls directly or through the glue (edge placement, stem-width snapping, bisection, interpolation of untouched points, standard widths). Dormant for the
  game (FONT.CPP passes hint = 0): the cases and the hinted end to end are the evidence (35 of the 37 entries are reached by it; each is sensitive to a source control there
  except the two `widest_stem` callbacks, whose result nothing reads). Registers: callbacks and leaves compared in full, the five entries that call other blocks' routines on
  r3, r9, r10. Cases' `answers` cannot stand in for a routine another block has routed (the original run declines the hook): the cases run those callees for real.
