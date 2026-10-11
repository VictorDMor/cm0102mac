# Engine global initialization reconstruction

## Scope and implementation

The root `0x10000000` and its nineteen immediate global initializer calls are
reconstructed as named C++23 initialization operations. The root retains the
original nineteen-call order. Shared operations prepare array parameters,
initialize date groups/pairs, construct registered objects, publish the season
date, and register destructor records. Guest addresses, fields and descriptor
slots remain big-endian word32 memory. No guest object is replaced by host storage.

All twenty bodies have one entry and no exterior-to-interior edges in the full
private instruction index. Complete generation appends exactly twenty records
with module `engine_initialization_adapter` after the accepted 539 records,
giving **559** semantic replacements. The preceding canonical digest is
`dd0b654f24290a715e0fbe252d807513955ad3827cf5eed0b7baad0d21291fbd`.
The full 559-record digest is `2d2d5079bbf41b2456c5d6c68aa30c2a4f1207d739c6c659ec1b3f2b88c9fc25`.
Generation remains 1,953,334 instructions / 1,922 pages with zero missing/pending
targets; all five previously accepted source fixups remain unchanged.

## Complete boundaries and pins

Intervals are end-exclusive; fingerprints cover address, assembly, ops, flows
and fallthrough using the established canonical serialization.

| Entry | End | Instructions | Operation | SHA-256 |
|---|---|---:|---|---|
| `0x10000000` | `0x10000068` | 26 | Central ordered initialization | `98aeeb7752286379c7fde9a8a0eb88432bbed36d37deaa1badc70e1c13bb54ed` |
| `0x10064770` | `0x100647d0` | 24 | Two arrays and finalizer registration | `bf7726f65414f49d477b17ead5523d53537ca745e9e5cc8dcf21097fedbd8b23` |
| `0x100939b0` | `0x10093a4c` | 39 | Sixteen default dates | `c68c93b460d5b924f5834a99a424021601e4b3d3356dd0463ff4bcb7b40b37da` |
| `0x100de3b0` | `0x100de408` | 22 | Initial season date publication | `348683c0932527bd1be0a350bb4f3c3b5441ff627546a9586076e5a7ac9cedaf` |
| `0x1013f000` | `0x1013f02c` | 11 | Two default dates | `a80a240f58a0a0cc827f2f6e3fb7cebb36705970415c2fda4f8d7983b8d0f349` |
| `0x10143240` | `0x10143274` | 13 | Plain global array | `cd33f817f8e3bd8391e36c1894f5cc2009a42916f165ee5fdde69f47c8cb6024` |
| `0x101935b0` | `0x101935e4` | 13 | Plain global array | `aa6ed8706f3d98406737390cca443382f2e77b118a26ac8ffab1b7c0c47ed701` |
| `0x102aac90` | `0x102aacd4` | 17 | Registered global array | `c5a272a5ca0169d108e2f7373e40fa5530976f1a434b6d220bb3ba6f620030ec` |
| `0x1030d470` | `0x1030d49c` | 11 | Two default dates | `2e4f1d07253ce748d226889277e81c09745d388acd1e9cdd58a1f6b6fceecce5` |
| `0x10315050` | `0x10315080` | 12 | Object construction and registration | `01c7d4d89bbe1b7fc98d72f33a1afaed065997b9271c392e2ec44537123629d4` |
| `0x103563e0` | `0x1035640c` | 11 | Two default dates | `c796c3adee01aa4eb08b0fb504e11c11cca4c8d37b428ab117e433d3325932e5` |
| `0x103810d0` | `0x10381104` | 13 | Fixed global date construction | `1287a8cbf6c3a2d6bc9e8fe6ed5a24f29b31e4b17777b23fddf6f05c58c2dee0` |
| `0x1048eb80` | `0x1048ebb0` | 12 | Object construction and registration | `59fd7683e1764a4ff7fe2c4a7e3f6f9c7d90bbc65dd0d43d6016aa29ed549853` |
| `0x106b6e60` | `0x106b6e8c` | 11 | Existing global object registration | `46b37f02054f4962df57ca554c6528439692e0c2cb273fd0c9dd13666a347174` |
| `0x106bbe10` | `0x106bbe3c` | 11 | Existing global object registration | `d0ca5e75d96becd47a0aed5050f9fbb80b06ed1f258666de2f693d3dda40d958` |
| `0x107259a0` | `0x107259fc` | 23 | Two buffer constructions and registration | `dfeef0130947df64e8719afb91c90bc8cb97e9a9dd9b49fad93218b9835f343d` |
| `0x10739f30` | `0x10739f68` | 14 | Descriptor object and registration | `475327c5ba084573c9cf240ce2f430a726430f8677b89a3ca47ff99342a0ca66` |
| `0x10744640` | `0x10744670` | 12 | Aggregate object and registration | `d97b9b77fb2d3095f5da84b38e3311b5d2255e65b0880c6e6ef5c35429f4a831` |
| `0x1074d340` | `0x1074d37c` | 15 | Object clearing and registration | `92457175436da5e390d2f4d209578c9da6753bed5df53d30f2f7af5d0530314a` |
| `0x1076ea20` | `0x1076ea64` | 17 | Registered scalar array | `b541e25e416dbbd000f97a68368173de75c8860a85f2177da65006a3e5d0f698` |

