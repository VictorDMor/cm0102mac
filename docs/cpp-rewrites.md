# Rewriting original routines as plain C++

How a routine of the original becomes hand-written C++ that the game runs in
place of its generated body, and how it is checked. The tactics system
(`FORMATION.CPP`, `TACTICS.CPP`, `TACTICS_SCREENS.CPP`) is the first
subsystem done this way; `tools/tactics_audit.py` lists its 234 routines.

## What must stay identical

The rewrite may compute however it likes, but must leave everything the game
can observe exactly as the original does (CLAUDE.md):

- **Guest memory**, written through `guest::Cpu` (big-endian, checked; memory
  observers see each write). Order of writes does not matter, final values do.
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
order, and both sides get the recorded outputs. A routine is done
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
