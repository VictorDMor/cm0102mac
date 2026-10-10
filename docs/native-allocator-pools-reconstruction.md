# Semantic Mac guest arenas, small pools and allocation facades

## Scope and reference

This block reconstructs 27 allocator entries as named C++23 arena, block, pool,
context and allocation operations. Guest addresses, word32 big-endian metadata,
ordered memory accesses and the existing Carbon storage bridges remain in use.
No host malloc/free/realloc/calloc or host container replaces the guest heap.
Lifecycle/atexit/init/teardown, acquisition/disposal glue, stream initialization
and Carbon allocation policy are dependencies, outside these 27 replacements.
No visual or performance improvement is claimed.

The starting checkout and freshly queried origin/main are
`e9577b4cc4e92630d6918d6ca3e7a2c5611dd116`. The 23 handoffs, including 18 private
untracked prompts, are preserved. The original executable pin is
`91692f95df17cf851e7c1d6dbdecd0bd0a5809ce1937133e209aee859a6b516b`.
The private SQLite export is read only; listings, bytes, generated sources,
heap snapshots, receipts, logs, saves and bundles are excluded from publication.

The oracle is the existing productive translation with its historical pool
progress correction. The raw Mac loop and that correction are distinct:
`0x10774c24` calls `next_pool_attempt(previous, candidate)` only after an
existing-arena allocation failed. With previous >= 10, it returns
`min(previous - 1, candidate)`. Already decreasing attempts are unchanged;
repetition/increase is capped to reach the existing growth fallback. The new
semantic branch calls the same helper at the same access/calculation site.
The generator's independent original-branch audit is retained unchanged.
The historical captured 213-arena heap is neither replayed nor reinvestigated.
The handoff's earlier guarded 16-case check is preparation evidence, separate
from this block's new equivalence and independent progress evidence.

Save/Load and persistence are **not retested**. Career/day/week/season/match,
serialization, benchmarks and the whole test suite are outside this acceptance.
Date's ascending-year leap asymmetry and all adjacent subsystem behavior remain
unchanged. Neither waived persistence nor historical acceptance is reported as
new evidence for this allocator.

## Pins and production integration

Pins serialize only address/assembly/ops/flows/fallthrough, in address order,
with sorted keys and compact UTF-8 JSON. Counts are indexed instructions,
including intentional gaps rather than padded spans. The total is 1,057.

