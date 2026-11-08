# CM 01/02 for Apple Silicon

A native ARM64 macOS port of **Championship Manager 01/02**, built through static
recompilation of the original PowerPC Mac executable and ongoing C++23
reconstruction of its routines.

**Status: playable development build, actively being developed.** Core
single-player gameplay has been verified in local use: creating and loading a
career, managing a club, advancing dates, completing matches, returning to
management, saving and reloading. Multiplayer and some less-used services remain
unfinished. There is no public binary release yet.

The Inter-American Cup is retired after its final 1998 edition. New careers
omit it. Existing careers retain its history, hide it from the Competitions
sidebar menu and remove future unplayed games on load.

New English seasons use three automatic First Division
promotion places plus an eight-club, single-leg playoff (4th–11th), with the better
league finisher hosting before a neutral final; four clubs leave the Premier
League. Existing seasons retain their saved rules until the next season. See
[competition rules](docs/native-competition-rules.md) for the original preset and
backup.

New Brazilian Série B seasons play 27 rounds in a single round robin with no
playoffs. The first four clubs are promoted. The final table's first and second
clubs are champion and runner-up. Seasons already in progress adopt the format
at their next rollover.

## How it works

- A custom Ghidra-based pipeline translates the original 32-bit PowerPC program
  into C++, which Apple Clang compiles ahead of time for ARM64.
- Native macOS bridges provide windows, input, drawing, audio and file services.
- Reconstructed C++23 subsystems progressively replace generated routines while
  preserving calling conventions, data layouts and observable effects.
- The runtime contains no CPU interpreter or JIT and runs without Wine or a
  legacy Mac OS virtual machine. It retains a checked, big-endian 32-bit data
  address space for the original game's pointers and structures.

Reconstructed subsystems coexist with statically generated original routines.
`CONTRACT_MANAGER.CPP` has all **95 audited routines** implemented as semantic
C++23; `CLUB_RECORDS.CPP` adds **92 audited routines** for club records, record
news, season archives and binary persistence. `EURO_CHAMP_QUAL.CPP` adds
**31 audited routines** for European qualifying calendars, draws, progression
and binary records. `TRANSFER_SCREENS.CPP` adds **75 audited routines** for
transfer offers, player status, transfer history, loans and related confirmations.
`INTERNATIONAL_AWARDS.CPP` adds **3 audited routines** for constructing six
international prizes and dispatching their dated publications.
`AWARD_MANAGER.CPP` adds **21 audited routines** for award groups, histories,
ballot context and award news.
`AWARD_SHORTLIST.CPP` adds **14 audited routines** for candidate lists, ballots,
award voting, publication and binary persistence.
`NATION_AWARDS.CPP` adds **20 audited routines** for award selection, publication,
team awards and monthly-rating persistence. `FIFA_RANKINGS.CPP` adds
**15 audited routines** for international ranking points, rolling history,
classification and binary persistence. `CASH.CPP` adds **12 audited routines**
for monetary conversion, editing, currency configuration and text formatting.
`SIPRINT.CPP` adds **9 audited routines** for printer lifetime, page-format
resources, page setup and text pagination. `DB_FILES.CPP` adds **59 audited
routines** for loading, converting and saving the database tables. `AWOL.CPP`
adds **21 audited routines** for staff absences, their messages and the daily review.
`EUROPEAN_CUP.CPP` adds **23 audited routines** for the Champions League's
entrants, calendars, group draws, knockout and results.
`media.cpp` adds **75 audited routines** for the press: its stories, reactions,
news text, response dialog and data file.
`SIMULATED_STATS.CPP` adds **7 audited routines** for club groups, simulated
fixtures, result publication and binary records.
`GER_FIRST.CPP` adds **11 audited routines** for the German First Division's
calendar, entrants, regional assignment and season transition.
`HALL_OF_FAME.CPP` adds **20 audited routines** for rankings, localized
categories, file import, reports and binary storage.
`DATABASE.CPP` adds **189 audited routines** for the game database: loading,
index resolution, saving, language files, new-game player and staff generation,
record lookups, names and searches.
Routine-owner counts in older
integration reports are not whole-game completion percentages. Much of the
simulation still uses generated code. Native performance is the current priority,
ahead of further reconstruction.
See the
[C++23 progress report](docs/native-cpp23-port-progress.md) and
[current status and limitations](docs/native-port-status.md).

## What is included

