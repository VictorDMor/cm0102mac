# Descriptor-tree mutation reconstruction

## Production scope

Three complete Mac bodies reconstruct descriptor-tree root insertion, recursive
insertion/replacement and postorder cleanup as named C++23 operations. Existing
lookup, scalar allocation, byte-string comparison, CFM callbacks and descriptor
lifetimes retain their actual implementations. No replaced original body is
called by the semantic implementation. Aggregate construction and other
collection methods remain separate generated dependencies.

Complete generation appends three `tree_mutation_adapter` records after the
accepted 570, giving **573** semantic replacements. The previous records remain
identical, with digest
`7c46bda5a1297c19d312785558d5b5dbec2beff2c1c06fd9e11a1aa5cbbb7256`.
The full manifest digest is
`4253c73fd9b0353e0d74f023c540b0386c11d144bfa001649a30956def5e0598`.
The 1,953,334 indexed instructions, 1,922 pages, roots and five source fixups are
unchanged. There are zero missing or pending targets. The full index rejects
exterior edges into any replacement's interior. Destination hooks precede
original operations, covering local, cross-page and indirect dispatch.

## Complete boundaries

Intervals are end-exclusive; fingerprints use the established address,
assembly, ops, flows and fallthrough serialization.

| Entry | End | Instructions | Operation | SHA-256 |
|---|---|---:|---|---|
| `0x1074df40` | `0x1074df5c` | 7 | Root insertion facade | `6b17d64817d46902a10f05d960e158dea98a784510fa612a30de80fed28ab0ec` |
| `0x1074e030` | `0x1074e148` | 70 | Recursive insertion and equal-key replacement | `45446bff05f78f8c1b86c837ff02143b24d9c135f1db0265444497818720c8c6` |
| `0x1074ea30` | `0x1074ed70` | 208 | Recursive postorder cleanup | `ff2a32c6a790558435fd1d19729cd0b73aa7ca283c01c3146f6bcd7b26fc350d` |

Total: **3 entries / 285 indexed instructions** across two pages. Missing,
changed, reordered, duplicated or wholly absent bodies and missing, duplicated
or misplaced hooks have audit negatives. Original records and generated sources
stay private.

## Preserved semantics and ABI

A node contains payload, metadata and two child pointers at offsets 0/4/8/12.
Insertion uses the live descriptor comparator vector at +4, signed word32 result
and the original direction: negative descends +8, positive descends +12. It
reloads the node reference after comparison callbacks. Empty insertion allocates
16 bytes through the actual scalar facade, writes the reference and increments
the live count at descriptor +12, even when allocation returns null. Count wraps
as word32.

Equal keys allocate a replacement, preserve both live child pointers, invoke the
descriptor deletion vector at +8 with flag zero and publish the replacement.
Count stays unchanged. Failed equal-key allocation still invokes deletion and
publishes null. These original failure behaviors are retained deliberately.

Cleanup visits children before their parent. Four expanded levels share each
96-byte guest frame; deeper children use recursive semantic dispatch. The second
child is read after first-child callbacks. The original first deep call inherits
live r3/r5/r6; later calls prepare the saved owner/vector/flag explicitly. The
callback receives the cached node and full word32 flag, followed by the live
TOC reload from the caller slot. Cleanup does not normalize descriptor root or
count. Cycles, invalid links and callback mutations retain original behavior.

The seven-instruction root facade has no LR capture/save of its own: it consumes
the original caller saved-link slot. Guest frames, multiple-register address
cursor, saves/restores, SP/TOC/CR/XER/LR/PC, GPR/FPR and temporary bytes, ordered
access prefixes, allocator metadata and finite budgets are preserved. No host
container replaces the guest tree. The small site table describes the original
expanded traversal's ABI placement; it does not interpret opcodes.

