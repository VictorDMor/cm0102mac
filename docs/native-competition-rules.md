# English promotion preset

New English seasons use three automatic First Division promotion places and an
eight-club playoff (4th–11th) for one further place. Premier League places
17th–20th are relegated; league sizes remain 20 and 24. First Division relegation
to the Second Division is unchanged.

The fixed bracket is 4th–11th / 7th–8th and 5th–10th / 6th–9th. Quarterfinals and
semifinals are single matches at the better league finisher's ground. The final
is a single match at the original neutral national venue. Every round allows
extra time and penalties, with no replays. May dates use the original calendar's
weekday adjustment: quarterfinals around May 12, semifinals around May 19 and
final around May 28.

`native/english_competition_rules.hpp` holds the preset and seeding policy.
`CM_ENGLISH_RULES_ORIGINAL=1` selects the original rules for newly configured
seasons and for the existing equivalence checks. The default is the expanded
preset. This is an intentional gameplay change, not an original/C++ equivalence
claim. Other competitions retain their existing rules.

Existing careers retain the current season's saved rules, including an existing
playoff, and adopt the preset at their normal next-season configuration. League
fields `+0xc0..+0xc3`, CupStage rounds and the existing entrant seed byte carry
the policy; there is no new save-file field or process-local rank registry.
The larger seed represents the better final league placing, so semifinal home
advantage survives upsets and save/load. An old application is not a supported
editor of a career after it has adopted the expanded rules.

The private pre-change backup is `.local/backups/english-rules-original-20261102/`:
`source.tar.gz` and `manifest.json` identify commit
`17aa8a91244e2feaad0e147ccf540a45268717af`; `installed.app` preserves the then-installed
application and `built.app` the then-built application. To restore the installed
version, use `tools/dev.sh install --app .local/backups/english-rules-original-20261102/installed.app`
with the game closed. Saves and original game data are not replaced.

Future work: a user-facing competition editor with validated presets, balanced
movement between divisions, calendar/bracket constraints, and explicit
season-effective versions persisted with each career. The current change adds
one preset; it does not expose arbitrary rules in the UI.
