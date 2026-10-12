# Player decision conversion: original characterization in progress

The next complete engine body is `0x102836a0..0x10285748`, with **2,090 indexed instructions** across three pages. It is not yet a production semantic replacement. The current production manifest remains **642**, with the accepted marking/pressure block unchanged.

The source body has 87 direct calls to 30 distinct dependencies. Its branches are forward-only, and the full source-index audit finds no external entrance into the interior. Complete-body and page-fragment fingerprints are pinned in `tools/player_decision_audit.py`. The whole-body digest is `0e6502a309eb476fbaba0bbde1274e23622b3fba464efb4bb7ec5aca2f094ae6`.

## Complete scope being converted

The body combines dead-ball priorities, live tactical context, team-oriented defensive-line choices, ball-pursuit probability, marking-target validation/following, destination selection, goal-area and emergency-keeper handling, goalkeeper actions and outfield actions. These are characterization names; raw trait offsets and action codes are not assigned new football meanings without original evidence.

The call frame is 256 bytes. GPRs 13–31 begin at old SP minus `0x5c`; FPRs 30/31 are saved at old SP minus `0x10/0x8`. The integer saves therefore include the 16-byte displacement for the floating saves. Original memory order, mutable TOC tables, signed/narrowed traits, stream draws, floating conversion and repeated dependency calls remain part of the future semantic contract.

## Independent original reference

`tools/make_player_decision_reference.py` emits all 2,090 original labels and checkpoints, with semantic and context hooks disabled. It rejects missing, changed, duplicate, reordered and unrelated instructions. Call probes preserve all 87 original target/argument boundaries. The generated source, original IR/listing and snapshots remain private.

The new `cm-player-decision-characterization` target compares this independent original reference with the currently generated original body. Both use the same production football/RNG/dependency implementations. It creates a fresh Arsenal/Manchester United world from the actual database, advances three shared-engine calls, and examines both sides' 11 starters after calls one and three. Additional private clones exercise designated/loose dead balls and marking-target alternatives.

The accepted matrix has **61 comparisons**:

- **50 normal returns**, with full architectural/temporary bytes, guest mappings and memory, heap/import state, ordered effects, native import boundaries and exact instruction accounting agreeing.
- **Eight instruction-budget prefixes**, with complete original-state/effect agreement.
- **Two native-event boundaries**. These paths await native interaction and are not counted as normal returns. The fixture stops before supplying any event.
- **One observable architectural negative**, rejected by the full-state comparator.

Presentation consumes host time. Both executions receive the same monotonic `Microseconds` input, sampled from the native epoch and advanced by one second per request; the initial value, interval and number of samples are recorded. Other services and rendering remain real dependencies. Headless characterization explicitly stops at `WaitNextEvent`/`GetNextEvent`, preserving the interrupted prefix. It neither invents a click nor claims interactive acceptance.

The matrix observes **869 of 2,090 original instruction sites** and 13 dependency targets. All 11 named regions are entered, but many branch alternatives remain unobserved. This is bounded original-characterization evidence, not complete branch coverage or proof of the new semantic implementation.

Seven source/reference audit tests and the focused characterization CTest pass. The characterization harness and independent reference also pass UBSAN with empty stderr; existing dependency libraries are not newly instrumented. The CTest takes 8.83 seconds in this run; that is a check duration, not a performance benchmark.

## Semantic work in progress

A private, unhooked C++23 prototype implements call-frame handling and dead-ball priority selection. Two actual home/away designated-receiver paths and 12 budget prefixes agree with the generated original in complete guest state, ordered effects and accounting. The prototype and shared ABI helper instantiations pass UBSAN with empty stderr. The 16-byte integer-save displacement was corrected from original ABI evidence before that acceptance.

Remaining decision phases explicitly report that they are unimplemented. They do not delegate to their own original body as a substitute for reconstruction. The prototype is not added to the manifest, bundled or installed. Completion still requires the entire 2,090-instruction decision contract, remaining branch/dependency cases, entry negatives, production dispatch/profile/nonlocal proof and relevant integration checks.

Current installed apps and all 818 original game files/four career files are preserved. Disposable test objects, executables, failed-run scratch backings and superseded trial captures are cleaned after this development cycle, recovering 1,397,952,512 bytes (1.40 GB) of available storage; the accepted original fixture, source, references and receipts are retained. Save/Load is **not retested**. Performance is **not benchmarked**. The full C++23 engine/debugger goal remains active.