| Entry | End exclusive | Instructions | Functional role | SHA-256 |
|---|---|---:|---|---|
| `0x107740f0` | `0x10774184` | 37 | Arena initialization | `899991252ae8d0a90551f0d0e27aff502b549b8bc4734cf233ca038dd0418874` |
| `0x10774190` | `0x10774268` | 53 | Arena block allocation | `e91c37ea32029bb0a5850ad914d8f1b92978176ffd0354232ee6ab753b74bb94` |
| `0x10774270` | `0x10774354` | 57 | Arena block release | `a9badebe7c44a6c29d520e42ed9646e8853cefea5f1c061abefc797c46349278` |
| `0x10774360` | `0x107743d4` | 29 | Free-block unlink / allocation marking | `46db492d7b8bf0117155650d71021ceaa443cc7a0d7bc047a4b254c062969afa` |
| `0x107743e0` | `0x10774430` | 20 | Block header / boundary-tag initialization | `defee65552080892727defd8cffa736f0841303341b081347028c8ca5c09315c` |
| `0x10774430` | `0x107744d0` | 40 | Block splitting | `1b3759e1aad753dacd69eb8885ee1fe5e54b128f41865de89a2bdd167090aa7a` |
| `0x107744d0` | `0x10774568` | 38 | Previous-block coalescing | `002e8f3b47aced8869ba50259b0213cafc05c9a9a29d9c046a6f6ab10e09f933` |
| `0x10774570` | `0x1077462c` | 47 | Next-block coalescing | `346e27dbb2ba8cc8f11e08e50205fcec3ad3f26c0f9b7821ab440509d618cec5` |
| `0x10774630` | `0x10774674` | 17 | Arena-list insertion | `ad89c9bf7c181551a63b09da1151ed8545f3ba7be7172c500149ca8438cd0995` |
| `0x10774680` | `0x107746cc` | 19 | Arena-list removal | `064156d3d9f352309ea1335ba3b8a7083de97ea14cc9a21317d85609f101b39b` |
| `0x107746d0` | `0x10774754` | 33 | Arena acquisition / growth | `5a4266e828c346e4cf92a6f4a48982cc12eae20181fee480e1b0d35391ca526f` |
| `0x10774760` | `0x1077483c` | 55 | Allocation with arena growth | `f888e846ac6fded2020fef449ad6121319c015714e9ea42b384b4d5b86608fd2` |
| `0x10774840` | `0x1077491c` | 55 | Existing-arena allocation / usable-size hint | `233fa935bc02c70159aab33616bcb29fb98bc4c8e2234abaf7a331c72c5bed23` |
| `0x10774920` | `0x107749ac` | 35 | Ordinary-object release / empty-arena disposal | `651472f54721c0bfe36e403a82b80c3f5db907e2d0db84e90ae039385ae5a87c` |
| `0x107749b0` | `0x10774ad4` | 73 | Small-object pool initialization | `860f062b04be068f9350cc9a7a6360d5d98ece352b00594f7f9c897ee1655fd2` |
| `0x10774ae0` | `0x10774b08` | 10 | Allocator-context clearing | `6f079c08c2fa85903a15178d319cc28b97303b6cd66cce7b213720fe871eb8cd` |
| `0x10774b10` | `0x10774b58` | 18 | Lazy global-context access | `19336e3f73c506ecd4a97f7814da2c88c081d95e671f38d7d2294b6d949fde69` |
| `0x10774b60` | `0x10774d20` | 111 | Small-object allocation / pool replenishment | `179cd6ecfc7129e22dfb7642b997dcee4c75e71cba1de3640ca676dad800fca0` |
| `0x10774d20` | `0x10774e74` | 85 | Small-object release / pool disposal | `daf82fb8c1795572a2102f5374813720c4b6171917059acebcec855369fec39a` |
| `0x10774e80` | `0x10774ed4` | 21 | Context allocation dispatcher | `abeb332fa94f94fd97cda4eac9f5fc82fd5764ff1611d06cfb7dd76904c34a5b` |
| `0x10774ee0` | `0x10774f38` | 22 | Context release dispatcher | `224588c051e02fca7eabd023d99ef087e9d3cd6fe85bb9d767dd40176ab260db` |
| `0x10774f40` | `0x107750e0` | 104 | Context reallocation | `de1e23660c8ab1f486b7b861ad2631aa25b9341609065cce13fab2c20bdd3ce7` |
| `0x107750e0` | `0x1077512c` | 19 | Context allocation and clearing | `95446dda867a8c76c3dd7075c60f4bc164662749615e3e9d0a2e0993e8393e2b` |
| `0x10775130` | `0x10775164` | 13 | Global allocation facade | `8c93c7ba18df1a72fbb8a46d81d3e5373180e9f65b65f34ab4777a08f298263f` |
| `0x10775170` | `0x107751a4` | 13 | Global release facade | `56a985f44c579aafb7ee77d6a06cd133a3770dfdd43a1c6edbd4e5c4ec60e71e` |
| `0x107751b0` | `0x107751f4` | 17 | Global reallocation facade | `fd300973f33c95758ade4fc7b0d9da5b0b8836635c55a7a3fb6987486d206be9` |
| `0x10775200` | `0x10775240` | 16 | Global count-times-size clearing facade | `d1840afa4b2dc77cb1c531b1f4a85af844c37bd7f07c66d65f032dd856455056` |

Reallocation crosses pages: the `0x10774` fragment has 48 instructions, pin
`4c2c60a384d74408d7a49906fb09f30964ec7191ab11857a4c60eabff422bb7e`;
the `0x10775` fragment has 56 instructions, pin
`20e909d26bf62b6c1bec4877f0024a4e49b6b6756c4b5800c576bd49c0172e54`.
Full-body and fragment checks reject changed, incomplete or duplicate records.
A completely absent entry or page cannot emit a full manifest. All indexed flows
and fallthroughs are audited for external interior edges: zero exist. Unknown
indirect targets are not inferred from this static statement.

The production default reaches all 27 bodies through hooks at generated
destination labels, before the first original operation. Local goto, cross-page
and CFM indirect dispatch share these hooks. The original diagnostic selection
retains the guarded translated bodies. Semantic bodies call only named children
or the existing external generated/bridge dependencies, never their own replaced
bodies. Selection, reset, original/semantic counters, resume and unknown-transfer
accounting are private and process local, with no serialization change.

