# Actor marking relations and local pressure reconstruction

## Delivered complete scope

Thirteen complete original entries / 396 indexed instructions now execute structured C++23 in the game and standalone debugger. The block maintains actor marking lists, chooses a tactical marking trait, qualifies a potential marker and computes weighted opposing-player contributions in the subject's current pitch cell. Optional candidate output uses the original guest collection operation and copied actor records.

The production manifest contains **642** semantic replacements. The complete accepted 629-entry prefix remains byte-for-byte unchanged, SHA-256 `e9e5838c664486a19e08cd3a5cd6dfe7df43a9dde77aab6f18d55afe1324a289`. The full 642-entry digest is `f2efaafb37166e008c7ed153d56bff3b63f9a8d80e872fe247500410d36ca6e0`. Generation retains 1,953,334 instructions, 1,922 pages, the existing root/five source fixups and zero missing/pending targets.

| Entry | End exclusive | Instructions | Behavior |
| --- | --- | ---: | --- |
| `101d5660` | `101d5668` | 2 | Unsigned actor trait at +10a |
| `101d56a0` | `101d56a8` | 2 | Signed action-code halfword |
| `101d5700` | `101d5730` | 12 | Append a marker with original signed-count bound |
| `101d57d0` | `101d5804` | 13 | Store/clamp pressure adjustment |
| `101d7da0` | `101d7da8` | 2 | Unsigned actor trait at +10d |
| `101d7dc0` | `101d8100` | 208 | Weighted local opponents and top-two candidate collection |
| `101d8100` | `101d8108` | 2 | Signed pressure adjustment |
| `101d8110` | `101d8118` | 2 | Unsigned position byte |
| `101d8120` | `101d8254` | 77 | Marker eligibility gates |
| `102373c0` | `1023741c` | 23 | Tactics-selected actor marking trait |
| `10283540` | `10283588` | 18 | Search marking list |
| `10283590` | `10283614` | 33 | Remove first matching marker by tail replacement |
| `10283690` | `10283698` | 2 | Unsigned team byte |

Complete-body and page-fragment canonical IR fingerprints are pinned in `tools/marking_pressure_audit.py`. Index auditing finds no external interior entrances. An independent private original reference contains all 396 original labels with semantic/context hooks disabled. Original payloads, listings, generated code and captured state remain private.

## Original contract

The actor marking-list count is byte `+104`, with word pointers beginning at `+1b4`. Append checks the **signed** byte count against 3, then reloads it, increments/stores the byte and writes the pointer selected by the signed old byte. It does not deduplicate. Negative counts are not newly clamped: count 255 (-1) wraps to zero and its index aliases actor `+1b0`, the marking target. A callback/alias can also change the reloaded count after the bound check; the source behavior is retained.

Search reloads the signed count during each iteration and returns 1 for the first pointer equality, otherwise 0. Removal finds the first equality, returns immediately through the original conditional-LR branch if absent, and otherwise replaces the slot with the original last member and decrements the reloaded count. It does not clear the stale tail. Complete GPR/CR/XER/PC residues, including the absence path's conditional return accounting, are retained.

Pressure adjustment is the signed halfword at actor `+1a0`. Its setter first narrows/stores the incoming word, then clamps the reloaded signed halfword to -200..200. Both original reloads and the early upper-clamp branch remain observable. Trait getters preserve their unsigned byte contracts and the action-code getter sign-extends actor `+19c`.

The tactical trait uses team `+27`, owner `+1a4` and current-position byte `+19` to call the original team-tactics query `101149f0`. Mask `0x2` chooses byte `+10d`; otherwise the result comes from `+10b`. These offsets are characterized data contracts, not new football-skill interpretations.

The eligibility chain preserves null-actor rejection, separate row/column comparisons, signed action-count `+2c < 2`, exclusion of the original match pointers at `+e000/+dff8`, nonzero tactical trait and zero interruption byte `+7928`. It returns the original narrowed boolean, with no new movement or safety policy.

The cell assessor accepts match, subject actor, signed-halfword sum output and optional guest collection in r3–r6. It resets the sum before computing `match + 0xa08 + row*0x450 + column*0x5c`, then selects the opposing team group and reloads its signed byte count on every iteration. Inactive position bytes are excluded. A listed marker contributes weight 2 to the returned contributor count; an eligible ordinary opponent contributes 1. The running sum narrows after each contribution.

Marked contributions combine the original tactics-selected trait, `+10d`, the subject's `+10a` and signed adjustment divided by 20. Ordinary contributions preserve their original activity, goalkeeper and trait/action-code gates, then divide the scaled result by signed activity count plus one. The mutable original double table, multiplication/division order, bias conversion, `fctiwz`, status bytes, truncation and failure behavior remain intact. Marked ranking doubles the already-converted integer, narrows it to signed16 and retains the original greater-than-5/top-two/tie policy.

