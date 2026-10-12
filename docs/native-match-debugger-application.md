# Standalone match debugger application

## Delivered core

`CM Match Debugger.app` is a separate ARM64 AppKit application with bundle identifier `local.cm0102.match-debugger`. Its private engine process creates a fresh detailed match from two selected original club IDs and an explicit nonzero random seed. The interface opens paused and provides Create, Step, Run, Pause and Reset, a named player inspector, raw engine fields and an ordered list of observed state changes and control actions.

The catalog exposes the 10,593 original clubs. The factory validates that the selected clubs can field eleven actual eligible players; the original selector chooses the starting lineup. Names come from resolved first-name, surname and common-name pointers characterized in `100caeb0`. Actor indices and position slots are displayed as original zero-based values. The actor work queue can repeat players and is not a pitch-player count. Clock values remain raw engine units, including original event-related adjustments.

Run executes at most 30 calls per second to make inspection and manual pausing practical. The UI remains responsive because it never owns or concurrently accesses a running guest state. Each completed step uses the same reconstructed shared advancement entry as the production game. An original pause, completion or abort result stops scheduled execution. A fault preserves the partial state, reports the guest fault address, blocks further advancement and permits a fresh reset.

The application/session core is implemented and its actual worker is accepted; final corrected installed-UI verification awaits an unlocked Mac. The subsequent [injury withdrawal](native-match-debugger-withdrawal.md) adds original player withdrawal, actual on-pitch counts and before/after intervention logging. [Awarded goals](native-match-debugger-goals.md) now update original scorer/team records, related context and kickoff preparation, with a player goal column and before/after logging. **Lineup editing and red-card dismissal remain pending.** The engine-change list reports observations rather than a complete decoded commentary stream. The full debugger product contract and the whole match-engine reconstruction are not complete.

## Process and file ownership

The executable runs AppKit normally and a headless worker with `--engine-worker program game private-root`. Commands and responses use newline-delimited JSON over private pipes; there is no network service. Optional `requestID` values correlate control responses so an earlier running update cannot finish a later creation/reset request. Integer validation rejects Boolean, fractional, negative and out-of-range IDs or seeds, unexpected fields and unknown commands. Input is bounded to 64 KiB per pending command buffer.

Original program segments are packaged under `Contents/Resources/OriginalProgram`. The installed original game database is a read-only input. Career runtime and captured matches are not factory inputs. Each successful replacement releases the old state and removes its caller-owned experiment backing immediately. Failed creation retains the previous experiment. Normal worker exit or stdin EOF releases its backing; the GUI also removes its temporary root on exit or worker termination. Cleanup never targets installed game inputs or career files.

Configure `native/CMakeLists.txt` against an accepted complete generation with `CM_GENERATED_DIR`, `CM_MATCH_DEBUGGER_PROGRAM_DIR` and `CM_MATCH_DEBUGGER_GAME_DIR`, then build target `cm-match-debugger`. Packaging includes only `program.json` and the three original program segment files. No instructions database or saved career belongs in the app bundle.

## Original injury/history prerequisite

The first longer installed-UI run exposed an independent-bootstrap omission: Liverpool/Chelsea with seed 67890 completed 85 shared steps, then its 86th attempted call queried original injury/history state at `1069e7e4` through `1028a358`. The manager at TOC `-7b94` was absent. The error remained inspectable; the 86th attempt was not counted as a completed step.

The factory now follows the original sequence at `1034649c..103464c0`: allocate `0x18`, call `10646d90(object, 0)`, publish to TOC `-7b94`. Its six fields are initialized by that original constructor. It precedes weather and player-development services in their original construction order. The isolated world has an actual empty manager; injury queries are not bypassed. This remains an original dependency, not a newly reconstructed semantic entry.

The focused worker check executes 100 complete steps for that concrete seed, including the formerly failing query. The missing-manager fault is retained as a private negative diagnostic. Arbitrary clubs may expose further dependencies; full-match and all-club acceptance are not claimed.

## Validation

The focused process check covers Arsenal/Manchester United and Liverpool/Chelsea in distinct workers, named and unique actual starting elevens, productive shared C++23 calls, Run/Pause, stable paused inspection, strict rejected-command preservation, response correlation, original awarded-goal records and subsequent execution, seeded reset, independent state, immediate obsolete-backing removal and normal quit/EOF cleanup. Factory and existing session checks also pass. The affected worker, factory and session units are checked with UBSAN against the injury/history regression; other production dependencies are not newly instrumented.

Interactive checks exercise club selection, creation, Step and Run through AppKit. The initial Arsenal/Manchester United run produces actual engine score changes for both teams. The first installed Liverpool/Chelsea run exposed the injury/history omission described above. Corrected Pause/Reset behavior is confirmed through the actual worker protocol; its final installed-UI check is **pending because the Mac is locked**. Signed candidate/installed identities are recorded privately. Original database files, current career files and the installed game executable are verified unchanged. Save/Load is **not retested**; performance is **not benchmarked**. Scratch cleanup is part of this cycle.
