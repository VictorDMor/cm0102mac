# Current-record query reconstruction

## Production scope

Six complete Mac bodies implement argument publication, counting records whose
two cache marks are clear, current-node first/last predicates, an unsigned node
mark and a payload-presence predicate as named C++23 operations. Original error
reporting remains a production dependency. Normal paths never invoke their own
replaced original bodies.

Full generation appends six `record_query_adapter` records after the accepted
590, giving **596** semantic replacements. The 590-entry prefix retains digest
`df2582404500734e3ee82bca17aa9deed4bcbcddcd7079cd92cf080a62c63740`.
The full replacement digest is
`ab68bf9563f7939b11a3c3c358bee40862331a129d17efb9bb97778681cc6af6`.
The application root, all 1,953,334 indexed instructions, 1,922 pages, generated
file inventory and five source fixups remain unchanged; missing and pending
targets are zero. Full-index flow/fallthrough audits reject exterior entrances
into body interiors. All six destination hooks precede original operations and
the instruction counter.

## Complete boundaries

Intervals are end-exclusive. Fingerprints include address, assembly, operations,
flows and fallthrough in the established serialization.

| Entry | End | Instructions | Operation | SHA-256 |
|---|---|---:|---|---|
| `0x1073e990` | `0x1073e9b0` | 8 | Publish record arguments | `c9354e753ad5384aa16f13a82d9785889419b57c7eeb2ba5db74b68b8e62abc8` |
| `0x1073e9b0` | `0x1073ea14` | 25 | Count unmarked cached records | `c53cc349f72035738ecfe04a3c7fff6a62160411927db4737f7246a8a883dc33` |
| `0x1073ea20` | `0x1073eaf4` | 53 | Current node is last | `379dfce53b881afee450b8034ae64861a9dde6e465ba56ba4faa6a7517faab11` |
| `0x1073eb00` | `0x1073ebfc` | 63 | Current node is first or eligible marked node | `0cc74cf9b3730893857f183b9325806dde8800a2062c0164112a80190a2378f7` |
| `0x1073ec00` | `0x1073ecc8` | 50 | Current node mark | `9370ea978adb7a3aa42b6d3c7a3811256d833bf6208d986f0a630066b1b609aa` |
| `0x1073ecd0` | `0x1073eda8` | 54 | Current node has payload | `08527aee88cafef55374876b207d4d60623228413d32eb87a6c7b6cc797cd3fc` |

Total: **6 entries / 253 indexed instructions**, all on page `0x1073e`.
Missing, changed, duplicate, reordered or absent bodies and missing, duplicate
or misplaced hooks have negative controls. Original references remain private.

## Preserved behavior and ABI

The leaf publisher retains seven ordered stores at owner offsets `0x2c00`,
`0x2c04`, `0x2c08`, `0x2c0c`, `0x2c10`, `0x2c14` and `0x2c18`. Arguments r4 through
r10 retain their original word/byte/halfword widths and truncation. Padding stays
untouched; the owner and argument registers remain live across memory callbacks.
There is no new null check, initialization or allocation policy.

Counting retains 44-byte cached records and two signed-byte marks at offsets 25
and 26. A record counts only when both are zero. Each loop rereads the signed
selected row at owner `+0x2c58` and its signed halfword count at `+0x2c38+row*2`.
The loop index is narrowed to a signed halfword while offsets and accumulation
keep word32 arithmetic. Callback changes to the row/count are immediately visible.

The four framed queries validate the selected row's current-node pointer at
`row*0x2c0+16`, then reread the selected row/current node for the result. Missing
nodes retain the actual reporting dependencies, guest temporary slots, diagnostic
lines `0x5cc/0x5df/0x5f3/0x606`, live TOC-derived error latch and application-exit
call. No modern early return replaces those effects.

The last-node predicate tests node `+0x1f8` for zero. The first-node predicate is
true for a null previous link at `+0x1f4`; with a previous link, a zero low argument
byte permits a nonzero signed node mark at `+16` to qualify as first. A nonzero
low argument byte suppresses that marked-node exception. The mark getter returns
the byte unsigned, including values 128 through 255. The payload predicate tests
the full word at node `+12`, preserving its intermediate zero-test register effects.

Two entries are leaves; four retain their original 112-byte frames, saved
registers and ordered restores. Guest checkpoints preserve SP/TOC/LR/PC/CR/XER,
full register storage, scratch bytes, caller arguments, ordered effects and
finite budget prefixes around the named operations. No opcode interpreter or
host container supplies the semantic body or replaces the guest layout.

