# Tasks

The live work list. Claude keeps it current during a run; read this instead of
the scrollback. Remove ticked items once their commit is pushed.

## Blocked on Victor

- [x] In-game check of the file fixes: new save, overwrite and reload work (confirmed by Victor, 2026-10-21).

## Now

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
- [ ] Direct cross-page calls from generated code (skip two dispatcher round trips per call); page frames are 48–96 bytes, so nesting is cheap.
- [ ] FPSCR: the only reader is `setjmp` (0x10773ecc), which stores it into the jmp_buf; status bits are never used for a decision. Find a way to stop computing them that keeps jmp_buf memory identical.

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
- [ ] Idle at the start menu the app uses 100% of a core: the event loop polls `NativeUI::poll` continuously (sampled 2026-10-20; not checked against older builds).
- [ ] Three jump tables don't match `check_jump_tables.py`'s pattern; inspect them by hand.
- [ ] Single-precision multiply-add with non-single operands rounds to double, then single (`native/fused_multiply_add.hpp`); exact would need round-to-odd. Rare.
- [ ] Builder multiply-adds look up their kind per call (binary search over 1,049 sites); resolve per call site if it ever shows in a profile.
- [ ] 250 of 288 tests can't run here: their binaries aren't built or their inputs were deleted by earlier cleanups. Restore the ones that matter.

## Profile notes

- Match play (600 advances, Arsenal, 2026-10-20): ~12% generated pages; ~28% routine-to-routine plumbing (`run_until`, `dispatch_page`, dispatch/native scopes, thread-local lookups, hook entry, diagnostics checks); ~15% `SemanticGuestCall` instruction methods; `spatial_role::invoke` 12.6%; rewritten bodies ~11%; guest memory ~8%.
- World processing, interest pass (`cm-world-replay --profile`, 9,208 guest samples over the four captures, 2026-10-21): register club interest `0x10363f60` (builder-based owner) 91.5% inclusive / 39.1% self; generated callees `0x1014c2e0` 10.8%, `0x100f5770` 10.7%, `0x100c7d00` 8.4%, `0x100d9580` 4.4%; contract query `0x10082f90` 6.4%; assessment `0x10361490` 21.3% inclusive.

## Known test state

- `tools/dev.sh test`: 62 runnable tests, all passing (2026-10-21). `match_engine_live_equivalence` and most others lack a built binary or inputs.
- Python audits: every `tools/test_*.py` passes against `.local/current/export`; `test_generated_temporaries.py` (needs a build dir) not run.
