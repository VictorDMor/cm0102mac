# Aggregate-object lifetime reconstruction

## Production scope

Six complete Mac bodies now implement embedded-table construction/destruction,
aggregate construction/destruction, record-group cleanup and stream detachment
as named C++23 operations. The semantic bodies call the actual existing buffer,
allocation, array-destructor, record-movement, stream and CFM dependencies. They
never invoke their own replaced original bodies.

Full generation appends six `aggregate_object_adapter` records after the accepted
573, giving **579** semantic replacements. The original 573 records retain digest
`4253c73fd9b0353e0d74f023c540b0386c11d144bfa001649a30956def5e0598`.
The full replacement digest is
`90c298066b2c7b10eedce39a0fd5e14464c7bcc2efb78542bfbc81b237d155dd`.
All 1,953,334 indexed instructions, 1,922 pages, roots, generated-file inventory
and five source fixups stay unchanged. Missing and pending targets are zero.
The full source index rejects exterior edges into replacement interiors; every
destination hook precedes original operations, including the hot-page counter.

## Complete boundaries

Intervals are end-exclusive. Fingerprints cover address, assembly, operations,
flows and fallthrough using the established serialization.

| Entry | End | Instructions | Operation | SHA-256 |
|---|---|---:|---|---|
| `0x10718b40` | `0x10718ba0` | 24 | Embedded-table construction | `e7913fb414349c7180f2498e347e3e3cf15d729a8b69f0ff2eb00c6ac42f9aaf` |
| `0x10718ba0` | `0x10718d74` | 117 | Embedded-table destruction | `ef07c7b8530fb902c58705e8aec7bbbfefd477011b07288f849a6ee1f4396245` |
| `0x1073c1b0` | `0x1073c45c` | 171 | Aggregate construction | `5f2ea432e446a5c155f907a1a8aa6eb9dfcdba8dcead08c790d26bca3582d62f` |
| `0x1073c460` | `0x1073c4e0` | 32 | Aggregate destruction | `4a414b8713224b377c066651f912a8f1c72db94ea9ca1f22c2ce066507d24633` |
| `0x1073c4e0` | `0x1073c5a0` | 48 | Record-group cleanup | `669a4d8cd26cebe07f657fff6e81a1e71440278abe676b3265309013193b4c09` |
| `0x1073f4e0` | `0x1073f52c` | 19 | Stream detachment | `44b50bd8c93cf53e016f1a39df6bde3df6834654981cfef699b3ac5771b43f1f` |

Total: **6 entries / 411 indexed instructions** across three pages. Missing,
changed, duplicated, reordered or wholly absent bodies and missing, duplicated
or misplaced hooks have negative controls. Original instructions and references
remain private.

## Preserved behavior and ABI

The leaf table constructor touches explicit fields through offset `0x12a946`;
the declared table extent is `0x12a948`. It preserves padding, initializes the
mode from the low argument byte, enables the table, clears active counts and
writes the original negative halfword sentinels. It returns the guest owner.

Table destruction reads two TOC globals before testing a null owner. Descriptor
cleanup uses the live pointer-array/count fields and actual global allocator;
the descriptor itself remains allocated as in the original. Active counts are
signed halfwords and are reread after callbacks. The two element families retain
their original `0xbf0`/`0x17c` strides. A cleared owned-pointer field is deliberately
tested again: a memory callback can republish it. Shared cleanup retains its
signed reference-count condition. The actual array helper always traverses 1,200
and 250 elements, including when active counts are zero. The deleting flag uses
its signed low halfword.

The aggregate has extent `0x2581b8`. Construction initializes its 50,000-byte
buffer and two embedded tables at `0x2c68` and `0x12d6b0`, then explicit fields,
16 row counts and 160 tail bytes. The live saved owner at guest SP+88 is reloaded
at the original sites; callbacks/aliases can redirect subsequent stores. The
first row starts at one and the selected-table field points to the first embedded
table. Padding and the five expanded byte stores per inner iteration are retained.

Record cleanup retains 16 rows of stride `0x2c0`, live signed row counts and
44-byte record movement. It publishes the active row, calls a nonnull live CFM
deletion vector and reloads the TOC before actual unlinking. The last active row
is not normalized. Stream detachment flushes first, rereads the attached stream,
destroys it with flag one and only then clears the field. Aggregate destruction
orders record cleanup, detachment, second table, first table, buffer and optional
scalar deletion exactly as the original.