Selection and counters extend the existing opaque diagnostics owner. The State
header/layout is unchanged; const queries allocate nothing and reset preserves
other subsystem diagnostics. `CM_AOT_RECORD_QUERY_ORIGINAL`, per-entry selection
and original profiling remain explicit development paths with separate outcomes.

## Component validation

The component harness passes **496** comparisons, with **416** interrupted or
faulting outcomes including **196** exact budget stops, and six own ABI negatives.
Its reference retains exactly 253 original instructions, disabled hooks and
isolated same-page entry functions. Controlled reporting dependencies record full
register snapshots and ordered effects. Original and semantic bodies execute
independently.

Cases cover byte/halfword narrowing, unaligned and stack-aliasing publication,
callback owner redirection, signed row/count/index/mark behavior, zero/nonzero
word predicates, all low-byte option/mark combinations, unsigned mark results,
missing nodes, reporting faults and live reporting TOC changes. Read callbacks
change row counts, select a different row during counting and redirect framed
queries between validation and result lookup. Minimal mappings, every distinct
accessed-byte hole and every finite normal-fixture budget prefix agree. Initial
normal fixtures must return normally; unexpected or divergent faults fail.

## Integrated validation

Seven focused CTests pass: component, actual production, profile and nonlocal
query checks plus directly related record-group production/profile/nonlocal
checks. Generator audits pass **243** executed tests; discovery's 13 skipped
classes/tests are covered by 28 explicit-fixture tests, leaving no skipped scope.
Thirteen generated-storage regression tests pass. Earlier prefix pins remain
unchanged; affected older assertions update only the global total and bound the
accepted record-group suffix.

Actual production passes **54** direct/generated/CFM/global/per-entry/negative
comparisons and six original-profile comparisons. Every default root has two
direct, two generated and two CFM comparisons plus an own ABI negative. Full
guest images, ordered effects, actual import-boundary register snapshots,
accounting and balanced boundaries agree. Default paths have dedicated nonzero
semantic counters, zero original/profile selection and no dispatch escapes.
Argument narrowing and every query result have independent assertions. A separate
application-exit prefix distinguishes termination from returns and faults.

Prerequisites use actual allocators and the original aggregate constructor on
independent platforms with private user directories, real cached records and
allocated current nodes. Both clear/set marks, null/non-null links and payloads
execute without opening user files. Reporting dependencies remain actual in
production; controlled reporting fixtures are explicitly component evidence.

Another **46** known nonlocal comparisons cover two leaves and four 112-byte
query frames, both scalar-context implementations, mixed capture/restore selection
and Turbo, including **24** restore/handler faults. All eight root argument
registers survive parent capture. Only the surviving parent import is controlled.
Actual query, allocation, CFM and scalar-context implementations execute; abandoned
continuations never resume. Ownership, boundary classification, ordered effects
and exact accounting outside the measured scalar dependency agree. Unknown
transfers remain zero.

Seven affected units pass UBSAN in component/production/profile/transfer modes
with empty stderr and byte-identical Release output. Unchanged dependencies use
the fresh consistent archive and are not newly instrumented. All 2,036
State-dependent build objects are compiled in the same fresh Release build.

Save/Load is **not retested** and performance is **not benchmarked**. No career,
day, season, match or historical heap campaign is part of this bounded cycle.

## Installed delivery and retention

The signed ARM64 candidate, installer source, installed application and loaded
process have executable SHA-256
`456b0195b0ebd66dbd23a4aaee28f4982db2b5492a0304b68aa55d6f49b4d559`.
Strict signature verification passes; the four original payload files match.
The prior installer source is preserved by rename and the prior installed app
has an exact private reference. The protected installer preserves all 809
original game files and four current runtime files, including the current save.

The owned installed smoke reaches the original menu in its 800×600 content-sized
window, reads and cancels the native File menu, observes the original Portuguese
quit confirmation and exits normally. PID absence and the original normal-exit
marker agree. Entry `0x1073eb00` executes once with one semantic call and return;
the other five have dedicated actual production coverage above. All six smoke
rows have zero original/profile selection, own faults, budget stops, transfers,
terminations and dispatch escapes. No save or load is performed.

Cleanup removes **4,126** old reproducible record-group build/test outputs,
recovering **1,419,870,208 available bytes** (approximately 1.42 GB). The latest
consistent query build is retained. All **212** preexisting project app bundles,
23 handoffs, original inputs, private references, game and current runtime files
remain exact. The previous generation's 1,924 files retain their hashes, sizes,
inode identities and metadata after cleanup. Available disk space after the
preservation checks is **50,637,873,152 bytes** (approximately 50.64 GB).

The complete port remains unfinished; these are six additional semantic bodies,
not a claim of full gameplay, persistence or performance acceptance.
