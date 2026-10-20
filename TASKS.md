# Tasks

The live work list. Claude keeps it current during a run; read this instead of
the scrollback. Remove ticked items once their commit is pushed.

## Blocked on Victor

- [ ] **Fused multiply-add.** `fmadd`/`fmsub`/`fnmsub` (1,049 sites) are generated as a rounded multiply then an add (`tools/compile_recomp.py:476-478`); PowerPC rounds once. Emitting `std::fma` matches the original hardware but changes simulation results against every current reference, and the semantic replacements copy the double rounding. Decide whether to switch.
- [ ] **Instruction accounting in performance work.** May an optimization change instruction counts or leftover stack bytes while game state, RNG order and saves stay identical? Unlocks sorted insertion in add-link (~17–22 s per weekly Continue) and row-gate hoisting.
- [ ] **Prune `.local`.** It holds 160 GB, mostly per-task folders from the old process; the disk is 94% full. Proposal: keep `.local/current`, `.local/exports`, `.local/ghidra`, the captured benchmark inputs and the reference save backups; delete the rest.
- [ ] **In-game check of the file fixes.** Save a career under a name containing "/" (e.g. "Cruzeiro 01/02"), quit, reopen, load it.

## Now

- [x] New process: `CLAUDE.md`, `TASKS.md`, `tools/dev.sh`, `.local/current`
- [x] Installer installs the current build (it pointed at an Oct 11 build), refuses while the game runs, signs the app
- [x] Three jump-table targets never compiled (`0x101b4b88`, `0x101b4b24`, `0x107593d4`): added to `tools/discovery_roots.txt`; `tools/check_jump_tables.py` now fails generation on any missing target
- [x] Turbo pages dropped instruction charges from 17 semantic hooks; all hooks there now run inside `counter.semantic_boundary`
- [x] Removed `Run Native Prototype.command` (ran a diagnostic binary on the real Runtime folder)
- [x] Saves reach stable storage (`F_FULLFSYNC`) before the game deletes or renames in that folder
- [x] Mac '/' ↔ host ':' in file names; names MacRoman can't hold no longer end directory scans
- [x] Generate, build, test, install, commit, push

## Next: speed, same instruction stream

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
- [ ] Three jump tables don't match `check_jump_tables.py`'s pattern; inspect them by hand.
- [ ] 250 of 288 tests can't run here: their binaries aren't built or their inputs were deleted by earlier cleanups. Restore the ones that matter.

## Known test state

- `tools/dev.sh test`: 40 runnable tests, all passing (2026-10-20). `match_engine_live_equivalence` and most others lack a built binary or inputs.
- Python audits: every `tools/test_*.py` passes against `.local/current/export`; `test_generated_temporaries.py` (needs a build dir) not run.
