# Semantic Mac byte and item stream I/O

## Scope and integration

Nine C++23 operations reconstruct slow/fast byte reads, line reads, byte pushback,
item read/write bodies and their facades, and byte orientation. They retain the
Mac guest descriptor, big-endian metadata, word32 arithmetic and generated CFM
callback ABI. The accepted stream-storage/lifecycle and allocator blocks remain
semantic dependencies on both sides of the comparison. The oracle uses these nine
original generated bodies with those accepted productive children; it is not a
claim to compare against a fully unmodified historical engine.

The preceding accepted checkout and remote main are
`f71ae1b782c0cc8afaed337c90442a5478acd876`. The manifest appends these entries after
the exact prior 140, whose ordered compact JSON SHA-256 is
`5c4dfac37d687da95ad3c8c4ee0cc14ecd0a4c71d88618a5a79d270c3600d121`.
The configured total is 149. Original executable identity remains
`91692f95df17cf851e7c1d6dbdecd0bd0a5809ce1937133e209aee859a6b516b`.
Private indexed original listings, generated bodies and original payloads remain
outside Git.

Pins serialize address/assembly/ops/flows/fallthrough in ascending address order,
as sorted compact UTF-8 JSON. These end-exclusive bodies total 682 indexed
instructions on pages 0x10775, 0x10776 and 0x1077f. None crosses a page, and a full
index query finds zero external edges into their interiors.

| Entry | End exclusive | Instructions | Role | SHA-256 |
| --- | --- | ---: | --- | --- |
| `0x10775ad0` | `0x10775c04` | 77 | read byte slow | `162461279eab557ab0115dda7f04984b4f74750f453802c206ae0625e5a51dfe` |
| `0x10775c10` | `0x10775c80` | 28 | read byte | `9049d04e891eba4d580228fbf49a3af4a41196b0b3836fef5654f542709a69f1` |
| `0x10775c80` | `0x10775d78` | 61 | read line | `bd3a3acdf94f63c8e0364cdba0e9b9c701d636fdecb96d960ecfdcf57cd0def8` |
| `0x10775d80` | `0x10775e5c` | 55 | push back byte | `f3e8e6966664893074a6ce3ec965c7954b2f3a4ff3979b8681f7457da87ad2f3` |
| `0x10775fe0` | `0x10776000` | 8 | read items facade | `818a9159b17268c51c844beb14952940b7ddb4fe98b6462764c49f48e192a110` |
| `0x10776000` | `0x10776360` | 216 | read items | `21ff0ebab7e18057c44d46a22d70fa4adbc25dbf30677b9714c2ed03f7849b37` |
| `0x10776360` | `0x10776380` | 8 | write items facade | `307003035ca3f591b1a9ab184fc5807857a906f1c325c8b5ed6a69e4cc4ce9b8` |
| `0x10776380` | `0x1077668c` | 195 | write items | `567e3390bc0953d9f2116b92df6d2e1cee878985129a4c1d843005587ade6845` |
| `0x1077fa50` | `0x1077fad8` | 34 | byte orientation | `825de1cd6849578fc2abe2ade4032cb1dc57aef04e5433922d8935aba200e507` |

Hooks precede the first original operation at generated destination labels.
Direct, local, cross-page and CFM entry dispatch use the same hooks. The default
runs named semantic bodies; process-local global/per-entry original selection,
resume, counters, observers and reset follow the established adapter pattern.
Normal calls never execute their own substituted generated bodies. Diagnostic
original selection and dispatch escapes have separate counters.

## Guest contracts

The 84-byte descriptor and its packed mode/state bits retain the stream-buffer
report's layout. +52 is the remaining buffer count saved during byte pushback.
Byte unget slots use +16 plus operational state; wide unget slots use +22 plus
twice state-minus-three. Neither is replaced by host storage. All ABI frames,
GPR/FPR/temporary register bytes, CR/XER/CTR/LR/PC/TOC/FPSCR residues, access order
and first-fault PCs remain observable through the shared unchanged MacAbi helper.

Orientation queries distinguish unset, byte, wide and the reserved value three.
A first signed negative request makes byte orientation sticky; a positive request
makes wide orientation sticky. The first request is returned verbatim, including
noncanonical magnitude. Reserved orientation retains the original descriptor
pointer result; it is not normalized. Byte operations reject a nonnegative
orientation result. Null or a closed descriptor returns zero from orientation.