The manifest appends these 27 entries in ascending order after the exact prior
100, giving 127. The prior compact JSON SHA-256 remains
`dee0bb555af0657479f3defa0e8a2b763aabd5717e716a213d9c25d8c75004b3`.
Earlier historical module slices and reports are unchanged.

## Layout and semantic contracts

Arena +0/+4 are the circular links, +16 is the largest-free-block hint, +20 is
the size/tag sentinel and +24 is the first block. The free-list head is at
arena + masked arena size - 4; the end sentinel is eight bytes before the end.
Block +0 is size/flags, +4 is owner with low bit 1, and free blocks have links
at +8/+12 and a boundary tag four bytes before their successor. Size is masked
with `~15`; flag value `2` marks in-use and `4` marks previous-busy. Ordinary user
pointers are raw block +8. Metadata and payload residue follow the original.

Arena search follows the circular free list, retains the maximum observed size
on unsuccessful traversal, and splits only when the remainder is at least 96.
A null free-list head returns zero without revising the largest-block hint.
Unlinking the last free node clears both head and hint. Release clears in-use,
updates the next block/previous boundary tag, inserts into the free list and
coalesces previous then next. Conditional early returns and live rereads retain
the original single-node and aliased behavior. Releasing the final ordinary
object unlinks and disposes its arena through the unchanged generated glue;
the native bridge's mapping remains mapped after DisposePtr.

Arena growth requests `(needed + 47) & ~15`, minimum 65,536. Ordinary helpers
round `(size + 23) & ~15`, minimum 96, with word32 wrap. Existing-only allocation
clears its out hint before reading the arena head and reports the largest usable
hint observed. Growth allocation retains the original fit/list rotation and
failure behavior, including its internal post-allocation pointer adjustment.
No new validity or double-free policy is introduced.

Context clearing calls the existing semantic fill for exactly 52 bytes. Lazy
context lookup loads the pointer through `r2-0x3b74`, tests the byte at
`r2-0x3510`, clears once and sets that live flag. Relocated TOC and mutable guest
context contents remain observable; there is no host singleton.

The class table is live through `r2-0x3b70`, with baseline 4/12/28/44/60/76.
Each context class uses two circular-list heads at +4/+8 plus class index *8.
Pool +0/+4 hold links, +8 usable class size, +12 free-slot head and +16 live
count. Slots begin at pool +28, have stride class size +4, owner immediately
before payload, and freelist next stored in free payload. Initialization receives
header/neighbors/class/first-slot/slot-byte extent in r3..r8. It divides extent
by live stride, retains eight-link store groups, scalar remainder, CTR and the
last-slot termination. A zero extent is not reinterpreted as an empty safe pool.

Replenishment starts from `0xfe4 / stride`, with the initial slot target capped
at 256. The actual usable ordinary block extent determines the slot count, including
rounding/slack. Allocation pops a free slot, increments live count and rotates
heads when full; free uses the caller's observed usable size to find the live
class row, relinks full pools when needed, pushes the slot and decrements count
with the original carry/CR effect. An empty pool is unlinked and released through
the ordinary arena path. Guest ownership is separate from State's native bridge
allocated/available maps.

The context dispatcher returns zero for size zero, routes sizes <=76 to pools,
and rejects sizes >=`0xffffffb8` before ordinary allocation. Release is a null
no-op; owner low-bit determines where usable size is read, and that size selects
the release path. Malformed headers retain matching finite mapping faults.

Reallocation uses context/pointer/new-size in r3/r4/r5. Null allocates; zero
releases. Small shrink retains the same slot. Ordinary shrink may split and free
a remainder; ordinary growth first attempts next-block coalescing, then splits
surplus or moves through allocate -> BlockMoveData -> free. Moving copies exactly
the old observed usable size and frees only after successful allocation/copy.
A failed allocation retains the old payload; any preceding permitted coalescing
remains observable. Common-to-small shrink keeps the ordinary classification.
There is no host realloc or copy beyond the original observed extent.

