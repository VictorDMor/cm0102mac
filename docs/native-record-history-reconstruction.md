# Record-history insertion reconstruction

## Production scope and complete boundaries

Two complete Mac bodies implement current-row history insertion and application
of that operation across matching cached records as named C++23 operations.
Normal semantic paths do not invoke their own replaced original bodies. Reporting,
byte exchange, allocation, field disposal, CFM callbacks and string/memory helpers
remain actual production dependencies.

Full generation appends two `record_history_adapter` records after the accepted
596 semantic entries, giving **598** replacements. The exact 596-entry prefix
retains digest
`ab68bf9563f7939b11a3c3c358bee40862331a129d17efb9bb97778681cc6af6`.
The complete 598-entry manifest digest is
`946618ec9658d018c53f6bce0dad8f775615d81ad667b37daa40be0ae96ed3c6`.
The application root, all 1,953,334 indexed instructions, 1,922 pages, generated
file inventory and five source fixups remain unchanged; missing and pending
targets are zero. All 1,923 generated C++ files are seeded as independent APFS
copy-on-write files, with zero regular copies, before full regeneration/audits.

Intervals are end-exclusive. Fingerprints include address, assembly, operations,
flows and fallthrough in the established serialization.

| Entry | End | Instructions | Operation | SHA-256 |
|---|---|---:|---|---|
| `0x1073e350` | `0x1073e4a8` | 86 | Apply history insertion across matching cached records | `fa2eb26667b7bbbd53beeff6fef7059e698fae8a491284f7bc2e1296506a1872` |
| `0x1073e4b0` | `0x1073e984` | 307 | Select, truncate, bound and append current-row history | `24bc7910af5ad4e6e04444a3b94c25a9e88b308dd7c65829ccfee482b8510353` |

Total: **2 entries / 393 indexed instructions**, on page `0x1073e`.
The full SQL flow/fallthrough audit finds zero exterior entrances into either
body interior. Pins reject changed, absent, missing, duplicate or reordered
bodies. Both hooks precede original operations and instruction counting at the
generated destination labels; missing, duplicate or misplaced hooks fail.
Exact original references remain private.

## Preserved history and cache behavior

Bulk application saves the selected row, visits sixteen rows with live signed
cache counts, filters the records by an optional full-word identifier and restores
the selected row after normal completion. The original `0x10745e00` dependency
**exchanges** the 44-byte record headers. It is not a copy operation: exchanged
headers retain each record's changed history, and the final exchange restores
the original record order. No host collection replaces those headers.

Current-row insertion preserves fallback to the arguments published at owner
`+0x2c00/+0x2c04/+0x2c08/+0x2c0c/+0x2c10` when both the cached identifier and
history head are zero. Identifier and auxiliary words must be nonzero; invalid
arguments retain the original reporting calls, guest temporary slots, line
`0x4eb`, live TOC-derived latch and zero return. No new validation is introduced.

For a nonzero low mark byte and an existing current node, the original search
finds a marked node with the requested identifier, selects successive marked
nodes and keeps the final marked selection only when its identifier matches.
Otherwise it restores the previous current node. Both outcomes return zero
without appending or allocating. Node links and selected-row fields remain live.

An ordinary insertion truncates nodes after the current node, disposes only
non-null fields whose signed ownership byte is nonzero, releases each node and
decrements the live history count. Sixty fields retain their eight-byte strides.
Borrowed fields survive. The tail/current link restoration follows the original
ordered reads, writes and row reloads across callbacks.

The bound compares the **signed** history count with 100 before appending. Above
100, a marked head refuses insertion with zero return. An unmarked head is
removed, the new head's previous link is cleared and the count decreases once;
the subsequent append increases it again. A well-formed 101-node history thus
retains 101 nodes. This preserves the original threshold rather than introducing
a new limit or validating inconsistent legacy layouts.

Before allocation, the original current-node callback executes through actual
CFM dispatch with its stack TOC restore. A new 764-byte node is allocated and
cleared, receives identifier, auxiliary word, payload, callback, narrowed mark
and default label, and is linked into the row's head/tail/current fields. Marked
and total counts and cache flags keep their original widths and ordering.
Published-argument fallback initializes borrowed fields through the actual field
setter using the live vector and signed count at owner `+0x2c14/+0x2c18`.

## Guest ABI and control ownership

The bulk and current-row entries retain 96-byte and 144-byte frames. Ordered
multiple-register saves/restores preserve their generated scratch pointer,
SP/TOC/LR/PC, full register bytes, CR/XER effects, padding and caller arguments.
Checkpoints preserve exact finite instruction budgets and interrupted memory
prefixes. Named operations and structured traversal implement the bodies; there
is no opcode interpreter or host history layout.

