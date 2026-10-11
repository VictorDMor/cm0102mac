# Native match debugger

## Product contract

The match-engine reconstruction includes delivery of a separate macOS application, **CM Match Debugger**. This is an engine debugging tool with its own executable, bundle identifier and private working directory. It shares the production engine implementation with the game. It does not require a career or modify installed game saves.

The initial usable tool must let a user select two clubs from the actual game database, inspect and edit their match lineups, start an isolated match, pause, advance one engine step or run to a selected boundary, and inspect the resulting score, clock, players and ordered events. It must support removing a player and adding a goal during the experiment. Interventions must appear in the event log and expose their effects on the real match state. Removing a player before kickoff and withdrawing or dismissing an active player are distinct operations; the tool must label the supported operation precisely.

The purpose is practical investigation: change one condition, execute the engine, and observe the consequences. The interface therefore exposes a state inspector, a chronological event log and explicit execution controls. The debugger opens paused and applies edits at a paused engine boundary. It must remain responsive while a match runs and must report an engine error without presenting an unfinished step as successful.

## Engine and session ownership

The debugger and the game link the same semantic engine sources and the same remaining original dependencies. There must be no second football model, prerecorded match masquerading as execution, or fixture-only club selector. Until all dependencies are reconstructed, the debugger must identify the engine coverage accurately.

Each debugger experiment owns its guest state, random state and scratch files. Process isolation separates the existing host globals from a running game. The production game installation and career runtime are inputs only where explicitly read for database discovery; debugger writes belong to its own directory. Reset creates a fresh experiment from the selected setup. Baseline execution retains the original ordering and rules; intentional debugger interventions are separately recorded.

Goal injection must update the related live engine records consistently, using the characterized scoring operation where available. Changing the displayed score alone does not satisfy the contract. Player removal must affect the active team state used by subsequent engine decisions. The inspector must distinguish observations from interventions and retain before/after values for each edit.

## Delivery and evidence

Acceptance requires a runnable ARM64 `.app`, actual selection of more than one club pairing, an isolated match that progresses through the shared engine, and observable effects from a player removal and a goal intervention. Pausing must stop engine advancement; stepping must advance the actual engine; reset must restore the chosen initial setup. A fixed private match capture may support development and equivalence checks but does not satisfy club selection or session creation.

Focused engine equivalence checks cover the reconstructed entries and their ordered effects. Debugger checks cover session ownership, execution boundaries and intervention consistency. A short interactive run must verify the debugger controls and the observable match effects. Existing career persistence is not retested for this bounded change. Performance gains are not claimed without a separate measurement.

## Current status

The standalone application, arbitrary-club match factory and interventions are not yet implemented or delivered. The session controller in `native/match_debugger_session.*` owns an independent real-engine instance and provides pause, step, scheduled run, reset and a small state inspector. It dispatches the same shared advancement entry used by the game; future reconstruction changes therefore apply to both callers.

A private live match capture exercised 24 actual original-engine steps outside the game UI. Focused controller checks verified paused-state preservation, one-step advancement, agreement with that original interval, deterministic reset, preservation after failed replacement creation, and blocking further execution after a partial budget failure. These are controller/development-fixture results, not club selection, a standalone app or acceptance of the pending semantic shared engine. The inspector reports raw engine time units and the live actor work-queue count; the latter can include repeated actors and must not be presented as the number of players on the pitch.

The three complete advancement bodies have been reconstructed in C++23: watched-match orchestration, background advancement and the shared advancement state machine. Focused component checks cover 1,189 comparisons, including 967 fault or interrupted prefixes and 670 instruction-budget stops, with a negative control for each entry. The affected units also pass UBSAN. An independent original live fixture agrees with 24 productive steps of the reconstructed shared engine in complete guest state, ordered memory effects and instruction accounting. These are bounded entry results; dependent football routines still retain their existing implementations.

The production hooks, original-selection controls and diagnostics are integrated into the fresh complete ARM64 build and installed game. Seven focused CTests pass, including actual caller composition, direct/generated/CFM controls and nonlocal transfers. The installed application passes an owned menu and normal-exit check; that menu check does not execute a match. [The advancement report](native-match-advancement-reconstruction.md) records the individual entry evidence and installed executable identity. The standalone debugger application and arbitrary-club match creation are still required deliverables.