Tree selection/counters live in the existing opaque `RuntimeDiagnostics` owner.
The State header/layout stays unchanged. Const queries allocate nothing; resets
preserve accepted utility/initializer diagnostics. Global/per-entry original and
profile selection retain balanced recursive boundaries. Normal production uses
semantic bodies; `CM_AOT_TREE_MUTATION_ORIGINAL` is explicit development selection.

## Focused checks

The component harness compares **2,196** cases, including **2,101** faults,
**1,073** exact budget stops and three own ABI negatives. Its private original
reference contains exactly 285 instructions, with hooks disabled and the 18
original same-page recursive calls exposed at their existing dispatcher
boundaries. Each isolated body stops at its own architectural return; a child
that escapes its expected continuation fails at that boundary. This avoids
confusing identical recursive return PCs with completion of the current body.

Controlled scalar allocation and CFM callbacks record exact arguments and
observable effects. Independent expectations cover a 31-node postorder tree,
empty cleanup, every pruned expanded branch, deeper recursion, callback-mutated
right links, signed comparison, equal replacement, allocation failures, count
wrap, full-word flags, live SP/TOC/address-cursor mutations and owner/stack aliases.
Minimal mappings and each individually missing accessed byte retain fault
prefixes. No fault or divergence is normalized to make equivalence pass.

Eight focused CTests pass: four new tree checks, the three directly related
utility production/profile/transfer checks and collection-lookup equivalence.
Generator audits pass 222 tests with zero final failures/skips; the three
fixture-dependent audit classes skipped by discovery were subsequently run with
the explicit original index. The original prefix and each older audit's scope
remain fixed; only the global total changes.

Actual production fixtures pass 45 direct/generated/CFM/global/per-entry/negative
comparisons and three original-profile comparisons. Identical original
prerequisites use actual scalar allocation, original insertion, a CFM byte-string
comparator and scalar deletion on a seven-node chain. Empty insertion, both
search directions, equal replacement and cleanup beyond the expanded levels are
covered. Existing descriptor lookup finds every inserted/replaced node. An
application-exit prefix check distinguishes termination from fault/return.

Another 23 known nonlocal comparisons include twelve restore/handler faults,
three frames, mixed scalar-context selection and Turbo. The surviving parent
import is controlled; tree, allocation, comparison and deletion dependencies
remain actual. No abandoned callback/parent continuation runs, and no unknown
context transfer occurs. Seven affected units pass UBSAN across component,
production, profile and transfer modes with empty stderr and exact Release output;
existing dependencies are linked from the consistent current production library.
All 2,029 State-dependent production objects were compiled consistently.

## Installed delivery and retention

The signed ARM64 candidate, installed bundle and actual running executable share
SHA-256 `7a4ed5b5b47583e4a44fc1d9188c751d52f0ac7230b8b7e44090dabb3505b675`.
Strict signature and all four bundled payload checks pass. The owned installed
main-menu/native-menu-cancel/original-quit-confirmation/normal-exit smoke records
**53** semantic tree calls and returns: 12 root insertions, 25 recursive insertions
and 16 cleanup calls. All three entries execute, with zero original/profile calls,
faults, budget stops, transfers, terminations or dispatch escapes. The process
exits normally. No saved game is loaded, written or deleted. The existing
scaled-content window is retained and the observed menu content is 800×600.

Original game files, private inputs, runtime files/saves, all 23 handoffs,
references and 199 prior application bundles retain their metadata. The prior
installer source is preserved by rename. Intentional run logs/preferences can
change. After acceptance, 4,118 reproducible old objects/dependencies, test
executables and static archives are removed, recovering approximately
1.65 GB of actual free space. The latest consistent build, all application
bundles, sources, inputs and receipts remain; approximately 56.0 GB is available.

Save/Load is **not retested** and performance is **not benchmarked**. No career,
day, season, match or historical heap campaign is part of this delivery. The full
engine/runtime reconstruction goal remains active. Reproducible outputs from the
previous accepted cycle are cleaned only after this candidate passes acceptance;
apps, originals, references, receipts, handoffs and user data remain protected.
