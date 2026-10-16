# Actor spatial membership and tactical role reconstruction

The later [query-performance delivery](native-spatial-role-performance.md)
preserves this complete contract, adds unobserved and observer-fallback coverage,
and measures the optimized 682-entry game build. The original reconstruction
checkpoint below records its historical 629-entry delivery.

## Result and complete entry scope

Twenty-three complete original entries / 754 indexed instructions now execute structured C++23 in the production game and standalone match debugger. This block covers tactical role lookup, raw position/coordinate and marking access, center-cell insertion, cell/mark relation removal, cached goalkeeper invalidation and both-side goalkeeper resolution/emergency replacement.

The semantic manifest contains **629** entries. Its accepted 606-entry prefix is byte-for-byte unchanged, SHA-256 `299c69ca6943599eaf4489e8bda2facc8fbeb6ecda654c89b8541a232bb616e0`. The complete 629-entry manifest SHA-256 is `e9e5838c664486a19e08cd3a5cd6dfe7df43a9dde77aab6f18d55afe1324a289`. Generation retains 1,953,334 instructions, 1,922 pages, the existing root/five fixups and zero missing/pending static targets.

| Entry | End exclusive | Instructions | Behavior |
| --- | --- | ---: | --- |
| `101d3b50` | `101d3b58` | 2 | Current position byte |
| `101d5680` | `101d5688` | 2 | Column byte |
| `101d5690` | `101d5698` | 2 | Row byte |
| `101d56c0` | `101d56c8` | 2 | Marking target |
| `101d5730` | `101d5738` | 2 | Set marking target |
| `101d5740` | `101d574c` | 3 | Clear marking-list count |
| `101d7db0` | `101d7db8` | 2 | Marking-list count |
| `101e3d50` | `101e3ddc` | 35 | Current primary role |
| `101e3de0` | `101e3e6c` | 35 | Current secondary role |
| `101e3e70` | `101e3efc` | 35 | Previous primary role |
| `101e3f00` | `101e3f08` | 2 | Previous position byte |
| `101f1880` | `101f192c` | 43 | Register in center cell |
| `101f1930` | `101f1938` | 2 | Set column byte |
| `101f1940` | `101f1948` | 2 | Set row byte |
| `101f1950` | `101f1dc0` | 284 | Remove cell membership, marking relations and appoint replacement |
| `101f1dc0` | `101f1dd4` | 5 | Indexed marking-list member |
| `101f1de0` | `101f1e0c` | 11 | Clear cached goalkeeper |
| `101f1e10` | `101f21c0` | 236 | Resolve both goalkeeper caches, including emergency replacement |
| `101f21c0` | `101f21c8` | 2 | Set previous position byte |
| `10289550` | `10289558` | 2 | Cached signed role |
| `10289560` | `1028959c` | 15 | Owner current-role wrapper |
| `102895a0` | `102895dc` | 15 | Owner previous-role wrapper |
| `102895e0` | `1028961c` | 15 | Owner secondary-role wrapper |

All complete-body and page-fragment canonical IR SHA-256 pins are versioned in `tools/spatial_role_audit.py`. The index audit rejects external interior entrances; this source has zero such edges. Independent private original references contain all 754 source labels with replacement/context hooks disabled. No original bytes, listings, generated translation units or captured state are published.

## Observable contract

Role lookup performs the original repeated position calls: lower-bound check, upper-bound check and final index extraction remain distinct. The third read is not newly clamped or assumed equal to the first two. Current/previous position comes from actor `+19/+1a`; role tables are guest pointers at match `+7934/+793c`, indexed by the signed low team byte and doubled signed low position byte. The selected halfword is sign-extended. Owner wrappers retain their original non-null test and a separate owner-pointer reload before delegation; a null owner returns zero. `+1c2` is the separate cached signed role value.

The original geometry uses unsigned coordinate-byte getters followed by caller-side signed narrowing: column `+106`, row `+105`. Cell address calculation is word32 `match + 0xa08 + row*0x450 + column*0x5c`. Center insertion sets row 8/column 6, then writes membership into match `+2eb0`. Each cell has team pointer groups with stride `0x2c`, byte counts at `+58/+59` and actor membership index `+28`. Removal swaps the last member into the departing slot, decrements the original count and updates the moved actor's index. These are trusted original operations; no host bounds clamp, list rebuild or new population policy is added.

Actor marking target is `+1b0`, indexed marking-list pointers start at `+1b4`, and the list count is byte `+104`. Removal preserves the original relationship-query/removal dependencies, repeated list/member reads, reciprocal target clearing and counter reset. The remaining relationship implementations at `10283540/10283590` and team gate at `101da5a0` are not part of this reconstruction.

The departure reason is interpreted through its signed low byte. Reason `-1` retains the original no-replacement path; reason `-3` and cached-keeper identity reach the original replacement policy, subject to role/team gates. Candidate iteration, flag exclusions, priorities 70,000/50,000/15,000/-5,000, 5,000 bonuses, ties, diagnostics and original position-setter calls are preserved.

