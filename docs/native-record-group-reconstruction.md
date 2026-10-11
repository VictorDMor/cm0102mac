# Record-group reconstruction

## Production scope

Eleven complete Mac bodies implement current-record labels, owned/borrowed fields,
field updates across groups, insertion/removal, callback publication and identifier
swapping as named C++23 operations. Their production paths call the existing actual
allocation, string, record movement, table, stream, reporting and CFM dependencies.
They never invoke their own replaced original bodies.

Full generation appends eleven `record_group_adapter` records after the accepted
579, giving **590** semantic replacements. The 579-entry prefix retains digest
`90c298066b2c7b10eedce39a0fd5e14464c7bcc2efb78542bfbc81b237d155dd`.
The full replacement digest is
`df2582404500734e3ee82bca17aa9deed4bcbcddcd7079cd92cf080a62c63740`.
All 1,953,334 indexed instructions, 1,922 pages, the application root, generated-file
inventory and five source fixups remain unchanged; missing and pending targets
are zero. Whole-index flow/fallthrough audits reject exterior interior entrances.
Every destination hook precedes the original operations and instruction counter.

## Complete boundaries

Intervals are end-exclusive. Fingerprints include address, assembly, operations,
flows and fallthrough in the established serialization.

| Entry | End | Instructions | Operation | SHA-256 |
|---|---|---:|---|---|
| `0x1073edb0` | `0x1073eddc` | 11 | Current label | `6879f911dd4df4306f389d6e570b272b03168770b37a662344506e174da0b2dd` |
| `0x1073ede0` | `0x1073ee30` | 20 | Set current label | `927c9a322346bc89d0dc59d76d4b783ecddbaedf955997185c8473ce7e2cdbb5` |
| `0x1073ee30` | `0x1073ef68` | 77 | Release current chain | `a79ec9349f9093db1036be68f8a9742abc51743d67dacb478dbd9f51242decdb` |
| `0x1073ef70` | `0x1073f09c` | 75 | Get current field | `43553ee94bc15ef648e52485a715b80b3a2948e5d59ce188cbd91eecc8eec686` |
| `0x1073f0a0` | `0x1073f1dc` | 79 | Set fields across groups | `763b9bea9d9c75b0f2eadfaa6ca8305e38cf9dfc5ae9306ed1bab820b8dc88ad` |
| `0x1073f1e0` | `0x1073f44c` | 155 | Set/clone owned field | `6a241d9180201519f94d7c674910dac6f93fe886ffb5949b607f57cf1fea101e` |
| `0x1073f530` | `0x1073f6fc` | 113 | Insert group record | `5eff2c09a8e33e6b24f343dc3703f385d3a0c3aeb20a894aa36cd683a639d952` |
| `0x1073f700` | `0x1073f79c` | 39 | Remove groups of type | `a38624b085d086245cf70bff39459853546e3061f51185e8bec5f69fa2617908` |
| `0x1073f7a0` | `0x1073f860` | 47 | Swap identifiers | `e9ccc54169ad8263349bd2c5e5ed4bf448c913f0de9babd041fb472b38cf3ed2` |
| `0x1073f860` | `0x1073f8fc` | 39 | Insert with callbacks/extra data | `0591136f306daf8428da28ca06f75f722504ec4cfd37385f0fe0dcac28d8f6d2` |
| `0x1073f900` | `0x1073fc18` | 198 | Remove/search/compact/reinitialize record | `2a3e4ed62d20bc34a7899ace43861c9355d4df059bd4c9b37057198807bdcba4` |

Total: **11 entries / 853 indexed instructions**. The getter crosses two pages:
36 instructions on `0x1073e` (digest
`a61981385025ca1b0748b57c9b5781d47fb29ce0724ef48e8229fb3deb1b1fb3`)
and 39 on `0x1073f` (digest
`36b35828ba81e9d09f9a803df3d9518b0e81f2e1669d369a366fa9e5b83e1097`).
Both fragments are independently audited and privately rendered. Missing,
changed, duplicate, reordered or absent bodies/fragments and missing, duplicate
or misplaced hooks have negative controls. Original references remain private.

