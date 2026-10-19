# Actor coordinates, heading, turning and possession reconstructed

## Production result and scope

Five complete original entries / 462 indexed instructions now execute named C++23 operations in both ARM64 production apps. The block migrates actors between live per-side pitch-cell lists, updates ball coordinates when its actual carrier moves, chooses and adjusts player headings, reads headings, tests whether a player can face a destination, and queries actual possession. The manifest is **648**, preserving the entire 643-entry prefix; its canonical full SHA-256 is `ad73754c9c759159ac6dc22a40d7392bd94d25283af8bf4c1c323f0c721f65f0`.

The complete 7,537-instruction movement/interaction coordinator at `1022e7e0..10235da4` remains original. It now uses these production C++23 primitives. Its independent original characterization passes 25 whole-body cases and 129 computed-prefix cases; 621 coordinator sites are observed on normal paths. That evidence characterizes the original coordinator and its live tables; it does not claim a semantic replacement or full coordinator branch coverage.

## Original contracts

The source pins serialize address, assembly, operations, flows and fallthrough in ascending address order. End addresses are exclusive; each body has zero static exterior interior entrances.

| Entry | End | Instructions | Operation | SHA-256 |
|---|---|---:|---|---|
| `102374f0` | `10237830` | 208 | set_coordinates | `5835c3c6b493d66f75df87effaa4966dfbad7d8c90580ac8af793bf0ca876135` |
| `10237830` | `10237af4` | 177 | set_heading | `babf2e8401ba3e2b72c1bf2e642e166c3308cbe74e2a65e033b259a2db858caf` |
| `10237b00` | `10237b08` | 2 | heading | `0bb0a15d0183c71c0ffe27e477356df45d260d24db61d35f043dfa8ce2f02b66` |
| `10245050` | `1024515c` | 67 | can_face_destination | `eb619cd852d0303b7cf9ddf7d51863d0f20af1dcc67d025ee08b5a8da36ca65e` |
| `102775c0` | `102775e0` | 8 | has_possession | `4cdc4f3028167bb137cdf43c839a08f37f5d509cc568b98339695189c9bde78a` |

The coordinate operation preserves signed byte coordinates and side/list indices, original 32-bit address arithmetic, repeated live reads, and the unordered cell-list removal rule: move the old last member into the departing member's position and repair that member's index. It separately registers previously unregistered actors. A missing old member count retains the original diagnostic path; a new cell exceeding its original capacity retains its diagnostic and termination path. There is no additional host clamp, repair or null guard. Original diagnostic dependencies remain external.

Heading selection reads the original live direction table, handles both goal-center directions, or draws an original random heading for equal non-goal coordinates. Its trait-dependent jitter retains the precise cursor/limit/seed reads, replenishment calls and subsequent samples. Player directional bias and the original single wrap correction remain ordered; this is not a new general modulo policy. The turn predicate retains its original signed deltas, angular folding, floating threshold, integer narrowing and external absolute-value dependency. The added multiply/add ABI operation preserves the indexed semantics' separate rounding with contraction disabled.

Named coordinate migration, registration, heading jitter, directional-bias and turn phases execute the bodies without replaying their own original instructions. The coordinate method composes the complete possession method through the ordinary guest call boundary. Explicit guest ABI operations preserve interrupted prefixes, volatile/nonvolatile and floating registers, condition/status bytes, allocation state, return PC, effects, dependency order and accounting. The existing native scope machinery handles nonlocal restoration, including leaf scopes with no new stack frame.

## Focused proof

Independent original references cover all five complete bodies with semantic hooks disabled. There are **427 component comparisons**: 131 ordinary positive returns, 291 budget/dependency/fault outcomes and five discriminating negatives. All **462 original instruction sites** are exercised. The matrix covers both sides, old-cell member positions, destination occupancy, missing membership, original overflow termination, equal coordinates, goal centers, signed traits/headings, every attempted-prefix budget boundary on representative paths, and all three heading-stream replenishment positions. Full architectural state, mapped guest memory, ordered accesses/effects, dependency ABI and instruction accounting agree.

Actual original database matches provide **60 production cases**: 45 direct/generated/CFM comparisons using default C++23, global-original and individual-original controls; five observable return negatives; and ten actual budget stops. **15 original-profile controls** exercise the same entry boundaries under profiling. Dedicated per-entry counters prove default semantic execution and reject silent original fallback; original-scope and dispatcher-escape residues are zero. **35 nonlocal comparisons**, including twenty restore/handler faults, preserve surviving parents and abandon interrupted continuations for every entry, including both leaf scopes.

Seven focused CTests pass: component equivalence, actual production, original profile, nonlocal transfer, the original coordinator characterization, debugger lab and actual worker protocol. Twenty-two focused source/reference/hook audits and 63 catalog-scope checks pass. The audited full generation retains 1,953,334 instructions, 1,922 pages, its original root, all five fixups and zero missing/pending targets. Only three generated pages and the compilation manifest change; the current reusable object tree is retained.

UBSAN checks the changed semantic body, adapter and component/production/nonlocal harnesses with empty sanitizer stderr. Other production dependencies are not newly instrumented. Current debugger clock, event history, goals, injury and unrestricted substitutions remain covered by directly related lab/worker checks.

## Installed acceptance and preservation

Both strict ad-hoc signed ARM64 apps are installed through metadata-preserving APFS clones and protected atomic swaps. Their previous signed bundles are retained privately. Candidate/installed internal-executable identities and all four packaged original-program files are verified. Installed game acceptance observes the original menu in its 800×600 content window, the original quit confirmation and a logged normal exit. Menu match counters are zero; entry coverage comes from the individual actual production checks. The installed debugger creates a real Arsenal/Manchester United experiment and reaches 01:00 with original turns, passes and headers; it exits with its worker and private root removed. Candidate, installed and running internal-executable identities agree: game `076a6becb670672b7fde28d9e6a0f9a957ed3356e1338e71c2853330a5d9768c`, debugger `a3b45eb7a4ed5c56e9b4c8cf6b9553c24b35aca52ca27e0c4e93ea57be2d1d69`. All 845 protected original game/runtime/save/handoff files are verified unchanged.

The accepted cycle removes 21 validated disposable targets and recovers 1.68 GB. Reusable production objects, accepted apps/references, current private fixtures and source/runtime evidence remain available. Original game/database files, career runtime files, saves and private handoffs remain protected. Save/Load is **not retested**; performance is **not benchmarked**. The whole match engine, complete movement coordinator and complete native port remain unfinished; the full goal remains active.

## Later target/corridor caller acceptance

The [target acceptance block](native-target-acceptance-reconstruction.md) appends four
owners after the exact 949-owner prefix, for 953 owners. Existing bodies/owners remain unchanged,
including actor-motion `0x10245050`. Its 12,477 component comparisons, 87 actual-service
comparisons, twelve profile controls and selected composed UBSAN pass. Returning
native coordinate/area/side/distance composition is explicit. No actual generic
sampler caller composition is claimed. Whole-engine reconstruction remains open;
SaveLoad not retested, performance not benchmarked, PDF updates paused at edition 39.