Keeper caches are match `+e01c/+e020`. Resolution always processes home and away in original order before returning the side requested by the signed low byte (1 selects away, other values select home). It invalidates inactive cached actors, finds active role-1 actors, retains the unavailable former keeper when appropriate and selects the first active emergency candidate. Original previous-position changes and tactical-slot exchanges remain observable. No new minimum-player or fallback-success rule is introduced.

## Implementation and ABI

`spatial_role.cpp` uses named cell, marking, role and keeper phases. `SemanticGuestCall` centralizes the existing explicit guest ABI checkpoints and optional component dependency boundary. The constructor's previous checkpoint implementation is extracted into this shared helper; its behavior is unchanged and its focused original-equivalence regression is rerun. Indexed access now supports the original byte/halfword/word widths needed by this block. This is an explicit semantic-call helper, not an opcode interpreter.

Guest arithmetic, narrowed widths, ordered reloads, temporary residues, condition/overflow bytes, LR/TOC/PC, spills and fault/budget prefixes are retained. Leaves remain unframed; role/owner/cell-insertion frames are 64 bytes, keeper resolution 96 bytes and removal 160 bytes. Destination hooks precede the first original instruction at every root, preserving generated/indirect/CFM dispatch and per-entry/global/profile-original controls. The opaque diagnostics gain an independent module; the `State` layout/header remains unchanged. Nonlocal transfers abandon interrupted C++ continuations and epilogues.

## Focused acceptance

- **2,014 complete-entry component comparisons:** 1,096 normal returns, 894 interruptions/fault prefixes and 24 observable own/child negatives. Every entry has a normal original-versus-C++ path. Architecture, all mapped memory, ordered effects, dependency boundaries and exact accounting agree. Cases include widths/high-bit aliases, repeated/changed role reads, null owners, marking lists, cell counts, role/team/reason decisions, cached/unavailable keepers, per-access byte holes, budgets and dependency interruptions.
- **277 actual production comparisons:** all 23 entries execute directly, through generated dispatch and through CFM, with global/per-entry original controls, 23 own negatives and 46 budget boundaries. Each entry has a dedicated nonzero normal C++ counter; normal selected-original/profile/escape counts are zero. An actual injured-home-keeper scenario agrees with the original emergency resolution.
- **69 original-profile comparisons:** all 23 roots through all three paths. The prerequisite is a privately captured fresh actual match; profile-only checks do not use the fresh-world bootstrap or claim modern execution.
- **161 productive nonlocal comparisons**, including 92 restore/handler faults, cover each unframed/framed entry, original/modern scalar restore and turbo controls. Full guest/effect/import/accounting state agrees; abandoned parent/callback continuations do not run.
- The directly affected injury-withdrawal and awarded-goal checks pass, including complete original operations and 30 following original-agreeing shared-engine steps in each check. The two-process real worker protocol passes with existing creation, roster/lineup, goal, withdrawal, reset and cleanup behavior.

Eight focused CTests pass. The new spatial-role and affected constructor audits each pass seven checks; 33 changed historical manifest-assertion methods pass without repeating unrelated historical campaigns. UBSAN instruments the new spatial body/adapter, shared helper instantiations, affected constructor bodies/adapter, diagnostic storage and four harnesses. Spatial component, actual production, nonlocal and constructor component runs pass with empty sanitizer stderr. Original references and existing dependency libraries are not newly instrumented.

Seven generated sources changed. Two additional constructor pages were conservatively recompiled because their public helper dependency changed. **1,914** unchanged source/flags/layout-compatible objects were reused by APFS cloning. The unchanged `State` header SHA-256 is `352fe18e156aaa51cfd4aefca141cdceb699ff0dae97bb217c5d597a5a435f6e`.

## Installation, storage and remaining work

Both installed apps are strict-verified ad-hoc signed ARM64 bundles with four unchanged original program resources. Candidate and installed executable identities agree:

| Installed app | Executable SHA-256 |
| --- | --- |
| `/Users/victordmor/Applications/CM Match Debugger.app` | `ad8583929b02f8d644a7a108a516f808f3b17dfcd9fdce20fb04f21a029afc1f` |
| `/Users/victordmor/Applications/CM 01-02 Native Prototype.app` | `0433e125576aa6b77a91d32583295df3fb3078fe59723cc3bf5bee20ca77414a` |

Protected previous-app references and temporary installation copies use APFS clones with Mac metadata preserved, reducing recurring backup storage. Original game files, current career/runtime files and feature handoffs are unchanged through installation/cleanup. Disposable test executables, sanitizer objects/backing directories and the superseded generated object tree are removed after acceptance; current apps/libraries/objects, sources, references, receipts and the private profile fixture are retained.

Installed-worker acceptance exercises its packaged program, actual reserve creation/promotion, original goal, goalkeeper withdrawal, actual replacement keeper and a following original-engine step. Normal quit leaves empty scratch backing. This is installed headless-worker evidence. Final visual/AppKit acceptance remains pending: the last attempted observation found the Mac locked, with no subsequent unlock reply. No new GUI or coordinate-editing control is claimed by this block.

Physical movement, remaining football/marking dependencies, red-card controls and a complete decoded commentary inspector remain unfinished. The whole engine and complete port goal remain active. Save/Load is **not retested**. Performance is **not benchmarked**.