## Preserved behavior and ABI

The guest layout retains 16 rows of stride `0x2c0`, 44-byte cached records and
60 field slots of stride eight. The selected row is a signed halfword at owner
`+0x2c58`; current node is row `+16`. Labels use node `+0x1fc`, with the original
TOC-derived fallback for an absent node. The setter clears one byte for null
input and otherwise calls actual string copying.

Chain release publishes the next node before deleting any owned fields, then
deletes the node itself. Ownership is a signed low byte. It rereads the selected
row and live count after callbacks, decrements the original halfword and clears
the five explicit row fields. Borrowed pointers remain borrowed.

Field indices are signed full words from zero through 59. Invalid indices and
absent current nodes retain the original reporting calls, temporary stack slots,
diagnostic line constants and error latch. Replacing an owned field deletes it,
rereads the current node, then clears ownership. Cloning uses actual length,
array allocation and copy calls, publishes the new pointer before checking it,
and marks ownership only after successful copying. Allocation failure leaves
the original published null/error state. Callbacks can redirect/repopulate live
fields; these effects are not cached or normalized.

Bulk updates walk all 16 live rows and signed record counts, optionally filter
the identifier, copy 44-byte cached records through the actual movement helper,
update the selected field and restore the first cached record and original row.
Insertion uses the global owner from the TOC, preserves the attached-stream/
pending gate, signed type and capacity limits, shifts records backward and
initializes explicit fields. A live CFM creation vector receives the original
arguments; the TOC is reloaded afterward. Type-zero records use the first
embedded table. Callback publication retains the original nonzero-extra-data
path and its temporary creation-vector suppression.

Identifier swapping scans the live selected row and chooses the last duplicate
match. It writes the first identifier, rereads the selected row and then writes
the second. Removal preserves the distinction between the argument owner and
global owner. The low-halfword minus-one argument selects a backward per-row
identifier search. It preserves payload deletion, current-chain cleanup,
shared-table cleanup, live destroy callback, TOC reload, cached-record compaction,
last-record clearing and the halfword store-with-update. Empty row zero is
reinitialized through its actual dependency; other emptied rows write the
original marker/buffer or reporting path, followed by the actual stream method.

Guest saves, two leaf/nine framed entries, SP/TOC/LR/PC/CR/XER, multiple-register
cursor, full register storage, scratch bytes, ordered accesses, caller arguments
and finite budget prefixes retain their original observable effects. Named record
operations use guest checkpoints for this ABI; no opcode interpreter or modern
container replaces their semantic bodies or guest layout.

Selection/counters live in the existing opaque diagnostics owner. The State
header/layout stays unchanged; const queries allocate nothing and subsystem resets
preserve other diagnostics. Default execution uses semantic bodies.
`CM_AOT_RECORD_GROUP_ORIGINAL`, per-entry selection and original profiling remain
explicit development paths with balanced boundaries and separate outcomes.

## Component validation

The component harness passes **6,002** comparisons, with **5,856** interrupted or
faulting outcomes including **3,664** exact budget stops, and eleven independent
ABI negatives. The reference contains exactly 853 original instructions with
hooks disabled, separate same-page entry functions and both getter fragments.
Only component dependencies are controlled; every dependency captures full
register storage and ordered effects. Nested original and semantic entries run
independently.

Cases cover null labels/fields, signed indices/ownership/types/counts, all rows,
cloning and allocation failure, live row/count/pointer changes, CFM/null callbacks,
pending/capacity gates, duplicate identifiers, owner distinction, forward
compaction, row-zero reinitialization, full buffers and original error reporting.
Every distinct accessed-byte hole and every finite successful-fixture budget
prefix is compared. Initial normal fixtures must return normally; unexpected or
divergent faults fail.

## Integrated validation

