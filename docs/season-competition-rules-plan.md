# Competition rules by database edition

Requested by Victor on 2026-11-17 after the CM00/01 FA Trophy initialization failure.
Status: the rules registry exists (global rules, inherited and customized per
edition) and carries the competition rules the port decides today. England's 2000/01
pilot (Conference, FA Trophy, Conference Cup) runs through it. Career-level
overrides, their persistence and the remaining league exclusions are open.

## The registry (2026-11-17)

`native/season_competition_rules.hpp` declares `CompetitionRules`: one struct
with every rule the port decides, each field holding its global value.
`global_rules` is that set. `native/competition_rules.cpp` defines the three
editions by inheritance:

| Edition | Database | Definition |
| --- | --- | --- |
| `cm0102` | 2001/2002 (Mac 3.9.62) | the global rules, unchanged |
| `si_3968` | 2002/2003 (SI 3.9.68) | inherits `cm0102`; no change yet |
| `cm0001_389` | 2000/2001 (CM00/01 3.89) | inherits the global rules; changes the Conference, FA Trophy qualification, adds the Conference Cup and lists the seven unavailable countries |

To add a rule: add a field with its global value to the struct; every
edition inherits it. To customize a season: assign the field in that
edition's `inherit(...)` block. Code asks `competition_rules(state)` and never
tests the edition or the year itself.

The career's edition comes from the database names it embeds (SHA-256 of the
English names file, also after loading a save), not from the calendar year.

Rules carried today: English promotion places and Premier League named
reserves, the Conference league's kickoff slots, playoff calendar and club
payment, FA Trophy qualification, the Conference Cup, Série B as a single
table, the Inter-American Cup's last edition, and unavailable countries.

`CM_ENGLISH_RULES_ORIGINAL` and `CM_BRA_NAT_SECOND_RULES_ORIGINAL` restore the
shipped game's values for the equivalence checks, applied after the edition.

Still outside the registry, decided by the 00/01 release itself in
`database_season_compatibility.cpp`: the optional symbolic indexes that data
lacks and the Brazilian top-flight alias. They describe the data, not a rule.

Between the moment a load replaces the database and the moment the loaded
names are inspected, the edition reads as `cm0102`. No rule that differs by
edition is read in that window today; the Premier bench hooks on league and
fixture load are the ones to check when an edition first overrides them.

Not there yet: user overrides per career, their place in the save, and
declared mid-career transitions (see Persistence below). A season configured
under one rule keeps it in its serialized fields, as before.

## Goal and completion criteria

Selecting a database must select its competition definitions, participant
selection, calendars and season-transition rules as well as its football data.
Every country playable in the source edition should eventually be playable in
its corresponding pack, with that edition's actual divisions. Do not invent
competitions or move clubs between divisions just to satisfy CM01/02 constants.

A pack is complete when all of its source-supported countries and their required
domestic and international competitions can initialize, resolve qualification
and promotion/relegation, and save/reload with the same rules. Countries genuinely
absent from the source edition are outside that pack's scope.

## What the current code establishes

- `tools/season_databases.py` imports tables and names; `DatabaseSeasons` chooses
  the data source and starting year. These do not supply a competition engine
  for the older edition.
- `native/database_season_compatibility.cpp` currently excludes Argentina,
  Brazil, Finland, Ireland, Italy, Northern Ireland and Turkey in CM00/01 because
  CM01/02 constructor dependencies do not fit. It also maps a historical
  Brazilian competition identity used by continental qualification. An inactive
  country can still be a dependency of a global competition.
- England's CM01/02 FA Trophy selector requires ten additional clubs outside
  its excluded divisions. The imported CM00/01 data yields zero. The unchanged
  original reports `COMP_UTIL.CPP 241`; continuing then consumes invalid list
  entries. The England pilot replaces the temporary four-division mode.
- `native/english_competition_rules.hpp` already separates some rule values,
  but its current preset is global. Victor's seven-player Premier League bench
  and promotion changes are deliberate customizations that must remain explicit
  policies rather than silently becoming historical defaults or disappearing.
- Premier League, First Division, Third Division and Conference already have
  C++ implementations. Reuse those and their guest-record contracts. The Second
  Division and relevant cup construction still include generated code.
- Conversion and country-dispatch checks alone did not detect the FA Trophy
  mismatch. Validation must reach participant selection inside each competition.

## Proposed boundaries

