# Debugging: Best XI

With Debugging enabled, the managed club's squad screen has a **Lineup**
(**Escalação**) drop-down between Sort and Clear Team. It is built like the Debugging
drop-down: a cell that owns an original popup menu, with three items, each
replacing the match selection immediately for the currently selected formation
(none changes the formation or its player instructions):

- **Best XI** (**Melhor 11**): the selection described below, over every available player.
- **Best U23** (**Melhor Sub-23**): players aged 23 or less on the game date (23 years
  364 days is in, 24 is out) first. Age is whole years from the original age
  operation (`10717dd0`: result slot, today's date at TOC `-0x7b80`, birth date
  at person `+34` day, `+36` year, `+38`). A young player takes a slot only if
  `best_xi::compatible` holds: no keeper/outfield specialist mismatch (the 5000
  penalty case) and a rating of at least 10 of 20 for the role's line, without
  and with the ball. Slots still empty go to the best compatible older players
  (same assignment, restricted to compatible pairs); slots still empty then go
  to whoever real is left by plain best fit, young before older. The bench is
  young first by the usual bench logic, then older players for the free places.
  A slot stays empty only when no player is left.

None of the three selections ever takes one of the game's stand-ins for a short
squad (the grey rows of the squad list), as a starter or on the bench. They are
the people numbered at or past the staff table's count (TOC `-7be4`): the
original status operation tells them apart the same way (`10060d28`, where they
skip the contract lookup), and the existing Recover Players and Play for Free
actions already skip them by that test. Their places go to real players by the
usual best fit; a slot stays empty only when no real player is left.
- **B Team** (**Time B**): the Best XI's eleven starters are computed as above and
  removed from the pool; the best eleven of the rest are selected, and the bench
  comes from the players left after both elevens.

The player-priority view and other clubs do not offer these actions.

The original squad status operation (`10060c00`) excludes ineligible,
unregistered, absent, injured, recovering, suspended and internationally absent
players from the club's 50-entry roster.

Version 2 compares every available player for every formation slot. The original
Mac profile getter (`100d2330`) decodes technical attributes using current ability
and the original goalkeeper exceptions. Physical attributes use the same profile
scale; the selection policy bounds profile values to 0..20. Hidden attributes
without a characterized profile conversion are omitted. Reputation, potential
ability and past match ratings are not selection scores.

`native/best_xi_selection.hpp` contains the explicit attribute weights for keepers,
central defenders, fullbacks, holding/central/attacking midfielders, wide players
and strikers. Both positions without/with the ball contribute. Current condition
reduces the attribute score, and unfamiliar lines/sides incur positional penalties;
specialist keepers receive an additional penalty outside goal and vice versa.
Player instructions add weight for crossing, dribbling, forward runs, through
balls, long shots, holding up the ball, free roles, pressing, passing and hard
tackling. Team instructions apply where inherited and also adjust for mentality,
counter attacks, men behind the ball and the offside trap. Explicit Pressing No
overrides the team setting.

An exact assignment over the eleven slots maximizes the combined scores, so a
versatile player can be kept for a role with no good alternative. Equal scores
resolve deterministically from staff IDs, independently of roster order. This is
a documented selection heuristic, not a prediction of match results. With a
depleted pool it fills as many slots as possible, using emergency positional
assignments if necessary. Unfilled slots stay empty; players are never duplicated.
Unused players compete separately for goalkeeper, defence, midfield and attack
bench coverage. The next fixture's high nibble at `+4e` determines the bench size,
bounded to nine. Without a fixture, all nine places are available. An empty pool
clears the match selection.

Selection is published through the original squad setters and lineup mapping,
then the original handler's redraw result refreshes the screen. Both construction
and dispatch are gated by the host Debugging preference. No new career fields or
original resource edits are needed.

The same screen offers a **Debugging** drop-down beside Lineup, only for a
managed club with Debugging enabled. It contains **Recover Players**
(**Recuperar jogadores** in Portuguese) and **Play for Free**
(**Jogar de Graça**). The six toolbar controls have equal widths and spacing
while Debugging is on. Sizing uses the original parent grids; the game's normal
layout publishes both drawing and hit rectangles. Debugging off retains the
original four-control row.

**Recover Players** recovers every player in the club's 50-entry roster,
including starters, selected substitutes and unselected reserves. Active injuries
are retired through the original medical operation; pending medical updates and
deferred injury risks are cleared, and both condition factors become 10,000
(100% in the original readouts). Original discipline operations cancel active
match-count/time bans across all four categories and deferred suspensions.
Completed injury and disciplinary history, accumulated cards, squad selection and
tactics are retained. The original handler rebuilds the screen immediately after
recovery. The changed career state is included in the next ordinary save without
new save fields.

**Play for Free** visits the same 50-entry roster and resolves each player's
primary contract through the original lookup (`10082f20`). It stores a zero
weekly wage (`+0c`) and reads it back, using one euro only if the contract store
refuses zero. The original term services clear the five clause-option bytes
(`+1c..+20`) and the three bonuses (`+10`, `+14`, `+18`, set to -1), then reset
the release amount (`+24`). Expiry, staff links, squad selection, tactics and
every other contract term stay unchanged. Players without a primary contract and
the opponent club are not visited. Both actions are host-gated and need no new
save fields.

Lineup and its items, the Debugging drop-down, Recover Players, Play for Free, Add Goal, the
repeat-goal prefix and the Debugging On/Off values follow the live original
language byte. The eleven original language IDs are supported; unknown IDs fall
back to English. Labels use the original single-byte text encoding, including
player names already supplied by the game.

`cm-best-xi-checks` checks the assignment against exhaustive alternatives and
exercises the selector on an original database world: competing fullbacks, swapping
decoded technical attributes, live crossing instructions (including the high word),
unchanged player records and RNG pool/seed/cursor, disabled gating, caller
preservation, formation changes, bench limits, availability, unique players,
depleted/empty squads and the localization catalog. Lineup modes are checked on the same
world: the age boundary (23 years 364 days in, 24 out), an older keeper in goal with an
all-young outfield, a short young pool completed by older players (also an incompatible one
when it is the only one left), and B Team sharing no starter with the Best XI; each is compared
with the plain selector run on the pool the mode is defined over. `cm-debug-options-checks`
checks the drop-down's cell, its three items, their order, actions and geometry.
The match factory's date slot holds year then day, the reverse of the career layout, so the
lineup check writes day/year itself.
It also prepares original medical injuries, recovery status, condition updates,
match-count/time suspensions and deferred bans; recovery is checked through the
original medical/discipline queries across the full roster. Checks cover disabled
gating, repeat use, caller preservation, retained history and opponent isolation.
Real primary contracts are then created through the original storage services;
Play for Free is checked for zero wages, cleared clauses/bonuses/release amounts,
unchanged unrelated terms, opponent isolation, disabled gating, invalid clubs,
caller preservation and repeat use.