Seven focused CTests pass: component, actual production, profile and nonlocal
record checks plus the directly related aggregate production/profile/nonlocal
checks. Generator audits pass **236** executed tests with no remaining skipped
scope: discovery's 13 skipped classes/tests are covered by 28 explicit-fixture
tests. Thirteen generated-storage regression tests also pass. The new seven
audit tests are rerun after adding explicit cross-page fragment negatives.
Older manifest prefixes remain pinned; earlier audit changes are restricted to
the global total and the now-bounded aggregate suffix.

Actual production passes **99** direct/generated/CFM/global/per-entry/negative
comparisons and eleven original-profile comparisons. Each default root has two
direct, two generated and two CFM comparisons and one own ABI negative. Full
guest images, ordered memory effects, actual import-boundary register snapshots,
accounting, balanced boundaries and dedicated counters agree. Original/profile
selection and dispatch escapes stay zero on default semantic paths. Current-label
and identifier-swap results have independent assertions. A separate application-
exit prefix verifies termination classification and selection unwinding.

Prerequisites use actual guest allocators and the original aggregate constructor
on independent platforms with private user directories. Both cached records have
distinct actual nodes and owned array-allocated fields. Three real CFM descriptors
call the original two-instruction constant-return body at `0x10002390`; no host
replacement callback supplies production evidence. A closed zero-initialized
guest-stream fixture exercises the actual method's minus-one-handle return path
without opening a user file. It is not claimed to be a constructed open stream.
The constructor's empty default first row is explicitly inactive for the bulk-
field fixture; an active row with no current node takes the original reporting/
application-exit path and is not a normal positive fixture.

Another **81** known nonlocal comparisons cover two leaves, nine record frames,
both scalar-context implementations, mixed capture/restore selection and Turbo,
including **44** restore/handler faults. Only the surviving parent import is
controlled. Actual allocation, record, CFM and scalar capture/restore dependencies
retain their production implementations. All five root arguments survive parent
capture. Abandoned continuations never resume; ownership, boundary classification,
ordered effects and exact accounting outside the measured scalar dependency agree.
Unknown context transfers stay zero.

Seven affected units pass UBSAN in component/production/profile/transfer modes
with empty stderr and byte-identical Release output. Unchanged dependencies use
the fresh consistent production archive and are not newly instrumented. All
2,034 State-dependent build objects are compiled in the fresh Release build;
the fixture correction rebuilds its two dependent harness units. The application
and semantic production sources remain the same validated build.

## Installed delivery and retention

The signed ARM64 candidate, installed bundle and actual loaded executable share
SHA-256 `be589e7f51dda32ccdf2746d69a3ba9d2d50181486a16715845730d35ac514c5`.
Strict signature, all four bundled payload files and exact installed-candidate
inventories pass. The owned installed menu/native-menu-cancel/original-quit-
confirmation/normal-exit smoke records **271,437** semantic calls and returns
across five entries, in entry order **4/2/2/271420/0/9/0/0/0/0/0**.
Original/profile selection, own faults, budget stops, transfers, terminations and
dispatch escapes are zero. The other six entries have dedicated actual production
coverage; this menu smoke is not claimed to execute them. The process exits
normally and content remains 800×600 in the existing scaled-content window.

All 809 original game files, four current runtime files, six original inputs,
23 handoffs, references and 208 preexisting bundles are protected. The current
save is preserved; no save is loaded, written or deleted by these checks.
Intentional run logs/preferences may change. The prior installer source is
preserved by rename. The preceding generated-source tree retains all 1,924 file
hashes, identities and modification times across cleanup.

After acceptance, **4,120** reproducible old objects/dependencies, test executables
and static archives are removed, recovering **1,410,248,704 bytes** of observed
available space. The current consistent build, all application bundles, sources,
inputs and receipts remain. Approximately 51.6 GB is available immediately after
cleanup; concurrent external filesystem activity can affect this figure. Cleanup
checks project build working directories and validated targets before deletion.
The new generation seeds 1,923 independent APFS copy-on-write C++ files from the
accepted source tree and still rerenders/audits every page.

Save/Load is **not retested** and performance is **not benchmarked**. No career,
day, season, match or historical heap campaign is part of this bounded cycle.