Allocate-and-clear fills only a successful returned pointer and only the requested
size. Global allocate/free receive one argument in r3, global reallocate receives
pointer/size in r3/r4, and counted clear receives count/element-size in r3/r4.
Each obtains the guest context through the lazy helper and preserves its own
frame/spills. Counted clear uses the low word32 product, including wrap to zero
or a small nonzero count; no checked multiplication changes that legacy contract.

## Mac ABI and observable effects

Frame backchains, LR saves/restores, nonvolatile spills, GPR scratch, CR, XER,
CTR, LR/PC/TOC, FPSCR, FPR and register/temporary bytes are compared without
normalization at returns and faults. `stmw/lmw` also retain the generated
architectural transfer cursor. Compare versus recorded-operation CR differences
and pool-free carry remain explicit. Generated local scratch stays instruction
local; State temporary residues are not reset to make comparisons pass.

| Entry | Frame bytes | Saved nonvolatile GPRs |
| --- | ---: | --- |
| `0x107740f0` | 80 | r29-r31 |
| `0x10774190` | 64 | r30-r31 |
| `0x10774270` | 64 | r30-r31 |
| `0x10774360` | 0 | none |
| `0x107743e0` | 0 | none |
| `0x10774430` | 96 | r25-r31 |
| `0x107744d0` | 0 | none |
| `0x10774570` | 0 | none |
| `0x10774630` | 0 | none |
| `0x10774680` | 0 | none |
| `0x107746d0` | 80 | r29-r31 |
| `0x10774760` | 80 | r29-r31 |
| `0x10774840` | 80 | r28-r31 |
| `0x10774920` | 64 | r30-r31 |
| `0x107749b0` | 0 | none |
| `0x10774ae0` | 64 | none |
| `0x10774b10` | 64 | r31 |
| `0x10774b60` | 96 | r24-r31 |
| `0x10774d20` | 64 | none |
| `0x10774e80` | 64 | none |
| `0x10774ee0` | 64 | none |
| `0x10774f40` | 80 | r26-r31 |
| `0x107750e0` | 64 | r30-r31 |
| `0x10775130` | 64 | r31 |
| `0x10775170` | 64 | r31 |
| `0x107751b0` | 64 | r30-r31 |
| `0x10775200` | 64 | r30-r31 |

The harness clones identical guarded-original fixture history, compares every
architectural byte and mapped guest byte, ordered scalar reads/stores and bulk
bridge effects, allocation/available maps, heap_end and memory_error. A private
runtime observer also compares import arguments, returns, mapping/storage history
and native-call counts at unchanged import dispatch. Only instruction accounting
and private diagnostics are excluded. Access PCs preserve fault locations and
all stores preceding faults. NewPtr/DisposePtr wrappers and BlockMoveData/fill
remain their existing productive dependencies.

## Focused acceptance

The final harness passes **337 comparisons**, including **86 expected unmapped
access faults** and **30 rejected negatives** (27 own entries, three child-before-
parent controls). All faults explicitly require the unmapped-access diagnostic;
instruction-budget exhaustion cannot count as acceptance. Minimal mappings,
removed accessed bytes, malformed links, zero pool extent, relocated frames/TOC,
header/slot/hint aliases and live class mutation retain architectural/access prefixes.
The first independent coalescing expectation incorrectly used 128 bytes for a
100-byte request; the guarded original establishes 112, consistent with its
specified rounding. That fixture expectation was corrected before final acceptance.
No production allocator divergence or additional legacy repair was introduced.

Independent checks cover fit/split thresholds, circular lists, merged boundary
tags and reciprocal links, occupied neighbors, all six pool classes, class/size
boundaries, full/nonfull rotation, slot reuse, refill and empty-pool disposal.
All 16 synthetic hint/replenishment combinations are compared against the guarded
reference and independently require progress, preserved live payload and no overlap.
Candidate retry counts below/equal/above previous retain the exact cap. Acquisition
failure returns zero with the existing memory error, without fabricating storage.
No captured historical heap is used.

Sequences cover global allocate/write/reallocate/free and counted clear, the
existing semantic string duplication caller, null/zero, small/ordinary shrink,
shrink with/without split, in-place growth, moving growth and failure preserving
the old payload. Live neighbors survive and clearing retains unrequested class
slack. Counted-clear wrap-to-zero and wrap-to-small-nonzero use low word32 results.
Twenty-seven generated CFM transitions reach the same destination hooks. Fourteen
mixed selections compare the guarded original against original-parent/semantic-child
or semantic-parent/selected-original-child paths. Diagnostic reset is independent.

