# Card controls in the standalone match debugger

## Behavior

Pause after kickoff, select a player, choose **Yellow card** or **Red card**, then
use **Add card** or **Remove card**. Yellow and red counts appear beside each
player, including substitutes, and interventions appear in the football event
log. A second yellow dismisses the player. Canceling that yellow cancels the
dismissal; canceling its resulting red leaves one yellow. Canceling a direct red
preserves any first yellow. Additions require an active on-pitch player during
active play; removals require a corresponding existing card.

Removal is an explicit debugging operation. It edits live discipline fields and
returns a dismissed player through original pitch-list and tactical operations.
It preserves the current clock, score and intervening play. The underlying foul
and rating effects are retained. **Back** restores the complete earlier state;
**Next moment** restores its recorded future. Card records, eligible return
positions and events follow the same branch/seek/reset boundaries as the match.
There is no display-only card overlay or disk-based checkpoint history.

## Original operations and corrected classification

Actor `+0x24` is the yellow count, `+0x23` the dismissal/red count, and `+0x25`
and `+0x26` the card reasons. Original dismissal at `101f1530` stamps `+0x21`
and sets position `-3`, removes cell/marking membership and reorganizes tactics.
The former debugger **Injury** control incorrectly used this dismissal method
and mislabeled `-3` as injured. This delivery corrects that classification.
The original pending-injury processor `101f6c60` now receives injury flags
`+0x1c/+0x1d`; it records injury time `+0x22`, clears the pending flag, removes
the injured actor and sets `-2` (or `-1` in the original keeper-tactics branch).
It does not increment the red count. Injury replacement remains unrestricted.
The semantic match-actor entry's descriptive name is corrected to `dismiss_player`;
its original body and ABI are unchanged.

| Operation | Original entry/end | Instructions | Source fingerprint |
|---|---|---:|---|
| Foul/discipline outcome | `10221e00..10222290` | 292 | `76d8fed64694d71aeb8750500fe29d0950645c18c50bb3c5caa96cd46875e8a1` |
| Dismissal | `101f1530..101f185c` | 203 | `a0cfcad85bee03beab0f9c951255b35730f6fa155105c04ef98b6dc4b6b6c276` |
| Pending injury | `101f6c60..101f70f8` | 294 | `c1e7dee3c842400399bf34608d9c57cb5ef86c7bb077a5199ec6d349e2f49be7` |
| Pitch re-entry | `101f1880..101f192c` | 43 | `a3b1ea80d1ecdca5e5593bd0df28496d1059e96dd9df2c266702e4c7181c4b49` |

The debugger executes the complete original discipline method with a bounded
original caller frame, decision/output storage and actual opposing actor.
Severity 6 exceeds every yellow threshold (0..5), but no direct-red threshold
(7..21); severity 127 exceeds every direct-red threshold. Original RNG, rating,
foul statistics and reasons execute normally. Original commentary commits the
card event, followed by original dismissal for direct red or second yellow.
This is forced debugger input, not a change to gameplay's foul/card policy.
The broad foul engine is not newly reconstructed in this delivery.

Re-entry calls the complete original list insertion, position and tactical
methods. The last active role is remembered independently of the current
position sentinel and checkpointed. If original keeper replacement occupied
that role, its occupant moves to a vacant role before the returning keeper is
registered. Invalid return configurations are rejected before mutation.
Underlying unavailable/substituted configurations are not silently healed.
The strict worker protocol accepts only side/index, yellow/red and add/remove,
with typed request IDs. Unknown fields, malformed values and unavailable edits
are rejected without effects. Interrupted interventions retain partial state,
restore the caller budget and fault the session until history recovery/reset.

## Verification and delivery

Focused real-database tests pass for yellow, direct red, second yellow, card
removal, return of outfield players and a goalkeeper, actual pitch-cell
membership, later engine calls, score/time preservation, invalid controls and
complete undo/redo/branch/seek. First-yellow ABI/memory/accounting agrees with
an independent direct invocation of the complete original methods. Existing
guest yellow fields are visible and editable without an injected-card record.

Corrected injury checks compare complete match/register/temporary state, ordered
writes and accounting against direct original injury processing, including
home/away/outfield/keeper cases, 30 following steps, invalid edits and partial
budget faults. The related lab and final-time checks pass. Worker checks cover
the same controls, strict malformed-command rejection, visible card counts and
events, subsequent productive movement, rewind, and normal quit/EOF cleanup.
UBSAN instruments the session, card bridge and card harness with empty stderr;
unchanged archive dependencies are not newly instrumented.

The updated signed ARM64 debugger is installed at
`/Users/victordmor/Applications/CM Match Debugger.app`. Both applications share
the [complete movement reconstruction](native-player-movement-reconstruction.md).
The full debugger/engine port remains in progress. Complete context-specific
commentary decoding remains unfinished. Save/Load is **not retested**;
performance is **not benchmarked**.

Installed AppKit acceptance observes the original game menu in its 800×600
content window and a normal original quit. The debugger creates a real match,
shows 00:05, keeps Thierry Henry selected through yellow/second-yellow/direct-red
additions and cancellations, shows the changed counts and on-pitch/sent-off
status, and emits one original card event per addition. Back/Next moment restores
the dismissal and its cancellation. Both owned GUI sessions exit normally;
the debugger worker and its private root are removed. Installed-process
executable hashes match the candidates. These observations are UI acceptance;
entry coverage comes from the separate component and actual-production checks.

The accepted cycle removes 29 validated owned disposable artifacts and recovers
1.93 GB. Reusable production objects/libraries, generated sources, current apps,
previous signed references and private source/runtime evidence remain available.
All 845 protected files and current candidate/installed identities are rechecked
following cleanup. The full-port goal remains active.
