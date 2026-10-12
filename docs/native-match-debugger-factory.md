# Independent detailed match factory

## Result

The debugger's C++23 factory creates an actual detailed match from two original clubs, a date and an explicit nonzero random seed. It loads original program/database inputs into a new owned world, initializes real player records and remaining original match services, and invokes the original constructor and initializer. The resulting paused session advances through the same reconstructed shared engine as the production game.

The initial factory delivery is a reusable engine prerequisite. The subsequent [application core](native-match-debugger-application.md) supplies the standalone `.app`, interactive execution controls and observed engine changes, and adds the original injury/history prerequisite below. Lineup editing and player/goal interventions remain pending. The semantic manifest remains at 601 entries. The factory retains original setup dependencies; it does not claim that their bodies have been reconstructed or replace them with a second football model.

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
| `-0x7b94` | `0x18` | `0x10646d90` |
| `-0x7aac` | `0x0c` | `0x10432540` |
| `-0x7a7c` | `0x67bc` | `0x103cfe50` |

The empty injury/history manager at `-0x7b94` was added after the installed debugger exposed a null original query on Liverpool/Chelsea's 86th attempted step with seed 67890. Its allocation, constructor and publication match original setup `1034649c..103464c0`. A focused application regression now completes 100 steps through that path. Initial factory comparisons below remain the original twelve-step acceptance scope; the application report records the added regression.

`0x10354cb0` initializes the original competition index from actual competition counts and empty manager competition lists. There are no scheduled career competition objects. The original isolated/statistics fixture kind (`-2`) supplies actual selected club and stadium pointers and the chosen date during player preparation. Before publishing the detailed match, the factory binds that fixture to the existing original Friendly competition record, constructs the original referee context with an actual original official, and supplies `-1` previous-leg goal sentinels. The match allocation is `0xe0bc`, constructor `0x101d16a0`, and initializer `0x101d1760`.

Detailed execution requires initializer argument 5 to be zero and the original watched-mode flag at TOC `-0x642c` to be enabled. Without that flag the initializer forces quick simulation, which finishes a match in one call. Both original lineup-selection boundaries reject fewer than eleven eligible players before entering the game's modal warning path. Creation has a finite instruction budget. The session uses a separate finite step limit, including fresh kickoff work.

## Detailed friendly completion

Cruzeiro Esporte Clube/Manchester United, seed 12345, exposed an end-of-match fault at `10464744`, reading `000008b9`. The original commentary eligibility queries `101fce60` and `101fcf20` pass fixture competition ID `-2` into `104646d0`. That query indexes both the competition records and the live competition pointer array; the negative index reads an allocator header as an object pointer. This is missing standalone fixture context, not a new semantic movement replacement.

The factory retains isolated player preparation, then validates and publishes the original Friendly ID at TOC `-0x7a50` and its existing manager/index binding. Fixture `+0x14` receives that manager's actual competition record; the record ID must match the fixture competition ID. Fixture `+0x4b/+0x4c` receive the original absent-previous-leg goal value `-1`, as in `10371bd8..10371bdc`. Career competition scheduling and career persistence are not implemented by this binding. No guest instruction, competition query, completion result or football event is skipped or replaced.

The focused completion check advances the reported pairing from kickoff using productive engine calls. Its last interval compares the complete architectural state, mapped memory, allocator metadata and instruction accounting between C++23 and original shared advancement. Removing the competition ID binding reproduces `10464744`; removing the competition record reproduces `101f958c`. The session reaches its actual completed state, emits the readable full-time event, rejects another live step and preserves the final state through Back/redo. The process-protocol check covers the full match, original halftime pause, readable final clock/event, completed-step rejection, undo/redo and normal scratch cleanup. Existing factory, lab and worker checks cover seeded reset, independent experiments, reserves, unrestricted substitutions/re-entry, injuries and event/time controls.

The initial completion correction incorrectly treated fixture `+0x14` as the referee pointer. Original initialization at `101d2580..101d25a4` instead resolves the referee ID at fixture `+0x2e` through the official table and passes that pointer to `1021b020`, which stores it at match `+0xe034`. Isolated initialization intentionally supplies null. A later Arsenal/Manchester United run exposed the missing cached referee in an original foul/card query at `10223378` (unmapped byte `24`) after 68 steps. The factory now uses original official ID 0 as its deterministic sandbox default and invokes the complete original referee constructor after isolated player preparation. The regression advances 100 real Arsenal/Manchester United steps, verifies the actual competition-record identity and observes the original foul query reading the actual referee. Removing the cached referee reproduces the original fault. This is context initialization, not a replacement or bypass of the foul/card engine.

This correction adds no semantic manifest entries. Save/Load is **not retested**; performance is **not benchmarked**. Private reproduction logs and delivery receipts are under `.local/debugger-competition-fault`.

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