| Component | Responsibility |
| --- | --- |
| `SeasonPack` | Immutable database release identity, data digest, starting season and default rules profile. |
| `CompetitionBindings` | Map stable competition/nation/club identities to this pack's actual guest records; record semantic aliases with evidence. |
| `CompetitionRules` | Typed parameters for a competition edition: participants, stages, calendar, tie rules, qualification, squad limits and finances where applicable. |
| `CompetitionPolicy` | C++ strategies for selection, draws, scheduling and transitions that differ algorithmically. |
| `CountryRules` | Domestic dependency graph and balanced movement between divisions; which competitions the source edition supports. |
| `CareerRules` | The profile revision and explicit user overrides selected for this career, plus any declared future rule transitions. |
| `GuestCompetitionAdapter` | Construct/read/write existing guest objects and call their existing services without exposing host pointers to the game or save format. |

Use one shared implementation for equivalent competitions across editions.
Start from built-in defaults, apply typed edition-specific overrides, and apply
the career's user overrides last. Keep distinct algorithms behind named
policies. A future editor can expose the validated fields and compatible
policies. Store a stable profile revision and the user overrides in the save,
so a later update to the defaults cannot change an existing career's rules.
Use checked-in C++23 definitions. Add a data-file parser if external authoring
becomes a requirement.

A rule profile is keyed by **source edition, logical competition and revision**,
not by the current calendar year or raw database ordinal. A career beginning
in 2000 must not accidentally switch to another database's rules in 2001.

### Optional historical rule changes

“Regras realistas” (working title) is a per-career switch for historical
changes. The database edition supplies the starting rules. With the switch
on, each competition also follows its own timeline of changes, keyed by the
first competition season in which each change applies. The pinned profile
revision includes this timeline. Explicit user overrides still apply last.
With the switch off, resolve the edition rules and explicit overrides alone.

Use the same resolver for domestic leagues, domestic cups and international
competitions. A timeline can be empty or contain several successive reforms.
Each entry identifies its competition, effective season and the rule fields
or policy it changes. Carry other fields forward. Adding a reform that uses
existing fields or policies should require only a timeline entry and its
tests. Record the source for the rule and its first applicable season.

For example, in a career started with the 2001/02 database, the UEFA Cup
uses knockout rounds throughout 2003/04. With the switch on, the 2004/05
edition includes a group stage and subsequent editions keep it until another
change applies. With the switch off, 2004/05 keeps the knockout format.
Select the format when configuring that competition's season, before its
first draw. Toggling mid-tournament affects its next edition. Loading a
save restores the current tournament, including draws and results.

The resolver exists (2026-11-17): `RuleChange` entries in the `timeline`
table of `competition_rules.cpp`, resolved by
`competition_rules(state, RuleCompetition, season)`. A season is the calendar
year it starts in, taken from the competition object being configured. An
edition's rules are those of its own first season, so only later entries
apply to it. The table is empty: no change is verified yet. To add one:

1. Wire the competition's season setup to the seasonal call, as
   `english_rules::new_season` does for English promotion and relegation
   (the only reader so far), and list it in `wired`; the build refuses an
   entry for a competition nothing reads seasonally.
2. Add the entry with its source, and its seasons before, at and after to
   `check_rule_timelines` (`database_seasons_checks.cpp`), replacing the
   check that the table is empty.
3. Raise `rules_revision` and keep the previous revision resolvable
   (`career_rules.hpp`).

A reform that reaches simulation results needs Victor's approval first, like
any other change of rules.

## Rule contents and validation

1. **Participants:** counts, eligible divisions, regional groups, prior-season
   positions, reserves, invitations and byes. Distinguish missing identities
   from insufficient candidates and from a genuinely absent competition.
2. **Format:** round-robin legs, groups, knockout rounds, seeded draws, replay
   rules, extra time, penalties, venues and standings tiebreakers.
3. **Calendar:** season-relative dates, weekday constraints, winter breaks and
   cup windows. Use the existing Gregorian date layer, including year 2000.
4. **Transitions:** promotion/relegation, playoffs, eligibility restrictions and
   continental qualification. Validate both sides of every division exchange.
5. **Other rules:** named reserves/substitutions, registration/discipline and
   financial settings where the original competition actually specifies them.

Run preflight before constructing the career: resolve dependencies, enumerate
entrant pools and check counts, uniqueness and the compatibility of connected
competitions. Diagnostic output should identify the competition, expected and
available participants, and missing dependency. Never continue with an
undersized list or silently substitute unrelated clubs. An unresolved case is
a tracked implementation gap, not a reason to permanently drop its country.

The authoritative starting point is the corresponding original edition's code
and data, inspected privately under `.local/`. Historical federation sources
can clarify intent but do not automatically establish the original game's
behavior. Do not assume today's rules or infer a format from competition names.
If correcting an original simulation behavior is desired, treat it as a
separate, explicit customization.

## Persistence and compatibility