The fast byte path decrements count before deciding to call the slow helper,
including a transient word32 underflow from zero. The slow path clears count
before sticky-error/class rejection, handles pending bytes, or refills through
the live read vector. Status one sets error; other unsuccessful statuses, and a
successful zero-byte refill, set EOF and reset state as the original does.
The last pending byte restores +52. Pushback rejects output state, state four
and -1; other values truncate to one byte, clear EOF, and preserve legacy state
wrap rather than introducing a new validity policy.

Line reads use a signed, decremented extent. Negative decremented extents return null
without writing a terminator; extent one writes an empty string. A newline is
retained. EOF after a nonempty prefix returns a terminated partial line; an error
returns null with the already written prefix and no new terminator. Access/carry
residues from the decrements are retained.

Item operations orient even before a zero or wrapped-zero size product is
rejected. The product and all pointer/count arithmetic use low word32. Sticky
error or closed class rejects further transfer. Read permission/state transition,
line-output flush selection, pending byte/wide slots, buffered refill and binary
direct refill remain named branches. A direct transfer temporarily installs the
caller's buffer, retains live callback effects, then restores the saved base and
capacity in original order. The return is complete bytes divided by item size.

Writes retain class-two initialization and append seek as existing generated
dependencies. Idle writable state prepares the buffer; invalid write state sets
sticky error. Fully buffered, line-buffered and binary direct drain strategies
preserve newline search, partial copies, drain timing and the live final mode.
Any failed drain sets error, while its reported byte count is still credited.
The return rounds up a partially written final item using the original wrapping
numerator. Host checked multiplication, modern EOF rules and host FILE objects
are not introduced.

The existing Carbon BlockMoveData bridge snapshots the bulk source and then
writes the destination. Read and write aliases preserve that snapshot behavior;
no scalar observer is inserted into the native bulk bridge. Sparse-map fixtures
include import source extents, while fault controls remove bytes from actual
scalar/byte access records. Callback tests mutate payload/count, volatile GPRs,
CR/XER, TOC, descriptor base/capacity and flags; callers restore their original
TOC linkage slots and reread fields exactly where the Mac body does.

## Acceptance

The consistent Release ARM64 generation/build passed with 1,953,157 indexed
instructions, 1,922 pages, zero missing/pending destinations and 149 ordered
replacements. The harness passed **226 comparisons**, including **27 expected
mapping faults**, and rejected **13 discriminating negatives**: one per own entry
and four at coupled child boundaries before the parent can mask a mutation.
Every architectural/temporary byte, mapped guest byte, first-fault PC, allocator
state, ordered memory effect, CFM argument/link/refcon/TOC and import/native storage
history is compared. Only instruction accounting and process-local diagnostics
are excluded. Budget exhaustion is never accepted as a mapping fault.

Fixtures cover all direct and CFM entries; twelve mixed parent/child selections;
minimally mapped successes and three removed-access bytes per entry; relocated
frames and TOCs including the live BlockMoveData vector; zero/wrapped size products;
read/write permission and idle transitions; sticky/closed rejection; reserved
orientation and wide byte-access rejection; byte/wide pending slots; signed extent
wrap; partial EOF/error lines; partial item credit; buffered/text/binary modes;
all callback statuses; zero-byte slow refill; class-two initialization; append
seek success/failure; callback base/capacity/flag mutation; overlap snapshots and
an output alias of descriptor count. Independent reset preserves buffer diagnostics.

Two fixtures were repaired before acceptance: a pre-read observer initially
selected EOF one callback too soon, and the relocated TOC omitted the external
copy vector. Original and semantic bodies agreed on both observed results;
production code was not changed to satisfy these faulty expectations. Failed
attempt receipts remain private. The five related CTests passed before these
harness-only repairs and were reused; the final new CTest passed after repair.
The six accepted CTests are stream I/O, stream buffer, formatted input, formatted
output, memory primitives and native bridges.

Seven new pin/hook/index audit tests and all 60 tests in eleven affected prior
audit suites passed, retaining the exact historical manifest prefixes. Three
focused UBSAN executables passed with empty diagnostic stderr: the new I/O harness,
the stream-buffer harness and the probe. The four new I/O units, shared runtime,
probe and their harness sources were instrumented and linked with the consistent
generated library. Unchanged dependencies and the complete generated engine are
not claimed to be fully instrumented.

All nine focused entries have direct=1, original=0 and escapes=0. The installed
smoke and both isolated career logs likewise have all nine modes modern, zero
original calls and zero escapes. UI zeros remain visible: line reads and pushback
are proven by their focused fixtures, not by these UI flows.