Diagnostics extend the existing opaque owner; the State header/layout is
unchanged. Const queries allocate nothing, and reset preserves other subsystem
counters. Global/per-entry original selection, original profiling, normal
returns, faults, budget stops, known transfers, terminations and dispatch escapes
remain distinct. `CM_AOT_RECORD_HISTORY_ORIGINAL` is an explicit development path.

## Component and generator validation

The component harness passes **2,359** comparisons with **2,307** interrupted or
faulting outcomes, including **484** exact budget stops. There are two own ABI
negatives and a separate semantic-child field mutation rejected by parent
equivalence. The private reference retains exactly 393 original instructions,
disabled hooks and isolated same-page entry functions.

Cases cover single/multiple cached records, optional identifiers, signed cache
counts and selected rows, unsigned word identifiers, low-byte marks, marked
selection/restoration, signed history bounds, forward truncation, first/last
owned fields, actual fallback layout, borrowed fields, callback row redirection,
unaligned frames, null/malformed pointers, reporting faults and allocation-failure
prefixes. Normal fixtures must finish normally; unexpected matching faults do
not count as normal acceptance. Minimal mappings, every distinct accessed-byte
hole and every finite normal-fixture budget prefix agree in full guest images,
ordered effects, dependency register snapshots and accounting.

Component dependencies are controlled explicitly; the byte-exchange model retains
exchange semantics. Actual production checks separately execute the full original
helper and allocator/field/callback chain. Independent component assertions check
new-node fields/links/counts, record order, selected-row restoration and updates
only to matching records.

Generator audits pass **250** executed tests: 232 discovery tests plus eighteen
explicit-fixture tests covering three discovery class skips. No skipped scope
remains. Thirteen generated-storage regressions pass. Earlier prefix pins remain
unchanged; older assertions update only the global total and bound the accepted
query suffix.

## Actual production and integrated acceptance

The freshly generated Release ARM64 application and focused targets build with
**2,055 fresh object files**. All State-dependent objects use the same unchanged
State header and current source snapshot. The private reference generated in this
build matches the independently compiled component reference byte for byte.

Nine focused CTests pass: history component/production/profile/transfer,
record-group production/profile/transfer and current-record query
production/profile. The new actual production harness passes **25 comparisons**
and two root original-profile comparisons. Actual generated byte exchange,
allocators, aggregate construction, array disposal, field setters and CFM
callbacks execute in independent private environments. Own and semantic-child
negative controls discriminate changed effects. Application-exit first-write
prefix and boundary classification also pass.

Six independently asserted current-row scenarios cover forward truncation with
real owned arrays in fields zero and 59, marked selection and restoration,
well-formed 101-node marked refusal and unmarked trim/append, and published
argument fallback with two real borrowed fields. Bulk scenarios independently
assert original record order, selected-row restoration and changes to both or
only the identifier-matched cached record.

**25 nonlocal comparisons**, including **12 restore/handler faults**, retain
96/144-byte root frames, 240-byte nested history frames and a surviving 112-byte
parent. Actual capture/restore, scalar dependencies and CFM run with two scalar
modes and Turbo composition. Abandoned root and child continuations do not resume;
eight argument registers, ordered memory/effects and unmigrated dependencies
agree. Only the surviving parent's Carbon Gestalt result is controlled. Unknown
transfers are zero.

Seven affected units are newly instrumented with UndefinedBehaviorSanitizer.
Component, actual production, original-profile and nonlocal-transfer modes exit
successfully with empty stderr and byte-identical Release output. Existing
dependencies are linked from the consistent fresh build; the whole engine is
not claimed newly instrumented.

## Installed application and cycle cleanup

The signed installed ARM64 executable has SHA-256
`96d8399080b94158afe410834f39b5b7d19e48d815c61e91b9ba2d2a97682723`.
Candidate, fixed installer source, installed bundle and loaded process identity
agree. The owned installed smoke renders the menu in the 800×600 content-sized
window, reads and cancels the native File menu, observes the original quit
confirmation and exits normally. Both history entries execute, with **three
semantic calls and returns**, zero own failures, original/profile selection or
dispatch escapes. Dedicated actual production checks cover their additional
branches.

The installation preserves the newer externally saved `Reading.sav`; the old
preservation inventory is retained in a private update receipt rather than
restoring an earlier save. Current runtime files remain exact across installation,
owned smoke and cleanup. The original game, inputs, private references, 23 handoffs
and 216 prior bundles also remain exact. The previous fixed installer source is
preserved by rename. Across cleanup, all 1,924 prior generated files retain their
hashes, identities and metadata; this is a cleanup check, not a claim that they
were inventoried before generation.

Cleanup removes **4,128** previous reproducible build/test outputs and recovers
approximately **1.36 GB**, retaining the latest consistent build. Available space
after preservation checks is approximately **49.39 GB**. The complete modern
engine remains unfinished.

Save/Load is **not retested** and performance is **not benchmarked**. No career,
day, season, match or historical heap campaign is part of this bounded cycle.
