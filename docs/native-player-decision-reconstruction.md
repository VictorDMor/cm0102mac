# Complete player decision reconstruction

The complete original `0x102836a0..0x10285748` body—**2,090 indexed instructions** across three pages—is reconstructed in C++23. The production manifest is **643**, preserving its entire accepted 642-entry prefix (`f2efaafb37166e008c7ed153d56bff3b63f9a8d80e872fe247500410d36ca6e0`). The new full manifest digest is `3f290aa60e8a00b40e6bceba6d3abfbe86e6a32413fa267d7ec7c2e07a7c1398`.

## Complete semantic contract

`player_decision.cpp` owns named phases for dead-ball priority, tactical context/special destinations, team-oriented defensive-line choices, ball-pursuit probability, marking-target validation/following, destination selection, goal-area/emergency-keeper handling, goalkeeper actions and outfield actions. It maintains their original ordering and early returns. No phase dispatches its own original body or interprets an opcode table. Remaining movement, tactical queries, physical actions, commentary queues and formatting remain production dependencies, not reconstructed entries counted in this block.

The actor's action halfword is reset and assigned at its original points. Marker invalidation preserves the original negative-position distinctions. Team-dependent bounds, repeated live owner/trait/table reads, scalar and scaled-float random ranges, signed remainders, jitter/clamps, single-precision rounding and original floating status remain observable. Physical capture and event operations keep their original call ABI and ordering; the host does not substitute a scoreboard update. Raw trait offsets and action/message codes are not given invented football meanings.

The 256-byte call frame saves GPRs 13–31 at old SP minus `0x5c` and FPRs 30/31 at old SP minus `0x10/0x8`. The integer saves retain their additional 16-byte displacement. Full returns and interrupted prefixes preserve the original stack, architectural and temporary bytes, memory accesses, dependency boundaries and accounting.

The shared guest-call helper adds single-precision products/indexed loads, register flag combinations and the original less-or-equal condition-bit operation. Optional instruction/dependency observers are process-local and absent by default. All affected helper users and diagnostic storage are rebuilt consistently; mixing the old/new helper class definitions was rejected by UBSAN during private development. `State` and its serialized layout are unchanged, SHA-256 `352fe18e156aaa51cfd4aefca141cdceb699ff0dae97bb217c5d597a5a435f6e`.

## Source boundaries and production integration

The body has 87 direct calls to 30 distinct dependencies, forward-only branches and no external interior entrance. Complete-body/page-fragment fingerprints are pinned in `tools/player_decision_audit.py`; the body digest is `0e6502a309eb476fbaba0bbde1274e23622b3fba464efb4bb7ec5aca2f094ae6`. The independent original reference retains all 2,090 labels with semantic/context hooks disabled. Original IR, listings, generated sources and snapshots remain private.

The generated destination-label hook precedes the first original instruction. Normal production uses C++; global/per-entry original selection and original-profile mode are private diagnostic controls. Dedicated calls, returns, faults, budgets, context transfers, termination, original/profile calls and escape counts are separate. Original-body guards prevent diagnostic reentry from being counted as modern execution. Opaque diagnostic storage grows without changing the 1,922 generated units' `State` ABI.

Only `page_10283.cpp` changes in generation; **1,922** compatible generated objects are reused through APFS cloning after source, flags, `State` and transitive helper checks. Generated units do not instantiate the changed guest-call helper; affected native users are rebuilt. Generation retains 1,953,334 instructions, 1,922 pages, five existing source fixups and zero missing/pending targets.

## Focused evidence

- **374 component comparisons:** 103 normal returns, 269 interrupted/fault/termination prefixes and two observable own/child negatives. Full architecture, mapped memory/heap state, ordered effects, exact ordered instruction sites, instruction accounting and complete dependency entry/return registers agree. Cases include both teams, goalkeeper/outfield captures, event alternatives, defensive-line direction/flags, live marking, integer/scaled-float ranges and clamps, negative positions, signed seeds, NaNs/infinities, per-access byte holes, finite budgets and dependency interruptions. The child negative is observed at its return boundary before the parent's continuation.
- **57 actual production comparisons:** home/away goalkeeper and outfield records execute through direct, generated and CFM paths, including global/per-entry original controls, an architectural negative and two budgets. There are 55 normal returns (including the negative's completed invocation). Dedicated modern counters are nonzero with no selected-original/escape residue; full guest state, ordered effects, native import boundaries and accounting agree.
- **18 original-profile comparisons** execute the three paths from a private fresh-match prerequisite. They prove the control path, not modern coverage.
- **Seven nonlocal comparisons**, including four restore/handler faults, preserve scalar restoration, the complete floating/integer-saved frame, turbo controls and abandoned-continuation behavior.

The original characterization remains available separately: 61 comparisons include 50 returns, eight budget prefixes, two pending native-event boundaries and one negative. In that original-reference matrix, presentation receives identical monotonically advancing `Microseconds` input and stops at native event requests. Pending UI paths are not normal returns and no fictitious click is supplied. The component matrix observes **1,864 of 2,090** source instruction sites; this is substantial discriminating evidence, not a claim of 100% branch coverage. Some signed-byte range alternatives are structurally unreachable with ordinary source inputs; their original code paths remain implemented.

Ten focused CTests pass, including the affected construction/spatial/marking components, original characterization, actual root/profile/nonlocal paths, withdrawal with 30 subsequent original-agreeing steps and the two-process worker protocol. Eight player-decision audits, seven marking audits and 34 changed historical manifest methods pass. UBSAN instruments the new body/adapter, shared helper users, diagnostic storage and the three root harnesses; existing dependency libraries are not newly instrumented. All three sanitizer runs pass with empty stderr.

## Installed artifacts and cleanup

Both production bundles are ad-hoc signed, strict-verified ARM64 apps. Candidate and installed internal executable identities agree:

| Installed app | Executable SHA-256 |
| --- | --- |
| `/Users/victordmor/Applications/CM Match Debugger.app` | `97197ff64cd06aabfe2b3500751a22163c1a8b39ffb02a2466bcae91e3c90c34` |
| `/Users/victordmor/Applications/CM 01-02 Native Prototype.app` | `e8701a6e1652b16cdcb1677a2818476616a76c93ed48ed940495f42c2b1b4e3d` |

Protected previous bundles and installation copies use APFS clones with Mac metadata preserved, followed by an atomic protected swap. All four original program resources remain unchanged, with no validation setting in the production plist. Installed-worker checking uses the packaged program, creates a real match, advances, awards a goal through the original operation, withdraws the scorer, advances twice more and quits normally with empty backing and stderr. This is installed headless evidence, not interactive acceptance.

All 818 original game files, four current career files and private feature handoffs remain unchanged. The cycle removes disposable check/sanitizer executables and objects, owned scratch/trial captures and the superseded generated object tree. Measured available space increases by **2,796,003,328 bytes (2.80 GB)**. Current libraries/1,923 generated objects, apps, sources, protected references, accepted fixtures and acceptance receipts remain available.

## Remaining scope

This is one complete decision body, not the whole match engine. The full movement/action dependencies and remaining runtime port are unfinished. The standalone debugger continues to use the same production engine. Installed interactive/AppKit verification remains pending after the last locked-Mac observation; headless worker evidence cannot replace it. Save/Load is **not retested**. Performance is **not benchmarked**. The broader goal remains active, with disposable build/check artifacts cleaned after each accepted cycle while preserving original game files, careers, private sources, references and receipts.
