# Tasks

The live work list. Claude keeps it current during a run; read this instead of
the scrollback. Remove ticked items once their commit is pushed.

## Blocked on Victor

- [x] **Uninitialised stack decides results** (found 2026-10-23). Decided by Victor 2026-10-23: option A, keep exact results; every rewrite reproduces the stack image the original leaves, checked byte for byte. Architectural changes that give up stack fidelity wait until the recompilation is complete.

- [x] In-game check of the file fixes: new save, overwrite and reload work (confirmed by Victor, 2026-10-21).

## Now

- [x] Debugging Best XI: select available starters for the current formation and reserves within the competition's bench limit. Localize Best XI, Add Goal, repeat-goal and Debugging On/Off controls in the 11 original languages. Actual-database checks cover gating, availability, formation changes, bench limits and depleted/empty squads; isolated Reading UI verified with Debugging off and in Portuguese/English. Career save/reload and performance not retested.

- [x] **Fused multiply-add (decided 2026-10-20).** One rounding at all 1,049 sites, in generated code and in the 41+ builder call sites of rewritten routines. Two 600-advance matches: state differs transiently after 4 and 1 advances, final state identical.
- [x] Track 1, first measurements (2026-10-20, 600-advance pairs): big-endian accessors as one load/store + byte swap: **5.3% / 8.3% faster** (Arsenal / Cruzeiro), identical final state; kept. Partial-page bounds entries for globals: 1.6% / 0.3% *slower* in match play; reverted.
- [x] Plumbing (2026-10-21): scope chains per State (3.6%), inline PC-profile flag (1.5%). With byte-swap: **13.9% / 13.5% faster** match intervals than before today's speed work.
- [ ] Next plumbing: per-call diagnostics accessors (`spatial_role_diagnostics` ~2%), what dominates `spatial_role::invoke` (~12%), owner→generated calls re-entering `run_until` (track 2 removes these).
- [x] World replays work again (2026-10-21): `cm-world-replay` runs pure imports (pow, BlockMoveData, NewPtr…) live and replays OS imports from the transcript; the import check compares only registers the import reads or its caller keeps. Interest pass ×4, daily transition, attribute world, database and daily sort reach their recorded end states and are ctest checks (`world_replay_*`). `background-fixture` is invalid since single rounding: its match now calls an OS import the capture never recorded (match play is covered by the 677/2262 intervals).
- [x] Profiled world processing (guest profiler, 2026-10-21); see Profile notes. The time is spread thin and dominated by mechanism: per-instruction accounting, dispatcher round trips, builder methods, slow global bounds checks.
- [x] Add-link keeps its link indexes sorted by insertion instead of append + full heapsort, when that provably equals the sort (see commit).
- [x] **Block accounting** in generated pages, fused sort and lowered leaves: one step per straight-line run. Measured: attribute world 28.9% faster, interest passes 7.6–9.5%, daily transition 8.4%, match intervals 4.5%/2.9%.
- [ ] Builder-based owners still step every instruction (`SemanticGuestCall::step`); they dominate the interest pass, where removing all accounting was worth 16–27%. Cheaper builder steps, or real C++ rewrites of the hot owners.
- [x] ~~Bridge the loader padding in the fast bounds table~~ — rejected 2026-10-21. It does remove the slow path for the whole TOC page (interest pass 1: 3.65M slow checks → 5.5k), but paired timing showed no gain (world −2.2…+2.9%, match +1.2%/+0.3%, mixed pairs): a slow check costs about a nanosecond, and the host profile overstated it.
- [x] ~~Force-inline `Memory::write`/`check_scalar` in generated pages~~ — rejected 2026-10-21: binary +16%, world ±1% (attribute +3.4%), match 1.7%/2.4% slower.
- [x] Inline leaf routines at their call sites in generated code (197 leaves): attribute world 12.1% faster, match 2.2%/3.9%, recruitment flat.
- [x] Owner→leaf calls run directly in `run_until` (`run_leaf`): match 5.4%/5.1% faster, world neutral.
- [ ] Production/transfer checks of other owner families with leaf entries (marking_pressure, actor_order, contract_terms/review, stdlib_core, utility_object, actor_effects, …; not built here) expect their hook to run when `run_until` enters a leaf, like the spatial-role checks did; use `is_leaf_entry` as those were updated.
- [x] Production pages call C++ owners in place (693 owners, ~37,000 call sites): `bl X` runs the owner's hook and continues at the return address unless the innermost dispatcher stops there. Day replay 23.43 -> 22.81 s (-2.6%, 5/5 pairs). Owners whose hook counts before declining (string_core, file_path, ...) keep the dispatcher.
- [x] Builder routines count instructions in batches when nothing watches them (f493033): day replay -2.1%. Routines with a host-watched PC keep stepping.
- [x] Player contract lookups in plain C++ (438e283): 17.0M first-contract, 4.6M club and 1.3M second-contract lookups per game day; -0.3..-1.2%. The club lookup declines to the builder when it needs the club resolver (1.0M/day).
- [x] ~~Force-inline the builder's instruction methods~~ — rejected 2026-10-22: day replay 1.6% slower (5/5 pairs), binary +8 MB.
- [ ] FP status bookkeeping: 210M updates per game day in generated pages (~25 byte operations each). Dropping it is not an option: the flags reach setjmp's jmp_buf (the only `mffs`, 0x10773ecc) and from there game memory and FP registers, and the replay diverged. An exact cheaper helper might save ~2-3%.
- [x] Registration shortcut: inactive recruitment entries return at once, writing the original's stack image (2.8%); rejected shortlist rows skipped natively (0.6%). Club control `0x1014c2e0` in C++ (`native/club_control.cpp`), checked against the original for every club.
- [x] Globals and TOC on the fast bounds path: a block where a mapping starts part-way has a second entry (the data segment starts in the block where the code ends). Day replay -1.4% (6/6). The 2026-10-21 attempts (bridging the gap, a begin offset) had measured nothing.
- [x] Builder transcriptions are slower than the generated code they replace: with production hooks off for actor_attributes, contract_terms, shortlist_assessment, shortlist_policy and transfer_offer (`GENERATED_FASTER_FAMILIES`), day replay 21.56 -> 20.19 s (-6.4%, 6/6). Owners stay, with their checks, as specifications. Screening the other families in original mode on the day was confounded by machine drift; on the week, original mode took the actor families 255 -> 247 s and 15 other match families 255 -> 238 s; all 23 now run generated code too (day 20.34 -> 19.16 s). A word-level FPSCR summary helper measured 2% slower than the generated byte chain and was dropped. `player_decision` in original mode diverges at an import (r19), so its owner and original differ somewhere: investigate.
- [ ] **The original reads uninitialised stack** (136 sites/day, `cm-world-replay --stack-audit`; see docs/aot-port.md). Rewrites that skip calls must reproduce the stack image those calls leave, or prove nothing reads it. This bounds every deeper rewrite; a definedness tracker (which bytes of game memory come from leftovers) would let checks ignore pure padding instead.
- [ ] Recruitment is ~53% of the 26-country day (guest profile): daily interest registration `0x10079050` -> `0x10363ca0` -> `0x10363f60` 24%, candidate scans `0x1035e9f0` from shortlist links 17% and `0x10359020` 9%, plus 4% from `0x1011c330`. Registration runs per (distributed player x club in order): per day 261 players x ~6,700 clubs = 1.76M `register_club_interest` calls, 76% stop at the club's inactive recruitment entry, 16.9M shortlist rows scanned, 477K rows available, 326K evaluated (two assessments each), 255 registrations.
- [ ] Direct cross-page calls between generated routines (skip two dispatcher round trips per call); page frames are 48–96 bytes, so nesting is cheap.
- [ ] FPSCR: the only reader is `setjmp` (0x10773ecc), which stores it into the jmp_buf; status bits are never used for a decision. Find a way to stop computing them that keeps jmp_buf memory identical.
- [x] **All-leagues benchmarks (2026-10-22).** Captures of the 26-country career in format 2: the 21->28 Dec week in `.local/performance/week-capture/SeasonFixture` (937 MB, ~300 s per replay) and 21->22 Dec in `.local/performance/day-capture/SeasonFixture` (91 MB, ~23 s). `cm-world-replay <capture> 1 --timing` replays to the recorded end state; use the day for quick pairs and the week for headline numbers. Real-game Continue times vary run to run (the game's results differ), so time with replays and find costs with `profile start/stop` through the driver.
- [x] Host overhead while the game works (732b5c0): cached snapshot env check (was 29% of samples), event polls (~20% -> 0.2%), import names as views.
- [x] List search `0x101b1620` in C++ (bc4abba): week replay -9.9% / -9.4%.
- [x] Fast paths ran only in benchmarks: the app's drawing observer made them step aside (b6c121e). Real-game week now ~290 s (two runs), against 350-372 s for the previous build's recent runs; not a paired measurement.
- [ ] Real-game profile now (current build, 2026-10-22): match simulation 32% inclusive; candidate scan `0x1035e9f0` 30% inclusive / 6.3% self; system calls 11%, of which file-system path work ~7.4% (`FSMakeFSSpec` 2.2%, `FSGetCatalogInfo` 1.4%, `FSOpenFork` 1.2%, `ResolveAliasFile` 1.1%); actor role wrapper `0x10289560` 4.5% self; list search 3.8% self (linear scans).
- [ ] File opens during processing: `0x10308350` -> `0x10748ce0` -> `0x10739de0` -> stdio open (`0x107768f0`/`0x10776940`/`0x10776da0`) resolves the same host path again on each open. Cache path resolution in the bridge (`allowed()` canonicalizes per component; `directory_id()` stats every known directory), invalidated on create/delete/rename/copy-on-write. Needs a save and reload on a career copy.
- [x] The dispatcher asks import_name only for PCs inside the import stubs (generated bounds): day replay -2.4% (5/5).
- [ ] Import dispatch: resolve each import stub to a handler once and switch on it (names are views now).
- [x] Capture format 2 (6458137): week 34 GB -> 937 MB, replay 563 s -> ~300 s.
- [x] Idle screens no longer spin a core (d4b29a1): start screen 99% -> ~5% CPU.
- [ ] Real-game runs differ (the game seeds randomness from the clock at startup). A fixed-clock validation mode would make real-game timing and save comparisons deterministic.

## Done: the tactics system in C++23 (goal set 2026-10-23, all 234 hooked)

Finish line: every routine of the original's `FORMATION.CPP`, `TACTICS.CPP` and
`TACTICS_SCREENS.CPP` (234 routines, 43,152 instructions; `tools/tactics_audit.py`)
runs as hand-written C++ in production, each checked against the generated
reference on captured calls and by end-to-end replays, suite passing, one
reviewed commit per block. Method: docs/cpp-rewrites.md.

- `FORMATION.CPP` 0x10110c80-0x10118ec0 (47): formation record (0x18e0 bytes),
  positions, zones, shape code, staff slots, instruction bits (+0x1888), `.tct`
  files. Called by the match engine, recruitment (0x10117490, 29 callers), screens.
- `TACTICS.CPP` 0x103acf80-0x103b3830 (30): CTACTICS, per-team records (0x94
  bytes at +0x5c4), set pieces, captain, presets, `tactics.dat`, AI choices.
- `TACTICS_SCREENS.CPP` 0x103b3830-0x103cf5f0 (157): the tactics, team
  instructions and penalty-taker screens.
- The 26-country day reaches 33 routines (`.local/tactics/census-day.txt`); the
  rest need session captures of the screens and file dialogs.

- [x] Inventory and pins: `tools/tactics_audit.py`, `native/tactics_routines*.inc`
      (TACTICS_ROUTINE hooked, TACTICS_DRAFT check tools only).
- [x] Guest ABI layer (`native/guest_abi.hpp`), `tools/call_registers.py`.
- [x] Fork-based routine checker (`cm-world-replay --check-routines`, `--only`,
      `--census`, `--strict-volatile`, `--synthetic`), with a negative control test.
      Exit rule: r4-r10 and every CR field as the original (they leak through
      later varargs spills and `mfcr`).
- [x] Session captures (automation `capture start/stop`, `drag FROM TO`): 13 UI
      sessions in `.local/benchmarks/ui` (game-save2 reaches the career save's
      tactics write; game-save stops before it), all replaying to their end state.
- [x] All 234 routines hooked (TACTICS_ROUTINE), every captured call equal to
      the original on 14 captures plus 845 synthetic calls; reviewed per group.
      The tactics hook replaces the builder hooks at 0x10113e10, 0x10117360,
      0x10117490. Save and reload on a career copy with the C++ save: OK.
- [ ] Not replayed (reviewed against the listings only): assertion paths (the
      error dialog's imports aren't in transcripts), national-team paths, the
      handler branches listed in 2a9e3a7, construct's AI-club path, old-save
      conversion, the preset dialog callback 0x10118210 (only with the presets
      flag at TOC -0x6330 set; same code as 0x10118180 apart from r6).
- [ ] Cleanups: one shared tactics helper header (fail/in_range/compare and the
      FPSCR model repeated in 4-5 files); direct C++ calls between the rewritten
      routines instead of `c.call` through the dispatcher (the performance step).

## Next: native lowering (decided 2026-10-22)

Victor's target: identical in-game results (same RNG seeds and draws) and the highest speed possible, well beyond 10x; the code may differ completely. The generated pages emulate the PowerPC: registers live in `State::registers`, a byte array the compiler must reload after every guest-memory store (both are `uint8_t`, so they may alias); every access is bounds-checked; every run is counted; every call goes through `run_until`/`dispatch_page`. Lowering each routine to a C++ function removes that:

- [x] ~~Registers as C++ locals inside a routine~~ — tried 2026-10-22 and not kept. 51 of the 60 hottest generated routines (34% of the week's self time) lowered to functions with registers in locals, rewritten from their page code: identical end states and instruction counts on every capture, but only ~1% faster (day capture, 5/5 pairs: 23.33 -> 23.08 s). Generated code already runs ~750M guest instructions/s (~1.3 ns each); register traffic is not the cost. Review also found that `run_until(s, return)` stops on a PC match alone, so a recursive re-entry that falls back to page mode can return to the outer lowered frame early; a lowering needs a frame check and should render from the IR, not reparse page C++. Sources kept outside the repo.
- [ ] **Where the time is (day capture, native sample):** the game's own work (billions of guest instructions per game day), builder-based owners stepping per instruction (`SemanticGuestCall::load/store/multiple`, ~10%), dispatcher round trips (`run_until`/`dispatch_page`/`run_leaf`, ~8%). The 10x path is algorithmic: rewrite hot subsystems so they execute fewer instructions for the same reads, writes and RNG draws (the list search did this: -10% of the week), starting with the recruitment candidate scan (30% of the week: own loop 6.3%, list search 3.8%, assessment 3.5%, `0x101ad080` 2.8%, contract terms 2.2%, transfer offer 2.2%).
- [ ] Condition and XER/FPSCR bits computed only where something reads them (liveness in the generator).
- [ ] Direct C++ calls between lowered routines (no dispatcher); indirect calls through one table.
- [ ] No instruction accounting in production builds (counts are diagnostics); keep it for checks.
- [ ] Guest memory: hardware guard pages instead of software bounds checks where a fault is a bug anyway.
- [ ] Then the hand-written C++ subsystems call each other directly (named types), replacing lowered routines by measured cost.

## Next: rewrite track

The end goal is every routine in C++ (see CLAUDE.md). Start where speed and rewriting overlap.

- [ ] Recruitment distribution and staff-link callees behind the 8-minute Continue: rewrite the hot generated callees (`0x1035e9f0` candidate scan, `0x101b1620` list search, `0x1014c2e0`) as real C++ called directly from their rewritten parents.
- [ ] Sorted insertion in add-link instead of append + full heapsort (`native/shortlist_links_storage.cpp`), now allowed: ~17–22 s per weekly Continue.
- [ ] Rebuild a runnable reference test set: only 63 of 298 tests run here. The old component tests (e.g. `shortlist_links_component_equivalence`, not built here) require identical dependency calls, instruction counts and budget cuts — the rule replaced on 2026-10-20 — so they fail on add-link's skipped sort and will fail on block accounting. Rework them to compare what the game observes (as `cm-world-replay` and `cm-shortlist-links-insert-checks` do).

## Next: speed

From the 2026-10-20 review. Estimates are unmeasured.

- [ ] Global-variable (TOC) reads miss the inline bounds check (`native/aot_runtime.cpp:54-63`). A begin-offset entry was slower in match play (extra compare on every access); re-test on world processing, where generated code dominates.
- [ ] Native sort: accept comparator `0x10309a00` (`tools/native_sort8.py`), ~6 s per weekly Continue.
- [ ] FPSCR bookkeeping is about half the statements on FP-heavy pages; emit one helper, or compute it lazily for its three readers.
- [ ] Fuse the candidate-scan helpers (`0x101b1620` + iterator `0x10738e00`, `0x1036f590` at `0x1035f734`, `0x10362d70`), est. 8–15 s.
- [ ] Generator: per-block instruction accounting; switch cases only at entry points; direct calls to known cross-page targets.
- [ ] Re-profile the weekly Continue with a cheaper counter: the current probe inflates per-entry seconds and ~160 s is unattributed.

## Later: fixes from the review

- [ ] `divw` INT_MIN / -1 guard never fires (returns `0x80000000`). Divide by zero now continues at the 1,385 quotient-free sites; the other 446 still stop (what the hardware returns there is undecided).
- [ ] `lfs` quietens signalling NaNs.
- [ ] Unknown intrinsics compile and throw at runtime; fail generation instead.
- [ ] `native_record_fields` promotion accepts calls that read `State` (latent).
- [ ] Turbo stores speed 0 in a manager record; check whether it reaches saves.
- [ ] An audio output device change stalls sound channels.
- [ ] Copy-on-write of an original data file isn't atomic (`native/platform.cpp`, `writable_file`).
- [ ] The installer writes the Game manifest after moving Game into place.
- [ ] Names over 31 characters can overflow the game's 64-byte name buffer (`0x1076ec60`).
- [ ] No App Nap / idle-sleep assertion during long processing.
- [ ] Run logs accumulate in `Runtime/Runs`; input tracing is on by default.
- [ ] Mouse-moved events pile up while the game is busy; every small `CopyBits` redraws a full frame.
- [ ] Idle at the start menu the app uses 100% of a core: the original main loop calls `WaitNextEvent` with sleep 0 (~10,000 calls/s; confirmed 2026-10-21 with an import trace). Blocking a millisecond when such a poll finds no event would idle the core; it changes only wall-clock readings.
- [ ] Three jump tables don't match `check_jump_tables.py`'s pattern; inspect them by hand.
- [ ] Single-precision multiply-add with non-single operands rounds to double, then single (`native/fused_multiply_add.hpp`); exact would need round-to-odd. Rare.
- [ ] Builder multiply-adds look up their kind per call (binary search over 1,049 sites); resolve per call site if it ever shows in a profile.
- [ ] 250 of 288 tests can't run here: their binaries aren't built or their inputs were deleted by earlier cleanups. Restore the ones that matter.

## Profile notes

- Match play (600 advances, Arsenal, 2026-10-20): ~12% generated pages; ~28% routine-to-routine plumbing (`run_until`, `dispatch_page`, dispatch/native scopes, thread-local lookups, hook entry, diagnostics checks); ~15% `SemanticGuestCall` instruction methods; `spatial_role::invoke` 12.6%; rewritten bodies ~11%; guest memory ~8%.
- World processing, interest pass (`cm-world-replay --profile`, 9,208 guest samples over the four captures, 2026-10-21): register club interest `0x10363f60` (builder-based owner) 91.5% inclusive / 39.1% self; generated callees `0x1014c2e0` 10.8%, `0x100f5770` 10.7%, `0x100c7d00` 8.4%, `0x100d9580` 4.4%; contract query `0x10082f90` 6.4%; assessment `0x10361490` 21.3% inclusive.

## Known test state

- `tools/dev.sh test`: 94/95 runnable tests pass (2026-10-23, Best XI/localization build); `debugging_best_xi`, actual goals and injury shortcuts pass. Known failure: `match_debugger_actual_lineup` since ed7749e (it requires the `match_actor` owner's call count, and production pages now run that family's generated code); fails with and without the tactics rewrites, and again in an isolated rerun. `match_engine_live_equivalence` and most others lack a built binary or inputs.
- Python audits: every `tools/test_*.py` passes against `.local/current/export`; `test_generated_temporaries.py` (needs a build dir) not run.

## Euro symbol (2026-10-23)

- [x] Locate the original E currency prefix and both native font paths.
- [x] Render the euro prefix as € with unchanged guest text and layout; both font paths, other currencies and 1x/2x/4x raster checks pass. Built and installed.
