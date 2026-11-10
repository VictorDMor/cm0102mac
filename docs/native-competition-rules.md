# English promotion preset

New English seasons use the original's rules between the Premier League and the
First Division again (restored 2026-11-10 at Victor's request): two automatic
promotion places, a four-club playoff (3rd–6th) for the third, and three clubs
relegated from the Premier League.

A preset used from 2026-11-02 to 2026-11-10 gave three automatic places, an
eight-club playoff (4th–11th) for a fourth and four clubs down. No new season
takes it, but a career whose current season was configured with it keeps that
season's saved rules and ends it by them; its code stays for that. Its bracket
is 4th–11th / 7th–8th and 5th–10th / 6th–9th: quarterfinals and semifinals are
single matches at the better league finisher's ground, the final a single match
at the original neutral national venue, every round with extra time and
penalties and no replays, around May 12, 19 and 28 by the original calendar's
weekday adjustment.

Three clubs are relegated from the Third Division and three promoted from the
Conference, without a playoff (the original has one each way; added 2026-11-10).
The original promotes the Conference champion only if his ground passes the
Third Division's test, and otherwise nobody moves. With three places each of the
three Conference clubs takes that test. One who fails stays in the Conference,
with the original's news item, and the best placed club in the Third Division's
relegation places stays up for him, so both divisions keep their sizes; if all
three fail nobody moves. When the Conference is not in the game the candidates
are the first three clubs of the original's own list of Conference clubs. The
Conference's own relegation (three clubs) and the Third Division's promotion
(three clubs and a four-club playoff) are unchanged. The exchange is in
`native/eng_prm_season.cpp`; the season's own rule bytes choose, so a season with
one place each way, as an existing career's current season is, ends through the
original's code. The generated ENG_PRM reference (`CM_ENG_PRM_ORIGINAL=1`) knows
one place only: a paired replay across the end of a three-place season cannot
match. The ground test (0x100f7cf0 with r7 = 1) is the original's, side effects
included, now run for each candidate; the news item for a refused club is the
original's text, written for the champion. That test is the automatic ground
expansion seen in play: a ground short of the division's capacity or seats is
enlarged, the club is charged and the expansion news (0x10103380) is posted, and
the club goes up. It refuses only a club without a ground record, one in the
range 0x100c7d00 tests, a ground already above 120,000, or a club whose balance
does not cover the cost while the cost is at least 500,000 times the byte at
+0x40 of its club+0x80 record's +0x1c record (each refusal raises that byte by
one, up to 20). Refusal is therefore rare.

`native/english_competition_rules.hpp` holds the preset and seeding policy.
`CM_ENGLISH_RULES_ORIGINAL=1` selects the original rules for newly configured
seasons and for the existing equivalence checks. The default (`current`) differs
from the original only between the Third Division and the Conference. This is an
intentional gameplay change, not an original/C++ equivalence claim. Other
competitions retain their existing rules.

Existing careers retain the current season's saved rules, including an existing
playoff, and take the rules above at their normal next-season configuration; a
season of the withdrawn preset is followed by one of the original's rules. League
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