- Retain current-season rule values in the original serialized competition
  fields wherever those fields already represent them. The engine continues to
  consume the same records through the guest adapter.
- Persist a profile ID, revision and user overrides with the career. The first
  persistence milestone must audit the existing archive/container and prove a
  supported metadata location. Do not append bytes speculatively or rely on a
  filename sidecar as the sole source of truth.
- If the original container cannot safely retain native metadata, introduce an
  explicitly versioned native container while preserving legacy save reading.
  Old-application readability must be demonstrated before being promised.
- Existing careers retain their current-season serialized rules. For migration,
  use embedded database provenance and the saved rule values; an ambiguous
  source must not be guessed from the year. Do not reinterpret existing careers
  merely because the installed pack or registry was updated.
- Bind references again after loading, without persisting host pointers. A
  copied save must carry enough information to load the same policy elsewhere.
- At the next season boundary, use the career's pinned profile and declared
  transitions. Applying a later rule revision to an existing career is a
  distinct migration, not an automatic side effect of updating the app.

### Container audit (2026-11-17)

A career file is an archive: one type byte (3 plain, 4 run-length packed), a
count, a 1000-entry directory (offset, size, 256-byte name) and the members.
The 2001 reference save has 120. Besides the fixed `.dat` members, the
original writer bundles **every `*.tmp` file in the game's temporary folder**
(`Runtime/Files`), which is how the per-competition files travel. The original
loader deletes every `*.tmp` there, then extracts the save's.

Checked on an isolated copy of the game driven by `tools/game_driver.py`, with
a copy of the reference save:

- A 35-byte `native_rules.tmp` placed in the temporary folder before a quick
  save became member 121 of the written file, bytes intact.
- After a restart with the file removed, loading that save restored it,
  identical.
- A stale `.tmp` left in the folder was gone after loading a save that does
  not contain it. Temporary files otherwise survive quitting the game.

So a native member needs no change to the container and no new bytes in any
existing member; the game's own save and load code carries it. Not checked: a
save through the Save dialog (the quick save shares its writer, 0x1034ba30),
what a new game does with the folder, and the original Mac or Windows
applications reading such a file (the Mac one runs the same loader code).

### Career rules member (2026-11-17)

Victor approved the member; every career save written by the port carries it.
`native/career_rules.hpp` holds the format and the list of rules a career may
override.

- **Name and lifecycle:** `native_rules.tmp`, in the folder the game resolves
  for `CM3_TEMP`. Written at the career writer's entry (0x1034ba30, mode
  0x16) from the running career's state, so the original code bundles it.
  Read at 0x10346ee8, right after the loader's second pass extracts the
  save's temporary members (0x10346ee4); the running career's rules are
  dropped earlier, where its world is released (0x10346dd4). A new career
  (0x10345bb0) or a database indexed outside a load forgets the rules and
  removes the file.
- **Content:** short text, one statement per line:
  `CM-NATIVE-RULES 1`, `edition cm0001_389`, `revision 1`,
  `historical_rules on|off`, then one `set <field> <value>` per override (for
  example `set england.promotion.conference_promoted 1`).
- **Meaning:** `edition` pins the career's edition from the extraction on,
  replacing the names-hash guess (which still decides for saves without the
  member, and during the part of a load that precedes the extraction).
  `revision` names the checked-in rules of that edition. `historical_rules`
  is the “Regras realistas” switch; missing means off. Nothing reads it until
  the timeline resolver exists. The `set` lines apply after the edition and
  any enabled historical changes; the test presets still apply last.
- **Overridable now:** `england.promotion.conference_promoted` and
  `england.promotion.third_relegated` (1 or 3, and equal),
  `england.premier_named_reserves` (0 or 7), `brazil.serie_b_single_table`
  (0 or 1): the values existing checks already run. Other fields wait for
  their engine paths to be checked with other values.
- **Unknown input:** an unknown format version, edition or revision keeps the
  whole text and writes it back unchanged, applying nothing. An unknown
  field or value is kept and ignored the same way. Each case leaves a status
  line and a log line. A member that cannot be read never blocks a load; an
  empty one counts as absent. A build that raises an edition's revision must
  keep the previous revision's rules and resolve members that name it.
- **Old saves and old builds:** a save without the member behaves as before
  and gains the member on its next save. An older build of the port carries
  the member through untouched, as it does any `.tmp`.
- **No editor yet:** nothing in the game sets the switch or an override. A
  Game Settings row for the switch existed for one commit (f3da8ebd) and was
  removed at Victor's request (2026-11-18) until the competition editor
  exists; `NativePlatform::career_present` and
  `edit_career_historical_rules` remain for that editor, which is to be
  reachable both at New Game and during a career.