Total: **20 entries / 327 indexed instructions**. Hooks occupy the root labels
before any original operation. The isolated reference contains precisely these
327 operations, with semantic hooks and native context ownership disabled.
Pins reject changed, missing, duplicated, reordered and wholly absent bodies;
edge and label tests reject interior entrances and misplaced/duplicated hooks.
Private instruction listings, payloads, generated units and receipts stay local.

## ABI and effects

Each checkpoint retains the original PC and finite instruction-budget prefix.
TOC slots, saved-link words, SP, returned r3 and nonvolatile r31 remain live.
The 64-byte frames store their backchain before publishing SP. The season-date
initializer uses an 80-byte frame, constructs a local date, sign-extends its
16-bit day, then publishes two halfwords and one byte in original order.
The buffer initializer preserves its destructor in r31, requests 50,000 and
3,072 bytes, and registers each returned object separately.

The central root increments SP before reading its saved link. Other children
read the link before incrementing SP; the two r31 users restore it after LR.
Registration uses the child constructor's returned r3. Plain arrays pass a null
destructor; registered arrays load live constructor/destructor/finalizer slots.
Live observers and aliasing do not permit caching the guest state across those
operations. All normal dependency calls use the existing dispatcher; no original
body of these twenty entries is replayed by a semantic initializer.

Already semantic dependencies remain Date `0x107156f0`/`0x10715880`, C++ array
construction `0x10773320` and lifecycle registration `0x10772df0`. The object
constructors `0x10738c00`, `0x1073ae10`, `0x1074dec0` and `0x1073c1b0` were
generated dependencies in this delivery. Their actual successful paths,
allocation effects and registration participate in production comparisons;
this block does not claim
that those four constructors have been semantically reconstructed. Date's
ascending-year leap behavior and the allocator progress guard remain unchanged.
The later
[utility-object delivery](native-utility-object-reconstruction.md) reconstructs
the first three constructors and their related lifetimes; the aggregate
constructor remains generated.

## Owned diagnostics and host continuations

`State` owns an opaque `RuntimeDiagnostics` extension through a unique pointer.
Its lazy metadata contains only selection, counters and private boundary
observers. Constant queries allocate nothing. Each State has independent data;
resetting this module preserves prior module diagnostics and guest state.
Existing snapshots serialize named architectural fields, not this pointer.
The out-of-line State constructor/destructor own the complete private type.
Later extensions can grow without changing the layout of generated units.
This first extension still requires one complete, consistent State-layout build.

