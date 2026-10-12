# Complete match actor construction reconstruction

## Delivered behavior

The original player-preparation entry and its complete match actor initializer now execute structured C++23 in the production game and standalone debugger. The initializer establishes identity and ownership, player-record fallback, attributes, fitness, availability and position, club affiliation, eligibility/history fields, names, random preferences, personality and competition/coach adjustments, kit information and final defaults. The debugger's existing roster editor uses these same production entries.

| Entry | End exclusive | Indexed instructions | Canonical IR SHA-256 |
| --- | --- | ---: | --- |
| `1020e8f0` | `1020e9f8` | 66 | `9cabd8c6fafdaef743ef162bb4171a8ab66ca550dd4b2be8d792a149e5420544` |
| `10223ac0` | `10226520` | 2,712 | `263340be710966e6996d21df30a30a76d07c3b68259c6ba6b5c0298974adfc80` |

The manifest contains **606** semantic replacements. Its complete accepted 604-entry prefix is unchanged, SHA-256 `1e4935c4e56744dd4024373da1a601be905d5bd79540ea70241b93ac830163b6`. The 606-entry manifest SHA-256 is `299c69ca6943599eaf4489e8bda2facc8fbeb6ecda654c89b8541a232bb616e0`. Generation retains 1,953,334 instructions, 1,922 pages, the prior root/five source fixups and zero missing/pending targets.

These are complete entry bodies, rather than wrappers delegating their replaced entry to generated code. Existing football, fitness, attribute, club, history, mathematics, diagnostic and RNG refill/rounding dependencies remain outside this block. This does not complete the whole match engine or debugger product contract.

## Contract and implementation

`actor_construction.cpp` uses named business phases and shared draw policies, with explicit architectural checkpoints in `actor_construction_abi.hpp`. Guest memory remains big-endian with original 32-bit wrapping and narrowed widths. There is no host player model, captured-state factory, opcode table or generic instruction interpreter in the implementation. The independent original reference is generated privately from all 2,778 pinned instructions with semantic and context-transfer hooks disabled.

Preparation accepts controller/index/staff/final-index in r3–r6. Its index validation uses the signed low byte, preserving high-bit aliases and the original diagnostic/exit path. A non-null staff record is passed to the initializer; a null staff argument returns zero. Its 128-byte frame and r26–r31 saves remain intact.

Initialization accepts actor/match/staff/team/index/final-index in r3–r8. The final argument is copied to the actor's original identifier byte `+2f`; it also supplies the original starting-position decision. It must not be repurposed as a debugger-selected position. Actual lineup editing restores a desired old position separately after construction. Staff/player pointers remain at `+6c/+70`, team at `+27`, position at `+19` and match ownership at `+1a4`. Availability result 2 retains original position `-2`; ordinary index bytes 0–10 use their original position/role path; other bytes use the original bench initialization. Initialization retains its 208-byte frame, r13–r31 saves and f30/f31 saves.

Pool consumption preserves pointer/epoch reloads, refill order, signed remainder rules, large-bound double conversion, rounding-helper calls and final clamps. Probability bands share an explicit C++ implementation while retaining their original allocations and instruction boundaries. Floating operations preserve separate multiplication/subtraction rounding, single rounding where required, original `fctiwz`, status bytes and ordered/unordered branch behavior. The legacy special-club zero-controller-address test is preserved and exercised with a valid split mapping; no gameplay correction is inferred from that unusual condition.

The production adapter hooks both generated destination labels before their first original instruction. Global/per-entry original selection and profile-original selection retain diagnostic counters and guarded original dispatch. The opaque `RuntimeDiagnostics` gains this module without changing `State` or the generated ABI. Unknown dispatch escapes, faults, budget stops, context transfers and application termination remain distinct from normal returns. Nonlocal restoration abandons the interrupted C++ frame rather than executing its epilogue.

## Focused evidence

- **1,782 complete-entry component comparisons:** 116 normal cases, 1,663 interrupted/fault prefixes and three observable own/child negatives. Comparisons include all architectural/temporary bytes, mapped memory, ordered reads/writes, dependency register boundaries, instruction accounting and allocator/import state. Cases exercise identity, position narrowing, fitness/injury paths, affiliation, eligibility/date/national contexts, name fallback, coach/competition pressure, probability bands, refill, FP exceptions/NaNs, mapping holes and 1,001 finite-budget prefixes. Normal cases execute 2,067 original instruction addresses; this is not claimed as every-instruction runtime coverage.
- **759 prior isolated preparation/RNG comparisons:** full ABI, memory/effects/calls/accounting, zero/signed/large bounds, pool refill and fault/budget prefixes. These characterized the shared policies; they do not independently establish complete initializer acceptance.
- **24 actual production comparisons plus six profile comparisons:** both entries execute directly, through generated dispatch and through original CFM calls, with global/per-entry original controls, two own negatives and four budget boundaries. Dedicated normal C++ counters are nonzero for both entries; preparation also reaches the C++ initializer. Selected original/profile controls do not count as modern execution. Original-scope residue and normal dispatch escapes are zero.
- **14 actual nonlocal comparisons:** two entry frames, original/modern scalar context restoration and turbo controls, including eight restore/handler faults. Surviving handlers agree, and abandoned parent/callback continuations do not execute.
- The directly affected real lineup check passes: original reserve/starter construction and position edits agree in complete guest state, effects and accounting, followed by thirty original-agreeing shared-engine steps. The actual worker protocol passes for independent club pairings/processes, editing, goals, withdrawal, reset and normal scratch cleanup.

Six focused CTests pass. New construction and previous match-actor audits each pass seven checks; 32 changed historical manifest-assertion methods pass without rerunning unrelated historical campaigns. UBSAN instruments all new construction bodies/ABI helpers, the adapter, changed diagnostic storage and the three new harnesses. Component, actual production and nonlocal runs pass with empty sanitizer stderr; existing original/reference and dependency libraries are not newly instrumented.

Only `page_1020e.cpp` and `page_10223.cpp` changed. The 1,921 unchanged generated objects were reused after source/flags/header checks through APFS clones. The unchanged State header SHA-256 is `352fe18e156aaa51cfd4aefca141cdceb699ff0dae97bb217c5d597a5a435f6e`.

## Installed artifacts and limits

Both installed apps contain strict-verified ad-hoc signed ARM64 executables and four unchanged original program resources. Protected previous apps are retained privately. Candidate and installed executable identities agree:

| Installed app | Executable SHA-256 |
| --- | --- |
| `/Users/victordmor/Applications/CM Match Debugger.app` | `60faf56cc0c93416d48381678b55eeabc626592703e4d37c566dead424b663f5` |
| `/Users/victordmor/Applications/CM 01-02 Native Prototype.app` | `6a68b032ed0fe5fcdfc577d9f1573e57ee91c3a7d653129bb92af0ba891c72f3` |

The installed debugger's real worker uses its packaged program to construct a fresh match, create Patrick Vieira in reserve actor 11, promote him, advance, apply an original awarded goal to the new actor and advance again. Normal quit leaves its experiment backing empty. This is installed-worker evidence; the final visual/AppKit-control check remains pending because the Mac is locked and no new unlock reply has arrived. The game GUI has not been reopened in this block.

All 818 original game files, four current career/runtime files and private feature handoffs are unchanged through installation and cleanup. Disposable check executables, sanitizer objects/backings and the superseded generated object tree are removed after acceptance; source, references, receipts, both apps, current libraries/objects and the private profile fixture are retained. Save/Load is **not retested**. Performance is **not benchmarked**.