Checked with `cm-database-seasons-checks` (format, round trip, ignored and
unknown input, load and save hooks, new career) and on an isolated copy of
the game with a copy of the reference save: the quick save's archive carries
the member, and a member patched inside the save is what the next load reads
and the next save writes. Not checked: the Save dialog path (same writer), a
writer mode other than 0x16 bundling the file, and the original Mac or
Windows applications reading such a save.

## Delivery sequence

| Milestone | Concrete work | Acceptance |
| --- | --- | --- |
| 1. England 2000/01 audit | Compare the original CM00/01 Conference/FA Trophy selection and format with the imported club classifications and CM01/02 call chain. Record exact dependencies and algorithm differences. | An evidenced participant/format specification; no guessed replacement entrants. |
| 2. Minimal registry and pilot | Add stable bindings and an England 2000/01 profile; adapt the differing selection/format in C++ and route only this profile. Keep ordinary 2001/02 and 2002/03 paths and explicit custom rules. | Restore Conference and FA Trophy; construct every required English competition with valid entrants. Remove the temporary English exclusion. |
| 3. Career identity and transitions | Audit/save profile metadata, rebind after load, and test a bounded promotion/relegation/next-season transition with controlled states. | Same rules, participants and fixture representation after reload; balanced division sizes. |
| 4. Recover excluded countries | Inventory actual source support and dependencies for Italy, Ireland, Northern Ireland, Finland, Turkey, Argentina and Brazil; deliver coherent domestic competition groups, ordered by audited dependencies. Include continental consumers. | Re-enable each country only after its competition constructors and transitions are supported. No blanket dependence on a missing CM01/02 award or division. |
| 5. Consolidate packs | Move applicable 2001/02 and 2002/03 definitions and existing English customizations behind the same resolver. Replace scattered compatibility exceptions once equivalent profiles cover them. | Every source-supported country available in its pack; season switching cannot leak bindings, rules or customizations. |

The England pilot needs only the smallest common abstraction justified by its
differences. Do not postpone it for a complete generic engine, and do not claim
all-country support from the pilot. Conversely, restoring a cup's constructor
alone does not finish its save or season-transition behavior.

## Focused verification

- A negative fixture reproduces the original FA Trophy failure with the real
  imported data; the profile must eliminate the invalid input, not suppress
  the error check.
- Check actual participant lists and resulting competition objects, not just
  country callbacks or dispatcher visit counts. Direct C++ calls can bypass
  dispatcher probes.
- For unchanged rules, compare original and C++ guest-visible state and RNG
  order on existing captures and controlled fixtures. For edition-specific
  rules, use that edition's original behavior where executable evidence is
  available, and name any limits of a static audit.
- Check calendar boundaries, progression, balanced division exchange,
  qualifying dependencies and exact existing serialized fields. Include unknown
  profile revisions and failed/partial reads in persistence tests.
- Exercise the shared resolver with successive changes in one competition
  and an independent change in another (for example, format and promotion
  places). Check seasons before, at and after each boundary with the switch
  on and off. Include a competition with an empty timeline. Reload on both
  sides of a transition and preserve the current edition's fixtures when
  toggling after a draw.
- Test 2000 -> 2001 -> 2002 switching and loading a career with another pack
  selected. Preserve Victor's explicit custom-rule policy.
- Live gameplay, visual checks and full-season QA remain Victor's; they are not
  automatic delivery gates or an excuse to skip the focused constructor tests.

The England pilot identifies the 3.89 rules from the embedded database digest.
The Trophy takes up to 50 English A Lower Division candidates in database order,
sorts by descending reputation with the Windows CRT tie order, and swaps within
the first tenth of the pool. It then takes the first ten and appends the 22
Conference clubs. Its semifinal leg rule is the same in both releases, so the original calendar is used unchanged. The Conference calendar
shares the nominal round dates with CM01/02 and uses only default kickoff times.
The 3.89 league has no playoff calendar or CM01/02 club payment. The existing
three-place promotion customization still applies.

The 3.89 profile also enables the Conference Cup with 22 Conference entrants
ordered by prior division and position. The first two rounds admit 12 and 10
clubs respectively. The last two rounds have two legs. The second-leg rules
word is undefined in the Windows constructor and takes a deterministic zero
in the port. The Trophy and Conference Cup share the PowerPC knockout object
and its save routines, with separate competition identities. Careers whose
archive predates the Conference Cup continue without it.

The career's edition, rules revision, historical-rules switch and explicit
overrides are persisted (Career rules member) and the shared timeline
resolver exists, with no entry yet. Next: the advanced competition editor
(TASKS.md), starting from an inventory of what one rewritten league sets at
season setup.
