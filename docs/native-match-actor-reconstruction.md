# Match injury withdrawal and position access reconstruction

> Classification correction (2026-10-12): the original `101f1530` body described
> below is dismissal, with actor `+0x23` red count and position `-3`. Its
> semantic body/ABI are unchanged and its source name is now `dismiss_player`.
> This historical report mislabeled it as injury. Current Injury handling uses
> the complete original pending-injury processor `101f6c60`; see the
> [card and injury correction report](native-match-debugger-cards.md) for current
> behavior and focused verification. The evidence below for `101f1530` is
> evidence of dismissal, not of the corrected Injury control.

## Result and bounded scope

Three complete original entries now execute named C++23 behavior in both the installed game and standalone debugger:

| Entry | Behavior | Original indexed instructions |
| --- | --- | ---: |
| `101f1530..101f185c` | Injury withdrawal, time/count bookkeeping and team/goalkeeper reorganization | 203 |
| `101f1860..101f1868` | Unsigned position-byte read | 2 |
| `101f1870..101f1878` | Position-byte write with original truncation | 2 |

The semantic manifest has **604** replacements. Its complete accepted 601-entry prefix is unchanged. The same 1,953,334 instructions, 1,922 pages, root, five source fixups and zero missing/pending targets remain. Only generated `page_101f1.cpp` changes. The `State` header and guest-facing layout remain unchanged; new diagnostics extend its owned private metadata.

`ActorCall` maintains ordered observation points for original stack argument homes, volatile/nonvolatile registers, flags, instruction charges, memory faults and suspended native frames. Structured helpers own withdrawal bookkeeping, period caps, team-controller selection, goalkeeper/tactical updates and team-change stamps. Remaining football dependencies retain their existing implementations and call ordering. No recorded match or second football model is a production factory input.

Original byte quirks remain intact: the getter zero-extends; writes truncate; signed byte flags and team indices retain their original interpretation; withdrawal counts wrap; raw time is divided with the original signed high-product/shift sequence, converted to a signed byte and capped according to original period codes. The nonzero injury argument retains the quick-caller path that skips already-performed count/time bookkeeping. Aliasing and interrupted prefixes are not normalized.

## Entry evidence

The independent original reference contains all 207 indexed instructions with semantic hooks and native transfer routing disabled. Pinned complete-body/fragment fingerprints reject missing or changed operations and external interior entrances. Seven focused audits pass. Updated manifest/scope checks retain prior entry membership and fingerprints.

Component comparisons cover **562 cases**, including **231 interrupted/faulting outcomes**, three own negatives and a position-child negative. They compare all architectural register/temporary bytes, every mapped memory byte, ordered memory effects, complete dependency argument/return boundaries and exact accounting. Variants cover detailed/quick injury flags, four period caps and signed clock boundaries, goalkeeper/tactic paths, both and unusual team bytes, unsigned/truncated position values, argument/match aliases, wrapped addresses, each accessed byte becoming unmapped, budgets and dependency interruptions.

Actual fresh Arsenal/Manchester United production checks cover **36 comparisons**, three negatives and six budgets across direct, generated and CFM calls and global/per-entry original controls. Each entry has nonzero dedicated coverage; the injury body composes both reconstructed position children. Nine separate profile comparisons execute original bodies and suppress modern root coverage. Profile fixtures are captured from the fresh ordinary prerequisite solely for this diagnostic run: enabling global original profiling during strict modern bootstrap would intentionally fail its initializer-composition check. Production match creation continues to use the original database and a new world.

The real withdrawal/session check compares home/away outfield and goalkeeper interventions against original bodies, with complete match records, registers/temporaries, allocations, ordered effects and accounting from each initialized match boundary. The 30 following shared-engine steps agree with the original, and the selected actors stay unavailable. Invalid/repeated edits, partial budget faults, independent worlds and seeded reset retain their existing proofs. The worker protocol also executes real withdrawal, before/after logging and later steps, and retains the second-pair 100-step injury/history regression.

Twenty-one actual nonlocal comparisons include twelve restore/handler faults. Original and modern injury frames and unframed position scopes abandon the same host continuations, resume the same surviving parent, preserve complete guest state/ordered effects, and leave no dispatch escape or original-body residue. Reconstructed and original scalar-context dependencies and the turbo control are covered.

UBSAN instruments the new body, adapter, owned diagnostics and focused harnesses. Component, actual production, profile and nonlocal modes pass; unaffected production dependencies are not newly instrumented. Six focused CTests pass individually after the profile prerequisite correction. No broad historical suite, career or season campaign is a gate.

## Build, installation and remaining work

Generation renders and audits all pages. The 1,922 identical generated objects, including the loader, are reused only after source/flags/object identity checks; the changed actor page and affected native units are compiled. Both application bundles retain their original program resources, are ARM64 and pass strict ad-hoc signature verification. Candidate and installed executable identities are recorded privately. Original database and current career files are preserved.

Current installed executable SHA-256 values are `ede6e4882489095502d4466bd07b29b4d1f31ac30216e0695e7e31bc38041f7c` for the game and `df84f05fdd2fa3d574c7a317e0abb4953ec296a0145b6bbccca48d26d2dd589f` for the debugger.

Installed GUI/menu/withdrawal verification remains **pending while the Mac is locked**. Per-entry runtime evidence above comes from the actual engine harnesses rather than a menu smoke. Save/Load is **not retested**; performance is **not benchmarked**. Owned backings, sanitizer/probe/check binaries and obsolete generated objects are cleaned after acceptance while preserving apps, current build inputs, sources, references and receipts.

The whole engine port and debugger product remain unfinished. Goal injection, pre-kickoff lineup editing, red-card/substitution controls and remaining football dependencies are still required. [The debugger contract](native-match-debugger.md) and [injury control report](native-match-debugger-withdrawal.md) distinguish semantic reconstruction from host controls and pending visual acceptance.