| Normal entry witness | Semantic calls | Direct fixture | Original calls | Escapes |
| --- | ---: | ---: | ---: | ---: |
| `0x107740f0` | 48 | 1 | 0 | 0 |
| `0x10774190` | 1242 | 1 | 0 | 0 |
| `0x10774270` | 113 | 1 | 0 | 0 |
| `0x10774360` | 189 | 1 | 0 | 0 |
| `0x107743e0` | 348 | 1 | 0 | 0 |
| `0x10774430` | 146 | 1 | 0 | 0 |
| `0x107744d0` | 39 | 1 | 0 | 0 |
| `0x10774570` | 45 | 1 | 0 | 0 |
| `0x10774630` | 51 | 1 | 0 | 0 |
| `0x10774680` | 45 | 1 | 0 | 0 |
| `0x107746d0` | 47 | 1 | 0 | 0 |
| `0x10774760` | 124 | 1 | 0 | 0 |
| `0x10774840` | 1162 | 1 | 0 | 0 |
| `0x10774920` | 58 | 1 | 0 | 0 |
| `0x107749b0` | 116 | 1 | 0 | 0 |
| `0x10774ae0` | 8 | 1 | 0 | 0 |
| `0x10774b10` | 28 | 1 | 0 | 0 |
| `0x10774b60` | 929 | 1 | 0 | 0 |
| `0x10774d20` | 867 | 1 | 0 | 0 |
| `0x10774e80` | 1011 | 1 | 0 | 0 |
| `0x10774ee0` | 884 | 1 | 0 | 0 |
| `0x10774f40` | 42 | 1 | 0 | 0 |
| `0x107750e0` | 13 | 1 | 0 | 0 |
| `0x10775130` | 5 | 1 | 0 | 0 |
| `0x10775170` | 5 | 1 | 0 | 0 |
| `0x107751b0` | 4 | 1 | 0 | 0 |
| `0x10775200` | 9 | 1 | 0 | 0 |

Counters above include only successful normal comparisons with no original
selection, excluding faults, controls and mixed diagnostic selections. They prove
per-entry execution; installed UI observations have their own narrower boundary.

One full regeneration emits 1,953,157 instructions / 1,922 pages, zero missing or
pending targets and 127 semantic replacements. The consistent Release ARM64 build
rebuilds all State-dependent units. Final source and harness compilation is clean
under the project's warnings-as-errors settings.

| Executed check | Result |
| --- | --- |
| `modern_allocator_equivalence` | PASS: 337 comparisons, 86 expected mapping faults, 30 negatives, all 27 witnesses |
| `original_allocator_replenishment` | PASS: the retained 16-case matrix explicitly selects the guarded translated allocator |
| `modern_string_core_equivalence` | PASS: directly used duplication/string dependency regression |
| `modern_memory_primitives_equivalence` | PASS: directly used copy/fill dependency regression |
| `native_bridges` | PASS: retained native storage/bridge checks |
| New allocator audit | PASS: seven body/fragment/missing-entry/hook/edge/manifest/guard tests |
| Nine affected prior audits | PASS: 46 tests; prior slices/pins remain intact, total expectation now 127 |
| Focused UBSAN | PASS: new allocator and modified runtime/probe sources, new harness and retained replenishment harness; three binaries, empty stderr |

The four related CTests passed in 12.78 seconds; the final registered allocator
CTest passed in 3.37 seconds. They were run as bounded groups. The new harness
was rebuilt after its independent expectation and explicit fault-type assertion
changed; affected allocator acceptance/UBSAN was rerun, without repeating green
unrelated groups. No full suite or broader historical campaign ran.

UBSAN compiles the four allocator sources, aot_runtime, allocator_equivalence_checks,
allocator_checks, aot_probe and probe_main with ARM64/C++23, -O1,
`-fsanitize=undefined -fno-sanitize-recover=all` and `-ffp-contract=off`, linked
against the final consistent libraries. The allocator binary repeats the complete
337-case matrix, replenishment passes all 16 reference cases, and the modified
probe's focused global-context call returns `0x10bf7644`. This is focused UBSAN,
neither whole-engine instrumentation nor ASan.

## Protected installation and smoke

