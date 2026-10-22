# Awarded goals in the match debugger

## In-game Debugging option

Game Options also exposes **Debugging**, off by default and stored separately
in `Debugging.plist`, with **Desligado / Ligado** choices. Changes take effect on
OK; Cancel discards them. Enabling it adds **Adicionar Gol** to the original match
sidebar. Its original-style dropdown lists
on-pitch players with their club and name. Choosing one immediately awards a goal
to that player's side. The button beneath starts disabled, then reads **Gol**
plus the most recently chosen player's common name or surname. Choosing another
player changes that shortcut; each click awards one more goal.

The shortcut resets when the original game initializes another match. Player
identity and eligibility are checked again when clicked, including substitutions,
withdrawals, stopped/completed matches and saturated counters. The operation runs
at the original match screen's action boundary, preserving the interrupted UI's
registers and stack while invoking the same characterized original goal regions
used by the standalone debugger. The ordinary goal announcement populates the
original scorer/minute history, and the normal report publisher refreshes the
visible scorer list before the original screen rebuild.
This intentionally changes the live career's match outcome and uses ordinary
goal bookkeeping; it does not edit only the scoreboard. A normal subsequent
career save retains the resulting game state. Original save layouts are unchanged.

The in-game regression check compares four interventions against original goal
execution, including consecutive clicks without advancing time. It reads the
original published report and verifies each scorer's staff identity, side and
nonzero minute, then compares 20 subsequent engine steps. Career save/reload after
an in-game intervention has not been retested.

The sections below record the earlier standalone-debugger characterization.

## Behavior and scope

**Add goal** applies an intentional awarded goal to a selected actual on-pitch player while paused after kickoff. The player inspector displays the original goal counter. Each accepted intervention logs the scorer's staff ID, side/actor, before/after score and scorer counter, raw clock, completed-step boundary and instruction count. It does not count as an engine advancement step. Reset clears the added-goal history and constructs the original seeded setup again.

The operation executes original goal bookkeeping against the owned live match: scorer statistics/rating, opposing goalkeeper records, related existing match context, team condition, commentary and kickoff preparation. It forces the awarded-goal outcome rather than asking a probabilistic shot to succeed. This is a debugging intervention, not a change to ordinary match decisions. It does not promise an unassisted goal: the original bookkeeping can consult existing assist context.

The helper in `native/match_debugger_goal.*` remains a bridge into characterized original regions. **The complete action routine is not reconstructed in C++23 by this change. The production semantic manifest remains 604.** The production game retains its ordinary action path; only the debugger explicitly invokes this intervention. No captured match, second football model, display-only score change or fixed-club world is used.

## Original regions and ABI

Natural detailed goals were observed at `102671c0` in `102576e0`, for actual Arsenal and Manchester United scorers. The action also contains chance, movement and cancellation paths. Calling the whole action with an extreme shot parameter was rejected during characterization: one later Liverpool/Chelsea scenario returned without a goal, and an early goalkeeper-shot scenario exhausted a subsequent step budget. That experimental route is not the delivered operation.

The accepted bridge reserves an independent `0x80` caller frame, places an empty action-context list at caller `+0x50`, and enters the original action with parameters `(actor, 0x16, 1, 0x1f86, 1, 0, context)`. Its original prologue owns the `0x1b0` callee frame and preserved integer/floating registers. Context placement avoids the original stack-argument stores at caller `+0x1f/+0x26` and saved-register area.

1. Execute original live setup through the completed minute/context write at `102578d8` (`match+0x77c2`). This initializes original tables, random-state references, scratch flags, frame and event-time context.
2. Resolve the live opposing goalkeeper through original `101f1e10(match, opposing side)` and retain its result in the original region's goalkeeper register.
3. Enter awarded-goal bookkeeping at `102626bc`. Execute the original scorer, goalkeeper, related-context and event effects through the completed context write at `10263614` (`match+0x3108`).
4. Run the original ordinary-goal announcement at `102636d8` through `10263700`. Its scorer and minute enter the original history consumed by the match report.
5. Enter original goal confirmation at `10267154`. This executes rating/condition changes, the real score store, original commentary and kickoff preparation, then returns through the original epilogue at `1026a190..1026a1b0`.

Two scoped write boundaries stop only after their certified original writes. They forward all map/clear/write effects to an existing observer and restore it on every exit. Normal return requires the expected reserved frames and original epilogue. The host restores its caller frame after that epilogue. A missing boundary, budget stop or other failure preserves the partial guest state, restores the caller budget, records no accepted goal, faults the session and blocks continuation until reset. This is acceptance of the bounded regions, not a return claim for the entire original action's ordinary path.

No host write increments a player or team goal counter. The original body increments scorer `+0xc`; confirmation writes the selected team's original score at `match+0xe03d/e073`, applies both teams' condition adjustment and sets the opposing kickoff/interruption state. Final validation requires exactly one selected-side goal, exactly one scorer-counter increment, the other score unchanged and the clock unchanged.

Before any guest write, the session rejects pre-kickoff, running/completed/aborted/faulted states, invalid side/index, absent/off-pitch players, mismatched original side/owner, negative or saturated original team score, and a saturated scorer goal counter. The original signed team score is limited to 127 and the unsigned scorer counter to 255. Worker commands additionally reject Boolean, fractional, negative and overflowing fields and unexpected keys.

## Focused evidence

Two independently created Arsenal/Manchester United worlds compare six interventions against direct execution of the characterized original regions. Scorers include home/away outfield players, both goalkeepers and a repeated scorer. Comparisons cover the complete `0xe0bc` match record, all register/unique bytes, return PC, allocation/free maps, caller budgets, instruction/import accounting from the initialized-match boundary and all ordered map/clear/write effects. Five subsequent original shared-engine steps agree with the C++23 shared-engine counterpart after each intervention, for 30 compared steps. A scoreboard-only negative control differs in every case.

Checks also cover rejected selections and counter limits, a forced instruction-budget stop immediately after the real score store, preserved partial fault state without an accepted goal or completed step, independence and seeded reset. Liverpool/Chelsea with seed 67890 accepts two additional interventions and subsequent steps. The actual worker protocol covers both pairings/processes, three awarded goals with player counters/logs and subsequent steps, strict invalid commands, withdrawal of a scorer and rejection of later goals for that withdrawn player, original injury/history advancement, reset and backing cleanup.

Release goal and worker checks pass. UBSAN instruments the affected session, goal bridge and goal harness; it checks the same live operations and original comparisons. Other original/production dependencies are not newly instrumented. Final interactive verification of the installed **Add goal** button remains pending while the Mac is locked. Signed candidate/installed identities and original-region fingerprints are retained privately. Disposable probes, binaries and experiment backings are removed after this cycle. Save/Load is **not retested**; performance is **not benchmarked**.
