# Tasks

The live work list. Claude keeps it current during a run; read this instead of
the scrollback. Remove ticked items once their commit is pushed.

## Blocked on Victor

- [ ] **Prune `.local`.** It holds 160 GB, mostly per-task folders from the old process; the disk is 94% full. Proposal: keep `.local/current`, `.local/exports`, `.local/ghidra`, the captured benchmark inputs and the reference save backups; delete the rest.
- [ ] **In-game check of the file fixes.** Save a career under a name containing "/" (e.g. "Cruzeiro 01/02"), quit, reopen, load it.

## Now

- [x] **Fused multiply-add (decided 2026-10-20).** One rounding at all 1,049 sites, in generated code and in the 41+ builder call sites of rewritten routines. Two 600-advance matches: state differs transiently after 4 and 1 advances, final state identical.
- [ ] Track 1 prototype: measure what better generated code and cheaper routine-to-routine calls gain (see profile notes below).

## Next: rewrite track

The end goal is every routine in C++ (see CLAUDE.md). Start where speed and rewriting overlap.

- [ ] Recruitment distribution and staff-link callees behind the 8-minute Continue: rewrite the hot generated callees (`0x1035e9f0` candidate scan, `0x101b1620` list search, `0x1014c2e0`) as real C++ called directly from their rewritten parents.
- [ ] Sorted insertion in add-link instead of append + full heapsort (`native/shortlist_links_storage.cpp`), now allowed: ~17–22 s per weekly Continue.
- [ ] Rebuild a runnable reference test set: only 40 of 288 tests run here.

## Next: speed

From the 2026-10-20 review. Estimates are unmeasured.

- [ ] Global-variable (TOC) reads miss the inline bounds check: `page_ends_` can't certify segment 1's partial first page (`native/aot_runtime.cpp:54-63`). 0.96 vs 0.36 ns per read, ~42k sites.
- [ ] `read_be`/`write_be` byte loops → `memcpy` + `std::byteswap` (−6% code on page `0x1071a`).
- [ ] Native sort: accept comparator `0x10309a00` (`tools/native_sort8.py`), ~6 s per weekly Continue.
- [ ] FPSCR bookkeeping is about half the statements on FP-heavy pages; emit one helper, or compute it lazily for its three readers.
- [ ] Scope chains from `thread_local` into `State` (`native/guest_context_transfer.cpp:7-31`), est. 10–15 s.
- [ ] Fuse the candidate-scan helpers (`0x101b1620` + iterator `0x10738e00`, `0x1036f590` at `0x1035f734`, `0x10362d70`), est. 8–15 s.
- [ ] Generator: per-block instruction accounting; switch cases only at entry points; direct calls to known cross-page targets.
- [ ] Re-profile the weekly Continue with a cheaper counter: the current probe inflates per-entry seconds and ~160 s is unattributed.

## Later: fixes from the review

- [ ] `divw` INT_MIN / -1 guard never fires (returns `0x80000000`); divide by zero throws where the original continues.
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
- World processing: no clean profile yet. The interest-pass replay harness spends most time cloning and comparing the 127 MB state; time only the pass.

## Known test state

- `tools/dev.sh test`: 40 runnable tests, all passing (2026-10-20). `match_engine_live_equivalence` and most others lack a built binary or inputs.
- Python audits: every `tools/test_*.py` passes against `.local/current/export`; `test_generated_temporaries.py` (needs a build dir) not run.