The installer ran exactly once after technical acceptance, at 22:18:59 on
October 9, 2026 (America/Sao_Paulo). Internal executable identities are:

| Artifact | Architecture | SHA-256 |
| --- | --- | --- |
| Retained linker output before manual signing | ARM64 | `117e2fb1df071483ba6defaef5371f8c596cc76d4b4334d71266b436a12741e0` |
| Signed source bundle | ARM64 | `14eb1c373854e95c6e642ebc1ddd99ae56f9c353ecb49de93e0822cc93e44ae8` |
| Private signed candidate | ARM64 | `14eb1c373854e95c6e642ebc1ddd99ae56f9c353ecb49de93e0822cc93e44ae8` |
| Installed executable | ARM64 | `14eb1c373854e95c6e642ebc1ddd99ae56f9c353ecb49de93e0822cc93e44ae8` |
| New protected reference of prior installed app | ARM64 | `0725840a0907bc6bd866d47632fbe75606defaa98d320ecb53b50c7348fed749` |

Strict deep signature verification passes for source, candidate, installed and
reference bundles. All four original program payloads match the private source;
the production plists match and contain no validation settings. Resources contain
only OriginalProgram, with no harness, fixtures or diagnostic configuration.
The running smoke process is verified under the installed application path,
with the same internal executable hash.

Immediately after installation, before smoke, full content and metadata checks
preserved 809 Game, five Runtime and 55 Runs files, the LFS save snapshot, all
23 handoffs (18 private), the original source payloads, the new prior-app reference
and 120 historical private bundles. Protection includes resource forks,
FinderInfo, extended attributes, permissions, flags and modification times.
The prior app was copied to a new reference before signing/installing the new
artifact; earlier references were not overwritten. No user session was active.

The installed application rendered Prepare Game. Command-O opened its chooser;
Command-[ cancelled back to the menu without loading a career. Command-Q rendered
the original exit confirmation and Return confirmed normal exit. The startup log
reports a normal original-application exit and no fatal diagnostic, and the final
process check finds no app session. UI inspection also briefly opened/dismissed
File while the initial resources loaded; one two-second read-only process sample
showed the existing file services. No heap snapshot or benchmark was taken.

The private exit summary records semantic calls to all 27 entries,
with zero original selections and zero allocator dispatch escapes:

| Entry observed in installed smoke | Semantic calls | Original calls |
| --- | ---: | ---: |
| `0x107740f0` | 93 | 0 |
| `0x10774190` | 14671 | 0 |
| `0x10774270` | 14680 | 0 |
| `0x10774360` | 14578 | 0 |
| `0x107743e0` | 22289 | 0 |
| `0x10774430` | 11098 | 0 |
| `0x107744d0` | 14521 | 0 |
| `0x10774570` | 14547 | 0 |
| `0x10774630` | 93 | 0 |
| `0x10774680` | 90 | 0 |
| `0x107746d0` | 93 | 0 |
| `0x10774760` | 14351 | 0 |
| `0x10774840` | 247 | 0 |
| `0x10774920` | 14574 | 0 |
| `0x107749b0` | 229 | 0 |
| `0x10774ae0` | 1 | 0 |
| `0x10774b10` | 89363 | 0 |
| `0x10774b60` | 30294 | 0 |
| `0x10774d20` | 30279 | 0 |
| `0x10774e80` | 44643 | 0 |
| `0x10774ee0` | 44628 | 0 |
| `0x10774f40` | 147 | 0 |
| `0x107750e0` | 7 | 0 |
| `0x10775130` | 44607 | 0 |
| `0x10775170` | 44602 | 0 |
| `0x107751b0` | 147 | 0 |
| `0x10775200` | 7 | 0 |

These counters prove execution during the observed menu/chooser/exit sequence.
They do not establish every branch, independent allocator expectations or
original-versus-semantic equivalence; those proofs belong to the focused harness.
No extra UI campaign was run for individual entries.

Final protection again confirms Game content/metadata, the LFS snapshot, all
handoffs, payloads, the new baseline reference and all 120 historical bundles.
Runtime/Runs and presentation preferences may change from the intentional smoke;
no post-smoke byte-identity claim is made for them. The installed executable hash
is unchanged after exit. No career, day, match, save or load ran. Persistence
remains **not retested**, neither passed nor an additional future acceptance gate.
