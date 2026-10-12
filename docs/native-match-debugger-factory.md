# Independent detailed match factory

## Result

The debugger's C++23 factory creates an actual detailed match from two original clubs, a date and an explicit nonzero random seed. It loads original program/database inputs into a new owned world, initializes real player records and remaining original match services, and invokes the original constructor and initializer. The resulting paused session advances through the same reconstructed shared engine as the production game.

This is a reusable engine prerequisite, not delivery of the standalone `.app`. Lineup editing, player/goal interventions, event inspection and the interactive application remain pending. The semantic manifest remains at 601 entries. The factory retains original setup dependencies; it does not claim that their bodies have been reconstructed or replace them with a second football model.

## Inputs and ownership

`MatchInputs` supplies the original program, read-only game and a new private directory. `MatchSetup` supplies distinct actual club IDs, a supported ordinal date and a nonzero seed. Clubs must have at least eleven original players; national-team setup and empty database clubs are rejected. There is no career file, captured match or fixed club pairing input. The default setup is an experiment dated in 2001; supported date validation preserves the existing calendar implementation.

The optional `DatabaseWorld` seed invokes the original pool initializer before loading the database. Existing unseeded catalog/world callers retain their behavior. Reset uses the same setup and seed with a new directory and state. Failed replacement creation preserves the prior session. Read-only session state and match-address access support the future inspector without transferring ownership.

## Remaining original setup dependencies

The full original new-world routine (`0x10343630`) also filters the database using selected career leagues. With no career league selection it discards most staff and clears Arsenal's roster. It is therefore unsuitable as the independent factory. Whole-world club ranking and reserve-roster maintenance are also outside this isolated experiment's preparation.

The factory executes the original 22-instruction reserved-frame prologue, then the complete live staff-record preparation region `0x10343990..0x10344074`. That region allocates the 100-byte staff records and retains original date adjustment, condition/fitness calculation, random consumption and negative record sentinels. A scoped boundary observer stops after its final record loop, before career ranking and unrelated services; the host restores the reserved frame after all nested calls have returned. This is explicitly a remaining original dependency boundary, not a return claim for the entire career routine. It remains a reconstruction target.

The factory constructs these original services in their characterized order:

| TOC slot | Allocation bytes | Original constructor |
| --- | ---: | --- |
| `-0x7a6c` | `0x5cc` | `0x103acf80` |
| `-0x7a54` | `0x10` | `0x10145af0` |
| `-0x7b54` | `0x28a4` | `0x1048ebb0` |
| `-0x7afc` | `0x30` | `0x1000e6b0` |
| `-0x7b3c` | `0x48` | `0x10301540` |
| `-0x7ba0` | `0x2c` | `0x101840e0` |
| `-0x7b7c` | `0x3c` | `0x10077ee0` |
| `-0x7aac` | `0x0c` | `0x10432540` |
| `-0x7a7c` | `0x67bc` | `0x103cfe50` |

`0x10354cb0` initializes the original competition index from actual competition counts and empty manager competition lists. There are no scheduled career competition objects. The original isolated/statistics fixture kind (`-2`) supplies actual selected club and stadium pointers and the chosen date. The match allocation is `0xe0bc`, constructor `0x101d16a0`, and initializer `0x101d1760`.

Detailed execution requires initializer argument 5 to be zero and the original watched-mode flag at TOC `-0x642c` to be enabled. Without that flag the initializer forces quick simulation, which finishes a match in one call. Both original lineup-selection boundaries reject fewer than eleven eligible players before entering the game's modal warning path. Creation has a finite instruction budget. The session uses a separate finite step limit, including fresh kickoff work.

## Focused evidence

- Arsenal/Manchester United and Liverpool/Chelsea initialize from actual database records in three fresh worlds.
- Every side has eleven distinct real starting players and positions, with staff/player records belonging to its selected original roster.
- Sessions open paused before kickoff. Scheduled execution and pause act on the real engine.
- Twelve productive ticks execute: eight through the shared C++23 body and four through the original-control body.
- Explicit-seed reset reproduces the complete initial match record. The original and C++23 four-tick intervals agree in complete match bytes, inspections, return values and instruction accounting.
- An invalid replacement seed preserves the existing actual experiment. A second pairing has independent state and does not change the first match.
- The affected bootstrap, factory, session and check units pass UBSAN with exact ordinary output and empty stderr. Remaining generated dependencies were not newly sanitized.
- Focused fresh-world and existing session checks also pass. The accepted production archive/generation is reused; no duplicate set of 1,922 pages is built.

Private characterization and receipts are under `.local/match-debugger-factory`. Original game/career files and the installed app are compared before and after. The installed game executable is unchanged. Career persistence is **not retested**; performance is **not benchmarked**.

## Required continuation

Deliver the separate macOS debugger application with club selection, actual lineup inspection/editing, pause/step/run/reset and chronological engine events. Characterize and implement player removal and goal injection against related live records, then verify their observable effects interactively. Continue reconstructing the remaining original match initialization and football dependencies in structured C++23; the independent factory now supplies repeatable actual runtime inputs for that work.