| Area | Contents |
| --- | --- |
| Native game | ARM64 runtime, macOS bridges and application shell |
| C++23 reconstruction | Runtime, calendar, storage and simulation subsystems, the whole tactics system (formations, team tactics, tactics screens), the main program (MAIN.CPP), player statistics (PLAYER_STATS.CPP: season records, career histories, the stats screens), recruitment (SHORTLIST_MANAGER.CPP: shortlists, candidate scans, interest evaluations and registrations) and contract management (CONTRACT_MANAGER.CPP: all 95 routines, including negotiation, daily reviews, news and persistence), with focused equivalence checks |
| Match Debugger | Native match inspection, deterministic scenarios and history controls |
| Recompilation tools | Ghidra exporters, indexed code generation and reference harness generators |
| Asset inspector | Optional Swift tools for examining locally supplied game assets |

This repository contains project source and documentation. Original game files,
generated program payloads, saves, application bundles and private validation
artifacts are excluded. **You must supply your own original Mac release.** The
Windows executable is not supported by this pipeline. Original game content
remains the property of its respective owners.

## Supported original release

The current pipeline supports the **original PowerPC Mac version 3.9.62**,
published on Macintosh by Feral Interactive. Both `vers` resources in the audited
executable identify version 3.9.62. The installer checks the executable and its
resource fork before copying game data.

| Original executable | Expected value |
| --- | --- |
| File | `Championship Manager/CM 01-02` |
| Data fork size | 9,394,972 bytes |
| Data fork SHA-1 | `a767c5b8edc139598c28afb16e404435d4877024` |
| Data fork SHA-256 | `91692f95df17cf851e7c1d6dbdecd0bd0a5809ce1937133e209aee859a6b516b` |
| Resource fork SHA-256 | `1f4ae4f499d43f163b9153026939b19a2ad022a8b4181690ba7b1b49cf5e551c` |