If a collection is supplied, the original append `10738d10` is called first for the best candidate and then for the second, each with size `0x1c4`. This is a guest list of copied actor records, not a host vector or two writes to one output buffer. The original list append/record constructor/allocator remain dependencies and are not counted as reconstructed entries in this block.

## ABI and focused evidence

`marking_pressure.cpp` uses named list, eligibility, contribution and ranking phases with the shared explicit guest-call ABI. `SemanticGuestCall::finish_if_equal` preserves the removal entry's conditional-LR instruction without inventing a second return instruction. All existing helper behavior remains intact. The constructor/spatial bodies that instantiate the shared helper are rebuilt consistently; the `State` header/layout is unchanged.

Leaves remain unframed. Tactical trait lookup uses a 64-byte frame, eligibility 96 bytes and cell assessment 144 bytes. Root hooks precede the first original operation at every destination. Global/per-entry original selection, profile-original selection, dedicated normal/return/fault/budget/transfer/termination counters and original-body guards remain independent from other modules. Unknown escapes and diagnostic paths do not count as normal coverage.

- **1,674 component comparisons:** 1,054 normal returns, 606 interrupted/fault prefixes and 14 observable own/child negatives. All 13 entries have successful original-versus-C++ cases. Full architectural/temporary bytes, mapped memory, ordered effects, dependency ABI boundaries, candidate payload/order and exact accounting agree. Cases cover narrowed aliases, list limits/negative counts, absent/found removal, adjustment clamps, tactical switches, cell counts, activity/role gates, FP factors/NaNs, eligibility, per-access holes, budgets and interrupted dependencies.
- Independent expectations establish contributor weight 4, sum 65, the two selected records in the expected order and size, and the count-255 append alias into the marking target.
- **157 actual production comparisons:** every entry executes directly, through generated dispatch and through CFM, with global/per-entry original controls, 13 entry negatives and 26 budgets. Dedicated normal C++ counters are nonzero for every entry, with zero selected-original/profile/escape residue on normal modern paths. A controlled clone of a real guest world forces two ranked opponents and verifies both actual original collection appends, allocations and copied records through full-state/effect/accounting comparison.
- **39 original-profile comparisons:** all 13 entries through the three paths, using a private fresh-match profile prerequisite; these are original-control evidence, not modern execution.
- **91 nonlocal comparisons**, including 52 restore/handler faults, cover each framed/unframed entry, original/modern scalar restoration and turbo controls. Surviving handler state, effects/imports and accounting agree. Abandoned C++/callback continuations do not run.

Seven focused CTests pass, including the directly affected spatial component, actual withdrawal with 30 following original-agreeing steps and the real two-process worker protocol. New marking-pressure and affected spatial audits each pass seven checks; 34 changed historical manifest-assertion methods pass without unrelated historical campaigns. UBSAN instruments the new body/adapter, changed helper instantiations, affected spatial/constructor bodies and diagnostic storage, plus three new harnesses. Component, actual production and nonlocal runs pass with empty sanitizer stderr. Original references and other existing dependency libraries are not newly instrumented.

Five generated sources changed; **1,918** compatible objects were reused through APFS cloning after source/flags/layout checks. The unchanged State header SHA-256 is `352fe18e156aaa51cfd4aefca141cdceb699ff0dae97bb217c5d597a5a435f6e`.

## Installed artifacts and remaining scope

Both installed bundles are strict-verified ad-hoc signed ARM64 apps with four unchanged original program resources. Candidate and installed internal executable identities agree:

| Installed app | Executable SHA-256 |
| --- | --- |
| `/Users/victordmor/Applications/CM Match Debugger.app` | `b768779ad3069ebb680a5e0b23ef39682ab850991abbeefa68f1dbac35d9952c` |
| `/Users/victordmor/Applications/CM 01-02 Native Prototype.app` | `1c70b710332d545b60b046f8ff0a2b63c166ae2faf9746d61ae53fb19369ec2c` |

Protected prior apps and installation copies use APFS clones with Mac metadata preserved. Installed-worker checking uses its packaged original program to create a real match, advance, award a goal, withdraw the outfield scorer with original marking cleanup and advance twice more. Normal quit leaves empty scratch backing. This is headless installed-worker evidence; final visual/AppKit acceptance remains pending after the last locked-Mac observation, with no subsequent unlock reply.

All 818 original game files, four current career/runtime files and private feature handoffs remain unchanged. Disposable check executables, sanitizer objects/backings and the superseded generated object tree are removed after acceptance. Measured available storage increased by 1,544,970,240 bytes (1.54 GB) during this cleanup. Apps, current libraries/objects, sources, references, receipts and the private profile fixture are retained.

The full movement/action engine, remaining tactics/collection dependencies, red-card controls and complete commentary decoding remain unfinished. The broader C++23 port goal remains active. Save/Load is **not retested**. Performance is **not benchmarked**.
