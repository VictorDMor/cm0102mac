# Debugging: Best XI

With Debugging enabled, the managed club's squad screen has a **Best XI** button
between Sort and Clear Team. It replaces the match selection immediately, using
the currently selected formation. It does not change the formation or its player
instructions. The player-priority view and other clubs do not offer this action.

The original squad status operation (`10060c00`) excludes ineligible,
unregistered, absent, injured, recovering, suspended and internationally absent
players from the club's 50-entry roster. The original positional selector
(`10210c50`) evaluates the remaining players against the eleven formation roles,
then chooses unused reserves with goalkeeper, defence, midfield and attack
coverage. The next fixture's high nibble at `+4e` determines the bench size,
bounded to the original maximum of nine. Without a fixture, all nine places are
available. Unfilled slots stay empty; players are never duplicated. An empty
candidate pool clears the match selection.

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

`cm-best-xi-checks` exercises the selector on an original database world, including
disabled gating, caller preservation, formation changes, bench limits, availability,
unique players, depleted/empty squads and the localization catalog.
It also prepares original medical injuries, recovery status, condition updates,
match-count/time suspensions and deferred bans; recovery is checked through the
original medical/discipline queries across the full roster. Checks cover disabled
gating, repeat use, caller preservation, retained history and opponent isolation.