These hashes identify the executable's forks, not a disc image or a native build.
The game is catalogued as
[Championship Manager: Season 01/02 (Wikidata Q1037535)](https://www.wikidata.org/wiki/Q1037535).
Other original releases, Windows executables and community patches have not
been accepted as inputs to this pipeline.

## Features and current limits

| Area | Verified behavior and limits |
| --- | --- |
| Career play | Create/load a career, manage a club, inspect players and competitions, advance dates, play complete matches and return to management |
| Persistence | Normal save, exit, reopen and reload have passed in local use. Command-S requests a save to the last successfully loaded or saved career file; Victor confirmed one quick-save and reload, while other contexts remain unverified |
| macOS presentation | Native text/control drawing, 100–200% interface scaling and Fit to Screen, keyboard commands and audio; euro amounts display **€** |
| Match speed | A native **Turbo** choice at the top of the original match-speed selector, selected by default and remembered across launches; match calculations are unchanged |
| In-game debugging | The career sidebar’s **Debugging Options** opens an original-style modal with independent On/Off settings for all 12 debug controls (squad recovery/free wages, contract extension/sale cancellation/release clause, both underdog modes, goals/repeat goals, red cards and injuries). Confirm saves the host-only preferences; Cancel discards edits. Existing global Debugging preferences migrate automatically. **Best XI** is always available for the managed squad. Enabled actions appear in a **Debugging** squad drop-down with **Recover Players**, **Play for Free** and **Extend All Contracts by 1 Year**, a **Debugging** submenu to the player Action menu, a scorer dropdown and a repeat-goal button; these controls follow the game's language. The player submenu can extend a contract by one year, including zero-wage contracts; it repairs an invalid or stale expiry cache from the valid contract/player date. Collective renewal extends every primary contract owned by the club, including non-playing staff and players loaned out, without extending borrowed players’ contracts. Its toggle is independent of individual renewal. The contract action preserves existing terms and cancels pending and accepted direct agreements to join another club after expiry. Best XI v2 compares available players' attributes, positional fit and condition against the current tactical instructions, optimizing all eleven starters together and filling the fixture's bench. Recover Players removes injuries and suspensions and restores every roster player's condition to 100%, including unselected reserves. Play for Free sets every roster player's primary-contract wage to zero (one euro if zero is refused) and clears clauses, bonuses and the release amount. Top-row/keypad **1/2** give random goals H/A (central forwards +30%, no keepers), **3/4** dismiss A/H, **5/6** injure A/H (uniform players including keepers, random medical injury); **Z** plays/pauses through the original control |
| Fixture debugging | With either underdog option enabled, the fixture list has a **Debugging** menu beside **Competition**: **Underdogs Win** (**Zebras Vencem**) gives the lower-reputation club's full roster 20 in all 31 profile attributes; **MAX** also gives the favorite 5. Technical attributes use the original ability-dependent conversion. The action is one-shot for the displayed pending fixtures; equal reputations are left alone. Temporary values are scoped to match processing, restored on every return (including failures), and disarmed at full time, so saves and other club operations retain normal attributes. |
| Player sales | With the corresponding debugging options enabled, **Cancel Sale** in the player Action → Debugging submenu immediately cancels a permanent transfer scheduled from the current manager's club, including a deal involving exchanged players. **Remove Release Clause** removes a player's minimum-fee release clause while keeping other contract terms. |
| Match Debugger | Separate native application with seeded matches, player/event inspection, substitutions and rewind/redo; its full product contract remains incomplete |
| Reconstruction | Semantic C++23 subsystems coexist with generated original routines; all 95 audited CONTRACT_MANAGER.CPP entries are implemented. The whole game is still unfinished |
| Performance | Large multi-league careers are slow: one weekly Continue with 26 playable countries took about eight minutes in local diagnosis, and creating an all-league career stayed CPU-bound for over 40 minutes in an uncontrolled diagnostic run. Optimization is ongoing |
| Availability | Developer source workflow for Apple Silicon / macOS 13+; your original Mac files and local compilation are required |
| Remaining work | Multiplayer and some services remain unsupported; the whole game port is unfinished and no public binary release is available |

## Developer build

The current workflow targets **Apple Silicon and macOS 13 or later**. It requires
Apple Command Line Tools with C++23 support, CMake 3.28+, Python 3, and Ghidra with
a compatible JDK. Ghidra 12.0 with its native ARM64 decompiler was used for the
recorded builds. Swift 6 is needed only for the optional asset inspector.

With your original Mac media mounted, analyze it and build locally:

```sh
git clone https://github.com/VictorDMor/cm0102mac.git
cd cm0102mac

JAVA_HOME='/path/to/jdk/Contents/Home' bash tools/analyze_mac.sh \
  '/path/to/ghidra' '/Volumes/CM 01-02'
JAVA_HOME='/path/to/jdk/Contents/Home' bash tools/build_aot.sh \
  '/path/to/ghidra'
```

The build script exports and indexes the analyzed program, generates C++, builds
the native targets and runs the configured checks. Generated inputs and builds
remain under `.local/current/`. After the first build, `tools/dev.sh` handles
day-to-day regeneration, incremental builds, tests and installation; see
[CLAUDE.md](CLAUDE.md) for the development process. Initial compilation covers roughly 1,922 translation
units. This is a development workflow; unsupported original addresses or native
services stop with explicit diagnostics.

After building, double-click `Install Native Prototype.command` in Finder with
the original media still mounted for the first installation. The installer
verifies the supported executable and resource fork, copies the required data,
and installs `~/Applications/reCM 01-02.app`. Game data and saves
use `~/Library/Application Support/CM 01-02 Native/`. Later installations verify
the existing data and preserve saves. The installer creates the app icon from
the original Mac game's local resources for each installation.

The installer requires your local build; cloning this repository alone does not
provide a prebuilt application. See the
[recompilation pipeline](docs/aot-port.md) for technical details and
[Match Debugger documentation](docs/native-match-debugger-application.md) for its
separate development workflow.

## Validation and contributions

Reconstruction work is accepted in bounded subsystems. Reports distinguish
component comparisons against original routines, production integration, installed
application checks and real-match evidence. Passing an isolated comparison does
not mean that a subsystem is installed or that the whole port is complete.

Useful contributions include reconstructing remaining simulation routines,
characterizing original behavior, improving macOS compatibility and reproducing
gameplay regressions. Open an issue describing the behavior, macOS version and
source revision. Keep original game content, saves and private runtime files out
of issues and pull requests.

For the optional asset inspector, independent checks can run without game data:

```sh
swift run cm-assets-checks
```

The root CMake project is an older, optional comparison against an external ReCM
checkout; the active game build uses `native/` through `tools/build_aot.sh`.

## Project reports

- [Native port status](docs/native-port-status.md)
- [C++23 reconstruction progress](docs/native-cpp23-port-progress.md)
- [Static recompilation architecture](docs/aot-port.md)
- [Match Debugger](docs/native-match-debugger-application.md)
- [Native presentation and commands](docs/native-presentation-and-direct-commands.md)
- [Native match speed](docs/native-match-speed.md)
- [Background processing and league loading performance](docs/native-background-performance.md)
- [AWOL records and absence processing](docs/native-awol-records.md)
- [Staff contract lookup and creation](docs/native-contract-integration.md)
- [Daily injury controller](docs/native-injury-daily-controller.md)
- [Injury controller persistence](docs/native-injury-controller-persistence.md)
- [Injury controller bootstrap](docs/native-injury-controller-bootstrap.md)
- [Injury reporting and history](docs/native-injury-reporting.md)
- [Injury status and text readouts](docs/native-injury-status-readouts.md)
- [Ball-action reconstruction](docs/native-ball-action-reconstruction.md)
- [Latest accepted reconstruction: actor decision policy](docs/native-actor-policy-reconstruction.md)

Individual reports retain the evidence and limitations of their delivery dates.
The root [`.recomp.json`](.recomp.json) supplies the project's listing metadata
using the [recomp.board specification](https://recomp.fyi/spec).
