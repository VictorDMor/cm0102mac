# Competition rules by database edition

Requested by Victor on 2026-11-17 after the CM00/01 FA Trophy initialization failure.
Status: England pilot implemented for Conference, FA Trophy and Conference Cup.
The remaining league exclusions are tracked below.

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
Historical transitions, if implemented, are explicit entries in that career's
profile. Logical identifiers are proposed internal identifiers, not assumed
original game IDs.

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

Next implementation task: persist explicit career rules revisions and user
overrides and verify the division movement that feeds the next season.
