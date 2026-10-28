# Debugging: Best XI

With Debugging enabled, the managed club's squad screen has a **Best XI** button
between Sort and Clear Team. It replaces the match selection immediately, using
the currently selected formation. It does not change the formation or its player
instructions. The player-priority view and other clubs do not offer this action.

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

The same screen offers **Recover** (**Recuperar** in Portuguese)
beside Best XI, only for a managed club with Debugging enabled.
The six toolbar controls have equal widths and spacing while Debugging is on.
Sizing uses the original parent grids; the game's normal layout publishes both
drawing and hit rectangles. Debugging off retains the original four-control row.
It recovers every player in the club's 50-entry roster, including starters,
selected substitutes and unselected reserves. Active injuries are retired through
the original medical operation; pending medical updates and deferred injury risks
are cleared, and both condition factors become 10,000 (100% in the original
readouts). Original discipline operations cancel active match-count/time bans
across all four categories and deferred suspensions. Completed injury and
disciplinary history, accumulated cards, squad selection and tactics are retained.
The original handler rebuilds the screen immediately after recovery. The changed
career state is included in the next ordinary save without new save fields.

Best XI, Recover, Add Goal, the repeat-goal prefix and the Debugging On/Off values follow
the live original language byte. The eleven original language IDs are supported;
unknown IDs fall back to English. Labels use the original single-byte text
encoding, including player names already supplied by the game.

`cm-best-xi-checks` checks the assignment against exhaustive alternatives and
exercises the selector on an original database world: competing fullbacks, swapping
decoded technical attributes, live crossing instructions (including the high word),
unchanged player records and RNG pool/seed/cursor, disabled gating, caller
preservation, formation changes, bench limits, availability, unique players,
depleted/empty squads and the localization catalog.
It also prepares original medical injuries, recovery status, condition updates,
match-count/time suspensions and deferred bans; recovery is checked through the
original medical/discipline queries across the full roster. Checks cover disabled
gating, repeat use, caller preservation, retained history and opponent isolation.