Global and per-entry original selections, separate profile-original accounting,
returns, faults, finite stops, context transfers and dispatch escapes are private.
No finite budget selects an original fallback. A native frame scope records the
initializer and its allocated frame; guest context restoration abandons the
interrupted C++ continuation instead of running an epilogue afterward.

## Validation status

Implementation, full regeneration and technical acceptance are complete.
Protected promotion and installed menu/chooser/cancel/normal-exit smoke pass.
The signed ARM64 executable SHA-256 is
`0af6b5bf6bea154f6b25e00f49593fc0c7fa2948c810b1daa70f27e4f5e5809c`.
Candidate, installer source, installed bundle and owned running executable have
the same internal executable identity; signature and all four bundled original
program payloads are verified. Installed diagnostics record all twenty entries
once (twenty semantic calls and returns), with zero original/profile selection,
own faults, budget stops, transfers or dispatch escapes.

Game/save/source-payload/handoff/reference content and metadata protection passes,
including 190 older bundles and 23 retained handoffs. The previous installed app
and installer source are preserved privately. Runtime/Runs and the presentation
preference may change legitimately through the owned UI smoke; they are not
claimed byte-identical afterward.

Ten focused CTests pass: four new initializer checks, four directly related
Date/object-storage/lifecycle/bridge checks, and two presentation checks. The
component matrix records 2,628 comparisons, 2,448 faults, 820 exact budget stops
and twenty own negatives. Actual production dependencies pass 124 comparisons,
including direct/generated/CFM paths and four mixed root compositions; profile
selection passes twenty comparisons. The nonlocal matrix passes 142 comparisons,
including eighty restore/resumed-handler faults and twenty Turbo compositions.
All 31 run Python audit suites finish successfully (206 unittest tests, no final
skips, plus three positional mutation tools); the affected/new subset is 25
suites / 167 unittest tests. Twenty-nine UBSAN units pass four runs with empty
sanitizer stderr and matching Release production/profile/transfer output.

Isolated checks for the last initializer first execute the original
`0x10739f30` file-controller initializer on both independent private platforms.
That earlier startup entry publishes the CFM vectors used by the final array's
constructors. Prepared architectural state and accounting must match before
measuring the target. Full root composition starts from untouched segments and
executes all twenty initializers once in original order.

The consistent Release library contains 2,025 fresh State-dependent objects,
including all 1,922 generated pages and 103 native units. New diagnostics are
owned through an opaque runtime object, so later additions can avoid changing
the generated State layout again. The original 539 manifest records and five
source fixups remain exact.

The component matrix compares all architectural bytes, temporary residues,
ordered guest accesses, mapped bytes/ranges, allocator maps, memory errors,
native calls and controlled dependency boundaries. It covers all checkpoints,
minimal mappings, individual accessed-byte holes, live SP/TOC changes, stack/TOC
aliases, signed date publication, constructor returns and one ABI negative for
every entry. Controlled dependencies are isolated fixtures, not installed code.

The production harness uses real original program segments and actual guest
constructors/dependencies. Direct/generated/CFM paths, global/per-entry selection,
profile accounting, complete root composition and mixed Date/lifecycle/object
storage selections are checked separately. Nonlocal checks compare known scalar
restoration, malformed restore buffers and resumed-handler faults for each entry.
The existing scalar-context module has different original/semantic step charges;
those are measured locally, while the surrounding initialization accounting must
remain exact. No register, memory, ABI or effect difference is normalized.

The same installed app also includes the requested presentation change: the
original desktop backdrop remains available to the engine, while its host window
is hidden. Visible game-window screenshots measure 800×600 at 100%, 1000×750 at
125% and 1200×900 at 150%; the original 100% preference was restored before normal
exit. AppKit checks additionally verify exact frame/content dimensions, repeated
desktop ShowWindow suppression, margin hit testing, bitmap invariance and the
existing mouse/menu/command paths. This is a presentation change, not an addition
to the semantic manifest.

Save/Load is **not retested**. No career, day/week/season, match or benchmark is
part of this bounded block. The complete engine/runtime goal remains active.