Guest register saves, frames, multiple-register cursor, ordered byte/halfword/word
accesses, SP/TOC/LR/PC/CR/XER, full register storage, caller arguments and finite
budgets retain original observable prefixes. Compatibility checkpoints implement
that ABI around named lifetime operations; no host object/container replaces the
guest layout and no host blanket initialization replaces the field operations.

Selection and per-entry counters live in the existing opaque diagnostics owner.
The State header/layout is unchanged; const queries allocate nothing and resets
do not erase other subsystem diagnostics. Default execution uses semantic bodies.
`CM_AOT_AGGREGATE_OBJECT_ORIGINAL`, per-entry selection and original profiling
remain explicit development paths with balanced boundaries.

## Focused validation

The component harness passes **4,006** comparisons, with **3,929** interrupted or
faulting outcomes including **3,021** exact budget stops, and six independent ABI
negatives. Its reference contains exactly 411 original instructions with hooks
disabled and isolated same-page functions. Only component dependencies are
controlled; every dependency call records full register storage and ordered
effects. Original and semantic nested aggregate entries execute independently.

Cases cover null owners, signed flags/counts, modes, both table families, all 16
rows, live count/owner mutations, repeated pointer publication, CFM/null callbacks,
TOC effects, unaligned and stack-aliasing owners, untouched padding, every distinct
missing accessed byte and every finite budget prefix. Divergent or unexpected
faults fail rather than being normalized.

Seven focused CTests pass: component, actual production, profile and nonlocal
aggregate checks plus directly related initializer production/profile/nonlocal
checks. Generator audits pass **229** executed tests with no remaining skipped
scope: discovery's 13 skipped classes/tests were subsequently covered by 28
explicit-fixture tests. Thirteen generated-storage regression tests also pass.
Older audit prefixes remain pinned; only the global replacement total changes.

Actual production passes **54** direct/generated/CFM/global/per-entry/negative
comparisons and six original-profile comparisons. Prerequisites use actual
allocators and original constructors on independent platforms with private user
directories. Fixtures cover both table modes, allocated descriptor contents,
positive/nonpositive deleting flags, row cleanup and an attached empty stream.
The stream fixture allocates zero-initialized guest storage; actual generated
flush/close/deletion run without opening a user file. An application-exit prefix
check verifies termination separately from a normal return or fault.

Another **44** known nonlocal comparisons cover the leaf and five framed entries,
both scalar-context implementations, mixed capture/restore selection and Turbo,
including 24 restore/handler faults. Only the surviving parent import is
controlled. Actual object dependencies and scalar capture/restore retain their
production implementations. Abandoned continuations never resume; boundary
classification, ownership and accounting agree and unknown transfers stay zero.

Seven affected units pass UBSAN in component/production/profile/transfer modes
with empty stderr and byte-identical Release output. Unchanged dependencies use
the fresh consistent production archive and are not newly instrumented. All
2,031 State-dependent production objects were compiled in the same Release build.

## Installed delivery and retention

The signed ARM64 candidate, installed bundle and actual loaded executable share
SHA-256 `86f34c78266f270d83b7f477322c635b38b417138ff68bf2f181c9a561c0d8df`.
Strict signature and all four bundled payload checks pass. The owned installed
main-menu/native-menu-cancel/original-quit-confirmation/normal-exit smoke records
**10** semantic calls and returns across all six entries, in entry order
2/2/1/1/2/2. Original/profile calls, own faults, budget stops, transfers,
terminations and dispatch escapes are zero. The process exits normally; the
observed content remains 800×600 in the existing scaled-content window.

All 809 original game files, four current runtime files, private inputs, 23
handoffs, references and 204 prior bundles retain their contents and Mac metadata.
One save was already newer than the phase's initial snapshot: its modification
at 16:19:34 -0300 precedes the 16:24 integrated tests. The current version is
preserved and compared before installation, after the smoke and after cleanup;
the earlier snapshot is never restored. No save is loaded, written or deleted by
these checks. Intentional run logs/preferences can change.

The prior installer source is preserved by rename. After acceptance, **4,114**
reproducible old objects/dependencies, test executables and static archives are
removed, recovering **1,471,557,632 bytes** of observed available space. The current
consistent build, application bundles, sources, inputs and receipts remain.
Approximately 54.5 GB is available at the preservation check; concurrent external
filesystem activity affects this figure. Cleanup verifies build working
directories and targets only this project's disposable outputs.

Save/Load is **not retested** and performance is **not benchmarked**. No career,
day, season, match or historical heap campaign is part of this bounded cycle.
The complete engine/runtime reconstruction goal remains active.
