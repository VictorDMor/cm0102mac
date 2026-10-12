# Original injury withdrawal in the match debugger

## Behavior and scope

The standalone debugger exposes **Withdraw (injury)** for a selected actual player while the match is paused after kickoff. `Session::withdraw_injured_player(team, actor)` invokes the original complete injury-withdrawal operation at `101f1530(match, actor, 0)` through the production dispatcher. It updates original availability, withdrawal/time bookkeeping, tactical organization and related live actor records. The actor remains in the match record with original position sentinel `-3` and is excluded from the active on-pitch count.

This is injury withdrawal, not pre-kickoff lineup editing, substitution or red-card dismissal. Original quick-match callers use a nonzero argument after performing withdrawal bookkeeping; detailed injury callers use zero. The quick caller also reduces original condition and increments the withdrawal field before its nonzero call. The debugger uses the characterized detailed path rather than writing a position field or erasing a display row.

Actual on-pitch counts are separate from the actor work queue. Each accepted intervention records its number, completed-step boundary, team, actor, actual staff ID, position/count changes, before/after inspection and instruction count. The worker adds these effects to the ordered log. Interventions do not inflate the completed engine-step count. Reset creates a fresh seeded world and clears intervention history.

The UI disables withdrawal before kickoff, during running/faulted/completed states, without a selected row, and for an off-pitch actor. The session independently validates the boundary, side/index, staff reference, original team byte and owning-match pointer before any guest write. Invalid selections preserve the match, registers, accounting and history. A budget failure or interrupted call retains partial diagnostic effects, faults the session, restores its caller budget and blocks continuation until reset.

The subsequent [match-actor reconstruction](native-match-actor-reconstruction.md) converts the complete 203-instruction withdrawal body and both position accessors to C++23 in the production dispatcher, raising the semantic manifest to **604** with its 601-entry prefix preserved. Remaining football dependencies, substitution, card and goal lifecycles still require reconstruction. [Goal injection](native-match-debugger-goals.md) executes characterized original bookkeeping through the separate debugger bridge. [Pre-kickoff lineup editing](native-match-debugger-lineups.md) adds actual roster/player construction, position exchanges and benching. Red-card controls remain pending.

## Focused evidence

Two actual Arsenal/Manchester United worlds with seed 12345 compare the session API against direct original withdrawal calls. The control advances subsequent steps through the original shared body; the session advances through the C++23 shared body. Checks compare the complete `0xe0bc` match record, all register/unique bytes, return PC, heap allocation/free maps, exact instruction/import accounting from the initialized-match boundary, restored budgets and complete ordered map/clear/write effects.

- Outfield actor 8: 556,196 instructions, 60,978 ordered effects, home on-pitch count 11 → 10.
- Goalkeeper actor 0 after ten further steps: 494,857 instructions, 54,483 effects, home count 10 → 9.
- Away outfield actor 8: 617,911 instructions, 65,298 effects, away count 11 → 10.
- Ten subsequent original-agreeing steps after each withdrawal keep the actor unavailable.
- Position-only negative controls differ from the actual original effects.
- Pre-kickoff, running, invalid side/index, empty actor and repeated selections are rejected without effects.
- A 400,000-instruction intervention limit produces a partial fault without an accepted withdrawal record or a falsely completed step; another session remains unchanged.
- A fresh reset restores both starting elevens and clears both histories.

Independent startup has varying path/time/import costs, so accounting starts at each initialized-match boundary rather than comparing unrelated absolute bootstrap totals. Per-call counts and every subsequent compared call remain exact. The worker protocol also applies withdrawal, checks its before/after log and later steps, rejects repeated/invalid commands, and resets the experiment. Its second pairing retains the 100-step Liverpool/Chelsea injury/history regression.

Withdrawal, worker-protocol and existing session checks pass. UBSAN instruments the affected session and withdrawal-check units; output agrees exactly with Release and stderr is empty. Remaining dependencies are not newly instrumented. AppKit row/control wiring compiles, but final interactive verification remains pending while the Mac is locked. Installed game/database/career files are preserved, and owned backings and disposable binaries are cleaned after this cycle. Save/Load is **not retested**; performance is **not benchmarked**.
