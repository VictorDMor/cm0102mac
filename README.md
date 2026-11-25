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

New English seasons use the original's rules between the Premier League and the
First Division (two automatic promotion places, a four-club playoff, three clubs
down). Three clubs go down from the Third Division and three come up from the
Conference. Existing seasons retain their saved promotion rules until the next season.
Premier League matches allow seven named reserves and three substitutions,
including pending matches in existing careers after loading. See
[competition rules](docs/native-competition-rules.md) for the original preset and
backup.

New Brazilian Série B seasons play 27 rounds in a single round robin with no
playoffs. The first four clubs are promoted. The final table's first and second
clubs are champion and runner-up. Seasons already in progress adopt the format
at their next rollover.

Automatic substitutions compare each player's own position with consistent
weights and ties. This intentionally fixes the original's inconsistent sorting
and can change which players the AI substitutes.

The opening sequence skips the Feral publisher splash and its wait.

## How it works

- A custom Ghidra-based pipeline translates the original 32-bit PowerPC program
  into C++, which Apple Clang compiles ahead of time for ARM64.
- Native macOS bridges provide windows, input, drawing, audio and file services.
- The application keeps AppKit on the main thread and the game on one dedicated
  worker, so native window events can be serviced while the simulation runs. Raster images
  are prepared on that worker without duplicating their captured pixel buffers;
  AppKit displays them in order and retains native text at backing resolution.
  Game commands still take effect at the original input boundaries.
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
Loan replies and wage-term edits handle zero-wage contracts (including **Play
for Free**) with a zero contribution instead of dividing by zero; nonzero-wage
percentage arithmetic is unchanged.
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
| New Screens | Game Settings toggle, localized in all eleven languages, enables adaptive tactics, club tables (Squad, Calendar and Transfers), all five main player tabs (Profile, Injuries & Bans, Contract, Transfer and History), and recruitment (Shortlist, Player & Staff Search and Scout tabs). Tactics v2 balances a proportional pitch, readable squad panel, supplementary statistics and compact equal Cancel/OK buttons. Club tables expand their tabs/columns and show more rows on taller windows, retaining the original menus, sorting, filtering and season navigation. Calendar row actions and cancellation cover the expanded area. Player details tabs share the available space between their information and season statistics, with aligned biography/loan links, proposal controls and menus. Contract and injury links follow their value column even with a scrollbar. History keeps its view/filter menus, career headings, event pagination and clickable records aligned with the expanded tables. Recruitment lists show more results on taller windows, expand columns and keep filters, pagination, selection and report links aligned. Scout assignment text and its club/staff links remain centered as the description grows; empty, locked and unavailable-scout states keep their original behavior. Search Filters use a reusable Yoga form shell for General/Attributes, including conditional text/range/menu fields; bounded panel growth keeps labels readable on large windows. Transfer proposals reuse Yoga section/column tracks for values, clauses and player exchanges, with room for wrapped introductions and compact footer buttons; original editing, amount menus and confirmation behavior are retained. Player contract negotiations reuse those tracks across Basic, Bonuses and Clauses, retaining rich-text links, requested terms, amount menus, scroll gutters and original offer/cancel commands. Loan terms use the same bounded form tracks for expiry, wage contribution and match conditions, including previous-term restoration. Proposal, player-contract and loan/trial confirmations share a centered message layout and compact buttons, preserving measured links and original submission/dismissal commands. The message screens (an incoming offer opened from the news, a completed transfer, a transfer-list confirmation, offer restriction and feedback messages, a staff release) become centered dialogs with a panel as tall as the message and one row of equal buttons under it; Player transfer status and Previous offers get the same bounded, centered panel with the price row and choices aligned. Board Confidence and Manager Statistics (Manager menu) and the FIFA World Rankings (Competitions menu) get a centered panel with their request links, nation menu and pager kept on their own side and compact equal Back/Next buttons; only the manager or nation column takes the extra width. UEFA Coefficients (with its scrollbar kept at the table's edge), the Hall of Fame, Job Information and Manager Status are laid out the same way. The Manager menu's confirmations (apply for a job, rescind the contract, retire) become centered dialogs like the message screens above, and the Holiday settings get a centered panel with their links carried along and compact equal Yes/No buttons. Arranging a friendly tour (Choose Country and Choose Dates), the Game menu's Credits and Web Sites and the nickname and Send Message dialogs get the same centered table or dialog layout with a compact footer. The transfer dialogs (reject an offer, remove an exchange player, shortlist, compare, withdraw a bid, release a player, end a loan, trial period) follow the same dialog layout, with the rejection reasons in a bounded panel and equal buttons. Scouting a player, the notice that two players must be chosen, and the questions and notices opened from the news (a job offer, a transfer request, the Intertoto place, a tournament invitation, an invalid national squad) use that dialog layout too. So do the disk-full and out-of-memory notices, taking control of a club, the English cup round, a player's appeal, the ultimatum to the board, the generic message box (Exit the game, Restart the game and the other notices and questions that open it, with or without a career) and the "Proceed to the match ?" confirmation before a match. The disciplinary action on a player, the press question and a player's role in the club become one centered panel with equal buttons under it, and the club screen's Last Meetings gets a centered summary and meetings table with its menus and Back/Next buttons aligned. The national team squad notices and confirmations, and the special holiday and counsellor confirmations for a player, join the same dialog layout, and the Manager menu's meetings with the board and the league join the disciplinary panel; the lists of awards and their nominees and the friendly tour's invited clubs get a centered table with a compact Back/Next footer; the transfer rules article, the Historial, the game database and a club's Historial become one centered panel under a tab bar of equal tabs with their filters and page controls on its sides; and the friendly tour's tournament setup dialog moves whole to the middle of the window. Squad Numbers and the European Squad get one header row (menus left, Auto/Clear right), the fifty numbers as a single two-row palette under it, and a bounded, centered squad list with equal rows that shows as many rows as the window holds, up to the whole squad, above a compact Back/Submit footer. News uses shared list/reading tracks, with more headlines and reading rows on taller windows, a compact date column, and rich text reflowed with its links; filters, unread navigation and both tab strips retain their original commands. Club Finances, Income and Expenditure share bounded table tracks with aligned monthly/yearly columns, balance and transfer funds; their View menu, headings and lower tabs follow the window while all amounts and accounting remain original. Next Match completes the main club sections, with bounded match details, more visible rows, aligned date/progress/history controls, and club/player/competition links that follow their text groups; original fixture navigation and the no-match message remain intact. General Info and Stats complete the club information section: taller windows reveal more staff, and metadata, statistics and person links follow the shared panels. The career sidebar fills the current canvas, with its bottom controls anchored to the window; day/match progress drawing and partial repaints follow the same footer even while screen history is cleared. Standard league standings (Overall, Home and Away) gain more visible rows and wider columns, with aligned menus/tabs, full-width promotion/relegation separators and adaptive future/unavailable messages. Compact group stages keep the full standings above a wider fixture list with up to 30 visible match rows. Group and stage menus, team links and available match reports follow the new bounds. Knockout draws use the available width for teams and scores, show up to 30 rows, and keep round selection, live draw controls and pending-draw messages aligned. Competition Results, Calendar and Match Dates share a wider layout with up to 30 visible rows, aligned date controls and compact navigation. Original score and club links, date-group labels and empty messages retain their actions and text. Competition History adapts all seven tabs. The winners table keeps a compact season column alongside placings and the host when available. Record descriptions and their embedded links move together. Games of the Day uses a bounded fixture list with more visible rows, aligned competition/debugging controls and compact equal Back/Next buttons; group headings, venues, scores and right-click result choices follow their original rows. The initial Prepare Game screen uses a centered, bounded two-column menu with five populated rows, clear gaps, a centered final Web Sites choice and compact equal navigation buttons; startup sidebar and headers follow the canvas, while unavailable actions stay disabled. Load Saved Game uses a centered, bounded filename/list panel with up to 30 visible saves on taller windows, retained selection and scrolling, and compact equal Cancel/Info/OK buttons; file filtering, filename editing and the original loading flow remain intact. Save Game and Delete Saved Game share that panel, with two buttons where the dialog has no Info. Nations & Clubs uses a centered panel across its six tabs and restricted pickers, with more visible rows on taller windows, aligned continent/country filters and compact equal Back/Next buttons. Nation, club and competition links retain their original actions. The global Transfers history uses a wider five-column table, up to 30 transfers per page, aligned date and country controls, and compact navigation. When a resize reduces the page count, the browser clamps to the last available page. The live match shares the archived report layout, with a wider scoreboard, commentary and team tables. Latest Scores and the live league table use the same space, and possession bars follow each original match update. Shared table, details and form layouts retain the original fonts, colors, labels and actions. Off restores the original layout; OK/Cancel apply or discard changes. The preference persists independently of career saves. Non-playing/Form profile views, remaining competition tabs and network sessions retain their original geometry. |
| Parallel Matches | Game Settings toggle, on by default: background matches of a day are played on several processor cores (about a quarter less waiting in a multi-league career on an 8-core machine). Each match draws from its own random sequence, so results differ from the original executable's for the same save while their statistics stay the same; Off plays the day in the original's order. Restart Game, which the Mac original left empty, now closes the game and reopens it at the start screen. |
| Season databases | Local New Game defaults to 2001/2002. Arrows above the leagues select the official CM00/01 3.89 data as 2000/2001 (base year 2000) or SI 3.9.68 data as 2002/2003 (base year 2002). Only tables, database names and player setup are imported; the engine and other assets remain unchanged. Each database has isolated conversion caches; saved careers retain their embedded data and dates. With New Screens enabled, league selection uses Yoga tracks, a bounded content panel, more visible rows and compact navigation on the adaptive canvas. |
| C++23 reconstruction | Runtime, calendar, storage and simulation subsystems, the whole tactics system (formations, team tactics, tactics screens), the main program (MAIN.CPP), player statistics (PLAYER_STATS.CPP: season records, career histories, the stats screens), recruitment (SHORTLIST_MANAGER.CPP: shortlists, candidate scans, interest evaluations and registrations) and contract management (CONTRACT_MANAGER.CPP: all 95 routines, including negotiation, daily reviews, news and persistence), with focused equivalence checks |
| Match Debugger | Native match inspection, deterministic scenarios and history controls |
| Recompilation tools | Ghidra exporters, indexed code generation and reference harness generators |
| Asset inspector | Optional Swift tools for examining locally supplied game assets |

This repository contains project source and documentation. Original game files,
generated program payloads, saves, application bundles and private validation
artifacts are excluded. **You must supply your own original Mac release.** The
Windows executable is not supported by this pipeline. Original game content
remains the property of its respective owners.

To add the audited official 3.9.68 data, run
`python3 tools/season_databases.py /path/to/patch-3.9.68.zip`, then
`tools/dev.sh install`. The importer extracts data without executing the Windows
installer. Private packs live under `.local/database-packs/`; their manifests
carry the label, base year and file inventory. The installer orders packs by
year while keeping 2001/2002 as the default. Additional years require an engine
calendar audit before enabling their manifests.

For the audited CM00/01 3.89 archive, use
`python3 tools/season_databases.py /path/to/cm0001-389.zip --release 3.89`, then
`tools/dev.sh install`. This pack offers 19 countries. Argentina,
Brazil, Finland, Ireland, Italy, Northern Ireland and Turkey remain unavailable
because their CM01/02 constructors require absent divisions, cups, awards or
fixed club identities; their clubs and players remain in the database. The
English Conference option enables both the FA Trophy and the English
Conference Cup. The Trophy takes 22 Conference clubs and ten qualifiers from
A Lower Division. The Conference Cup has 22 Conference clubs and five rounds.
The native compatibility layer recognizes the embedded English names by
SHA256, including saved games,
filters unavailable country constructors and resolves the historical Brazilian
top-flight name for continental qualification. The missing Danish database-name
translation uses this season's English names, and the required Green 4 UI colour
comes from the original Mac palette. Birth dates, contracts and history years
are not shifted; Gregorian dates support the leap year 2000.
Competition rules are one global set that each database season inherits and
customizes; see the [competition rules by edition](docs/season-competition-rules-plan.md),
which also tracks the remaining work for source-supported competitions and countries.

For new 2002/2003 careers, installation completes the missing 2002 World Cup
podium (Brazil, Germany, Turkey) and Ronaldo's Golden Shoe (eight goals), plus
the 2001/2002 European Champions Cup (Real Madrid, Leverkusen) and UEFA Cup
(Feyenoord, Dortmund) finalists. The imported archive stays intact; corrected
data receives a new conversion-cache directory. Existing careers keep their
embedded history. The single host field cannot represent Korea/Japan jointly,
and ordered award slots cannot represent tied Silver Shoe winners, so those
optional fields remain empty.

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
| Career play | Create/load a career, manage a club, inspect players and competitions, advance dates, play complete matches and return to management. **Z** activates the enabled **Continue Game** button across career screens; during matches it retains play/pause. Text editing and dialogs retain their normal keys; holding Z does not repeat the action. |
| Persistence | Normal save, exit, reopen and reload have passed in local use. Command-S requests a save to the last successfully loaded or saved career file; Victor confirmed one quick-save and reload, while other contexts remain unverified |
| macOS presentation | Native title bar, shadow, dragging and minimization; normal quit flow. Tactics pilot adapts to window size with additional player columns, preserving the original fonts/colors/controls; other screens retain 800×600 logical content. Native drawing, 100–200% scaling, Fit to Screen, keyboard commands and audio; euro amounts display **€** |
| Match speed | A native **Turbo** choice at the top of the original match-speed selector, selected by default and remembered across launches; match calculations are unchanged |
| Lineup and national call-ups | A **Lineup** squad drop-down (**Best XI**, **Best U23**, **B Team**) is always available for the managed squad. **Command-A** on that squad screen invokes the same **Best XI** action in both classic and New Screens layouts. All three exclude players loaned out from both starters and bench; incoming loans remain eligible. National teams also offer **Call Up** (**Convocar**) there: keep up to 23 eligible nationals ranked by current ability, preserve retained players and release previous selections outside that list. Locked tournament lists and deadlines are checked before any roster change; an impermissible update leaves the squad intact. Injury eligibility uses actual elapsed calendar days across year boundaries. This is a normal feature with no Debugging toggle. |
| In-game debugging | The career sidebar’s **Debugging Options → Enable/Disable Options** opens an original-style modal with independent On/Off settings for all 19 debug controls, grouped under headings by what they act on (**Squad**, **Player**, **Contracts & Transfers**, **Match**, **Results**; localized like the labels). The groups are laid out with the Yoga layer (`debug_options_layout.cpp`): one column on the classic 800x600 canvas, paged with **Previous**/**Next** (two pages), and two columns on one page when the canvas is wide (the modal itself still runs on the classic canvas today, as every modal group does). A utility row holds **All On**/**All Off** and the pager; **Cancel**/**OK** close the footer. Confirm saves the host-only preferences; Cancel discards edits. Existing global Debugging preferences migrate automatically. Enabled actions appear in a **Debugging** squad drop-down with **Recover Players**, **Play for Free**, **Extend All Contracts by 1 Year** and **Remove Unhappiness** (**Remover Insatisfações**; its own toggle, the human's own squad only: for every player with a contract at the club it clears his transfer-request grievances through the original review routine, the press's Unsettled mark and a negative happiness (raised to the game's own resolved level, 15), and resets his playing-time and grievance counters; morale, the transfer-list byte, squad status and other status marks stay), a **Debugging** submenu to the player Action menu, a scorer dropdown and a repeat-goal button; these controls follow the game's language. The player submenu offers **Recover Player** (**Recuperar Jogador**) with its own toggle, for any injured or suspended player (it is absent for one with neither): it ends his injury, queued medical updates and active or deferred suspensions through the same original operations as **Recover Players**, and leaves everyone else alone. It also offers **Maximize Attributes** (**Maximizar Atributos**) with its own toggle: apply 20 to positive attributes and 1 to Dirtiness and Injury Proneness once; normal attribute changes remain unlocked. It can also extend a contract by one year, including zero-wage contracts; it repairs an invalid or stale expiry cache from the valid contract/player date. Collective renewal extends every primary contract owned by the club, including non-playing staff and players loaned out, without extending borrowed players’ contracts. Its toggle is independent of individual renewal. The contract action preserves existing terms and cancels pending and accepted direct agreements to join another club after expiry. Best XI v2 compares available players' attributes, positional fit and condition against the current tactical instructions, optimizing all eleven starters together and filling the fixture's bench. Recover Players removes injuries and suspensions and restores every roster player's condition to 100%, including unselected reserves. Play for Free sets every roster player's primary-contract wage to zero (one euro if zero is refused) and clears clauses, bonuses and the release amount. Top-row/keypad **1/2** give random goals H/A (central forwards +30%, no keepers), **3/4** dismiss A/H, **5/6** injure A/H (uniform players including keepers, random medical injury); **Z** plays/pauses through the original control |
| Fixture debugging | With an underdog or All Draw option enabled, the fixture list has a **Debugging** menu beside **Competition**: **Underdogs Win** (**Zebras Vencem**) gives the lower-reputation club's full roster 20 in all 31 profile attributes; **MAX** also gives the favorite 5. Technical attributes use the original ability-dependent conversion. The action is one-shot for the displayed pending fixtures; equal reputations are left alone. Temporary values are scoped to match processing, restored on every return (including failures), and disarmed at full time, so saves and other club operations retain normal attributes. Armed underdog plans also clear when loading or starting a career. **All Draw** (**Todos Empatam**) forces pending simulated, unobserved matches in the displayed competition and date to draw, across all time slots and rounds. It has an independent Enable/Disable Options toggle; clicking the action again cancels it. Individual right-click choices take priority, followed by All Draw and then recurring club rules. The day selection lasts for the session and clears on career load/new game. |
| Individual match results | Enable **Match Results** in **Debugging Options**, then right-click (or Control-click) a pending unobserved match on the fixture list: the home team's name selects a home win and turns light green, the **v** selects a draw and shows the localized word for draw, and the away team's name selects an away win. Right-clicking the chosen cell again clears that match; another cell of the same match switches the choice. Left clicks keep their original actions. Choices last for the current session and are consumed when that fixture finishes; they are not stored in career saves and are cleared when a career load begins. Selected unobserved matches use the quick engine even when the competition would normally simulate them in detail. They generate varied, mostly narrow scores (including occasional 4–0 and 4–1 wins) through the original goal/event/scorer/keeper bookkeeping; normal matches follow the original simulation. A cup draw keeps a level football score through extra time, with original penalties/aggregate rules deciding qualification. Official abandonments and forfeits retain the original rules. Watched matches are excluded, including a fixture chosen for viewing after its result was marked. |
| Recurring club results | **Debugging Options → Teams with Forced Results** manages an initially empty club list. **Add team** opens Nations & Clubs with the default nation and continent populated; browse to any club and choose **Force Results** in its header, using the club colors like Apply for Job. The list shows 12 clubs per page, clear column headings, a team count/range, page indicator and disabled navigation at the ends. Empty-state instructions explain how to add the first team. It retains the tactics-instructions pitch, green available choices and a yellow selection frame. Each club defaults to **Win**, with **Draw** and **Loss** alternatives. Every row has **Remove**, followed by a confirmation naming the club and explicit **Remove/Cancel** buttons; Escape cancels in place. Names are informational labels. Rules apply only to simulated, unobserved matches. When both opponents are listed, complementary Win/Loss rules are honored; other pairs cancel their club rules. All Draw and manual Match Results keep their higher priority. The list is independent of the Match Results toggle and persists on successful career saves in a `.forced-results` companion next to each save; keep that file when copying a career. New careers start empty. Original career bytes and watched matches are unchanged. |
| Force Purchase | Enable **Force Purchase** (**Forçar Compra**) in **Debugging Options**, then choose it under **Action → Debugging** on a player contracted to a CPU-controlled club. Submit the normal transfer bid and contract offer. The seller and player accept the submitted terms, and the original transfer confirmation, dates and eligibility rules apply. Complete negotiations in the same session so the game can apply the override to the pending bid. |
| Force Staff Signing | Enable **Force Staff Signing** (**Forçar Assinar**) in **Debugging Options**, then choose it under **Action → Debugging** on an eligible staff member at a CPU-controlled club. Managers are offered the assistant manager role. Assistants, coaches, scouts and physios keep their role. The staff member accepts the submitted terms, and the club still confirms the deal. Submit the offer and receive the reply in the same session to apply the override. |
| Player sales | With the corresponding debugging options enabled, **Cancel Sale** in the player Action → Debugging submenu immediately cancels a permanent transfer scheduled from the current manager's club, including a deal involving exchanged players. **Remove Release Clause** removes a player's minimum-fee release clause while keeping other contract terms. |
| Match Debugger | Separate native application with seeded matches, player/event inspection, substitutions and rewind/redo; its full product contract remains incomplete |
| Reconstruction | Semantic C++23 subsystems coexist with generated original routines; all 95 audited CONTRACT_MANAGER.CPP entries are implemented. The whole game is still unfinished |
| Performance | Large multi-league careers are slow: one weekly Continue with 26 playable countries took about eight minutes in local diagnosis, and creating an all-league career stayed CPU-bound for over 40 minutes in an uncontrolled diagnostic run. Optimization is ongoing |
| Availability | Developer source workflow for Apple Silicon / macOS 13+; your original Mac files and local compilation are required |
| Remaining work | Multiplayer and some services remain unsupported; the whole game port is unfinished and no public binary release is available |

## Developer build

The current workflow targets **Apple Silicon and macOS 13 or later**. It requires
Apple Command Line Tools with C++23 support, CMake 3.28+, Zstandard with its
static library (`brew install zstd`), Python 3, and Ghidra with a compatible JDK. Ghidra 12.0 with its native ARM64 decompiler was used for the
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