| Entry | Focused normal calls | Installed menu smoke | Isolated load/save/exit | Isolated reopen/load/exit |
| --- | ---: | ---: | ---: | ---: |
| `0x10775ad0` | 15 | 112 | 208 | 119 |
| `0x10775c10` | 4 | 439535 | 823689 | 465158 |
| `0x10775c80` | 14 | 0 | 0 | 0 |
| `0x10775d80` | 23 | 0 | 0 | 0 |
| `0x10775fe0` | 9 | 292318 | 300412 | 299778 |
| `0x10776000` | 57 | 292318 | 300412 | 299778 |
| `0x10776360` | 9 | 8 | 10560 | 3019 |
| `0x10776380` | 57 | 8 | 10560 | 3019 |
| `0x1077fa50` | 297 | 731887 | 1135138 | 768207 |

### Isolated persistence

A separate signed diagnostic bundle and a resource-fork/metadata-preserving copy
of the installed Runtime were used outside the installed support directory.
The original Reading save remained unchanged. The private game loaded Reading,
selected the original in-game Options -> Save Game flow, created `streamio.sav`,
returned to the squad, exited normally, reopened and loaded that newly created
file. The reloaded screen retained Reading, Victor Moraes, Tuesday 7.8.01 NTE and
the visible squad including Ashdown, Mackie, Cureton and Tyson. No day or match
was advanced. The save is 20,121,077 bytes, SHA-256
`9b7f2143881b356775111c329819f2af8c9eaf2c499ce8b994e0191fcaa85339`,
unchanged by reload. Native file logs show private temporary-save writes, successful
rename to streamio.sav and subsequent private reads. Both career sessions ended
normally with all nine modes modern, zero original calls and zero escapes.

This is an isolated Save/Load runtime witness for the compiled I/O candidate;
it is not installed-production Save/Load or a full serializer audit. The initial
production-pointer attempt could open the chooser through a named command but
could not select a career: CUA background clicks carried Command and their event
coordinates differed from the live desktop cursor. The accepted isolated flow
used the existing diagnostic event-coordinate mode and its Validation click
menu, which sends unmodified original mouse events. No guest-PC/data shortcut or
new input bypass was added. Earlier subsystem persistence waivers remain not
retested for their original delivery.

The source/candidate executable SHA is
`5e219ce7b0c4e35a0761f0ba4ff52a326873bfe1faf9f7e99dd60172ec6c373a`.
The event-validation bundle has a different signed executable SHA,
`425c893f29ec8074cf1e7afa39c58e0c77edda0f946b7e06d9c6b5fd95bad036`;
both ARM64 binaries have the identical compiled __text section SHA
`91bdb224a2b79d182a8cc8e09797941a49843d7c167b8d091385b0356a93473a`.
The private bundle ID/configuration and signing difference are retained in receipts,
and are never shipped in the normal production bundle.

### Installed artifact and preservation

The linker, signed source, accepted candidate and pre-install reference are
preserved privately. Installation ran once after technical and isolated persistence
acceptance with no game process active. The internal ARM64 executable in source,
candidate and installed application matches SHA-256
`5e219ce7b0c4e35a0761f0ba4ff52a326873bfe1faf9f7e99dd60172ec6c373a`.
The preserved reference is
`41326fc78142dcd8bd323c4139f6453842c0e1cf977be180378af718c7f9e97f`.
Strict signatures, normal production launch metadata and all four original bundled
payload hashes passed. The running installed smoke process used this exact
executable. It rendered
Prepare Game, opened the original saved-game chooser showing only Reading,
cancelled by the named Back or Cancel command, displayed exit confirmation and
exited normally. No fatal diagnostics occurred and no game process remains.
No career was loaded in the installed smoke. Final process absence was checked
without post-exit accessibility calls, avoiding unintended app relaunch.

Before installation and immediately after it, full content/metadata inventories
matched for 809 Game entries, five Runtime entries, 60 Runs entries, workspace
saves, presentation preferences, the new pre-install reference and 126 historical
bundles. Content hashes also matched for the four original payloads and all 23
handoffs. After UI use, full content/metadata inventories still matched for Game,
installed Runtime save files, workspace saves, reference and historical bundles,
including resource forks, extended attributes, Finder information, modes,
timestamps and flags; payload and handoff content hashes still matched.
Runtime preferences, Runs and
presentation preferences may change legitimately and are not claimed byte-identical
after UI. All original/temporary listings, indexes, generated sources, payloads,
saves, logs, diagnostic bundles, receipts and the 18 private handoffs remain
outside the product commit. No visual/performance improvement or whole-game
compatibility is inferred from this stream acceptance.

The complete engine/runtime goal remains active; seek/tell, open/mode parsing and
other generated file services remain separate dependencies.
