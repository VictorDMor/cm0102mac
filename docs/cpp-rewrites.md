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

Selection preserves database order: lower-division non-reserves before
reserves (one shared 70-candidate cap), second-division places 13–20 before
places <=12, lower-division additions up to 36, then first-division clubs.
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
