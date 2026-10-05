# Semantic Mac basic strings and descriptors

## Scope and evidence boundary

Fourteen Mac basic string and descriptor entries are reconstructed in readable
C++23. This closes the NUL-terminated byte-string core used by semantic collection
queries and background selection. Language services, templates, general
formatting, numeric parsing, character classification, table initialization,
allocator and general memory primitives remain generated. The large service at
`0x1077de10` remains outside this block. No visual or performance improvement is
claimed, and this is not reconstruction of all engine text services.

The starting checkout is `main` at
`5d1efac195e73e3dddd3863bb3ac34028cb723bb`, independently verified on the existing
`git@github.com:VictorDMor/cm0102mac.git` remote. The thirteen untracked handoffs
are preserved and excluded from the product commit. No applicable `AGENTS.md`
was found in the repository or its ancestors. The private Mac SQLite index is
opened read-only; no new export or proprietary listing is published.

Date, RNG, presentation, backgrounds and collection reports describe historical
acceptance. They are not evidence for this block. Save/Load, save/exit/reopen/load,
career comparison and serialization auditing are expressly waived by the user
on October 5, 2026. Persistence is **not retested**, neither passed nor a pending
gate. No live app session or personal career is opened for this delivery;
synthetic composition fixtures are not UI evidence.

## Pinned original boundaries

Normalized records contain `address`, `assembly`, `ops`, `flows` and `fallthrough`,
ordered by address, serialized as sorted compact JSON with UTF-8. The audit pins
all original bodies and rejects changed, partial or duplicate indexed records.
Internal gaps are intentional. The wrapper has eight indexed instructions;
the handoff's seven-instruction cell was a transcription error. Its fingerprint
is unchanged, and the correct total is **295 indexed instructions**.

| Entry | Exclusive end | Indexed instructions | Operation | SHA-256 |
| --- | --- | ---: | --- | --- |
| `0x107393f0` | `0x107394dc` | 58 | Mac table comparison | `6f374bd1dea6768a6421e7051816861b7cbf0c59614b21b308005c6581bec66d` |
| `0x107394e0` | `0x107395f0` | 66 | Signed16 Mac table comparison | `c23f8fb6a286136572ec06ecc9eeca532a402f40b68c32b3d067806e79628a53` |
| `0x1074de10` | `0x1074de30` | 8 | Descriptor ABI wrapper | `f16b28e9df93c459efaf7236ff6d18a0f15e97eb1126ed662e814e0a6ffe6f03` |
| `0x107395f0` | `0x10739640` | 20 | Allocated duplication | `ac606d3aa261c47d1fbdaa1f0d5b172bb243fcae4dc0a394155227854fbe39e4` |
| `0x10739640` | `0x1073967c` | 15 | In-place Mac table mapping | `efa50a960b458ddba3e0d46cac87f6c8058fc784e97abad96bba13fc4006878f` |
| `0x1077dbd0` | `0x1077dbec` | 7 | NUL-terminated length | `d877b190f9567050ea7f43437dff67853054426d5b9841fe83432bfe6680d4fb` |
| `0x1077dbf0` | `0x1077dc0c` | 7 | Forward copy including NUL | `774d415f9f2b8f7f1ecddcc18432a8a87aaba3a51506c94eae9dc9b462052838` |
| `0x1077dc10` | `0x1077dc54` | 17 | Word32 bounded copy and padding | `b54b65eae1a77dca62311420c12f051c7b5ea5949d047d60287185be2cabe465` |
| `0x1077dc60` | `0x1077dc8c` | 11 | Forward concatenation | `6dfee147a562bb700896eb5e9f31e620be83e74cbb0b07bb526f32b9f2423a91` |
| `0x1077dc90` | `0x1077dcc8` | 13 | Unsigned-byte difference comparison | `5717c6b7d0892339f0b3784045f4cc34d6421c7dfbc6f56cb865028ea7c469cc` |
| `0x1077dcd0` | `0x1077dd10` | 16 | Word32 bounded byte comparison | `79aca0a48020beeb690115fd1e124bbdcd748aa7c70be9b8501bd6299ad063ae` |
| `0x1077dd10` | `0x1077dd44` | 12 | First byte occurrence | `5fe3ff62a0b22e9d4c00e4e50c887920c159cbd6af02688617124168cf297b20` |
| `0x1077dd50` | `0x1077dd98` | 18 | Last byte occurrence | `3ac24746630eecd0833c589527c2e401291a2c2ee97bea64f7f156ee8d02425b` |
| `0x1077dda0` | `0x1077de0c` | 27 | First substring occurrence | `eb0d586f64e7df8aba82ff0f40db56cef3169fe98328dd02254ebdbc96643208` |

Every indexed static flow and fallthrough is inspected for edges from outside
each body into its interior. There are zero such edges for these fourteen
entries. This statement does not classify unknown dynamic entry points.

## Original contracts and compatibility

The complete Mac comparator receives left/right in r3/r4. Its initial r3 null
comparison precedes the mandatory table-pointer load from `r2-0x62bc`, including
null arguments. Null arguments return zero if that load succeeds. It reads raw
left then right bytes and terminates only on raw NUL. Other bytes are extended
as signed8; raw `0xff` gives -1 without a table lookup. All other bytes index the
live table by unsigned8, and its mapped byte is interpreted as signed8. Results
are exactly -1/0/+1. The table pointer is captured once, its contents remain
mutable, and repeated pointers/terminator reads are retained. r3/r5 are cursors,
r4 retains the right base and r9 retains the index. The key-read PC remains
`0x10739484` for the existing adversarial collection observer.

The limited Mac comparator truncates r5 to signed16. It reads the next left
byte, and a right byte when left is non-NUL, before checking the limit. Zero,
negative and exhausted limits can fault on that lookahead; after N equal
characters it may read index N. It uses r11 and r6, retains the termination-time
signed16 reduction and the original right-base rereads. It is distinct from
the word32 bounded byte comparator.

The descriptor wrapper creates a 64-byte guest frame, writes LR and backchain
before comparison, calls the same guest `0x107393f0` entry and restores its own
link/frame. Spills may alias strings. Dependency clobbers remain visible.
Collection lookup reloads each mutable vector and uses the existing generated
transition ABI; the comparator is not globally fixed. The original background
vector observation is entry `0x1074de10` with TOC `0x108bba90`. The existing
alternative `0x1077d9e0` comparator fixture and its signed32 values remain intact.

Length performs no null guard, loads through the raw terminator and increments
its length modulo 2^32, retaining r4 and CR. Copy loads and stores forward
through NUL and returns the original destination; r4/r5 finish at their original
positions. Concatenation scans the destination before any source byte load.
Bounded copy uses r5 as a word32 count, initial increment and pre-work decrement
with wrap, XER carry and recorded CR. It copies at most the count, pads remaining
bytes with individual zero stores after NUL, reads no more source during padding
and adds no terminator when truncated. A zero count does not touch pointer bytes.
Overlaps retain progressive effects; no `memmove`, bulk clear or pre-read is used.
Updating accesses commit their cursor only after success. The comparison between
load and store and all effects before a fault remain observable.

Byte comparison loads both unsigned8 bytes before equality/termination and
returns the exact difference in [-255,255]. Its bounded variant uses a word32
counter in r6, with increment/decrement wrap, carry and CR. Zero does not load
pointer bytes. First/last byte searches truncate the requested byte to unsigned8,
return the respective guest address or zero, and can return the NUL terminator.
They retain scan order, registers and flags.

Substring search returns the original haystack for null or empty needle without
reading haystack, retaining the cursor adjustment. A nonempty needle adds no
haystack null guard. Candidate scans, repeated prefix reads, inner haystack-before-
needle loads and the first matching candidate remain explicit.

Duplication preserves its 64-byte frame, r30/r31 spills, backchain and LR. It
calls semantic length, adds one byte with word32 wrap, calls the original
`0x10775130` allocator, and calls semantic copy only for a nonzero result.
Allocator effects, imports, registers and faults remain supplied by the original
engine ABI. Controlled result intervention in the harness proves zero-result
and copy-fault branches; it does not claim a naturally occurring allocation
failure or a reconstructed allocator.

Mapping independently loads the second table pointer from `r2-0x618c`, then
transforms each raw non-NUL byte in place. Raw `0xff` is written unchanged without
lookup; raw NUL is not written. Mapped zero does not stop the following scan.
It returns r3 advanced to raw NUL, and aliases can alter subsequent table lookups.
The observed original comparison/mapping tables are distinct at `0x1091c978`
and `0x1091c878`; neither is replaced with a fixed host/Unicode/locale table.

Named semantic operations share explicit ABI helpers for Mac32 big-endian
accesses, word wrap, signed reductions, fault PCs, flags, guest calls and frames.
No normal semantic leaf executes an original replaced body or dispatches opcodes.
Complete architectural register/temporary bytes, LR/CTR/TOC and mapped memory
remain the focused equivalence boundary.

## Production integration and focused harness

Destination-label hooks precede the first original operation on generated pages
`0x10739`, `0x1074d` and `0x1077d`. Same-page calls/gotos, page dispatch and indirect
vectors all reach these hooks. The manifest appends fourteen entries to the
unchanged ordered 46, for **60 semantic replacements**. Date's ascending-year
leap asymmetry and the club/competition null-getter guards at `0x10713aec` and
`0x10713b3c` remain intact.

Private string-only and per-entry original selections are independent of Date,
RNG, backgrounds and collections. `CM_AOT_STRING_CORE_ORIGINAL` is the probe
selection. Optional counters/boundary observers are process-local diagnostics,
not serialized in saves/snapshots, and add no guest reads or replace existing
memory/drawing observers. Normal defaults select all fourteen semantic entries.

`cm-string-core-checks` / `modern_string_core_equivalence` starts original and
semantic runs from identical state with the older modules still semantic.
It compares all architectural registers/temporaries, PC, every mapped memory
byte, allocator/import state and interleaved read/write/map/clear effects,
including fault prefixes. Instruction counts and private diagnostics are
excluded. Expectations for results, bytes, returned pointers, read order, table
lookups, requested allocation size and cursor/count/carry effects are independent
of the new adapter.

The bounded matrix covers null/empty/prefix/equal/different strings; original and
mutable signed-byte tables, `0xff`, mapped zero, relocated TOC, signed16 limits
and truncation/lookahead, exact unsigned differences and word32 limits; copies,
padding, concatenation, progressive overlaps and finite missing terminators;
first/last/high/truncated byte searches and repeated-prefix substring scans;
actual allocated duplicates and selected allocator/dependency faults; mapper
stores, raw termination and self-table aliases; exact accessed-byte mappings,
partial faults, repeated pointers and wrapper spill aliases. Extreme counts use
small mappings to reach discriminating faults without enormous loops.

Each of the fourteen entries has its own return-boundary mutation, with nested
child boundaries checked independently and diagnostics reset. Mixed modes cover
original wrapper/semantic comparator and the inverse, and original duplication/
semantic length-copy and the inverse. Normal witnesses require all fourteen
counters nonzero and no original-selected execution. A generated descriptor
vector transition and two background/collection/descriptor/table/RNG fixtures
exercise the dependency chain. The existing collection regression retains its
adversarial vector/child/register mutation and alternative signed32 comparator.
Six tiny semantic Date fallback fixtures call the new copy, and two normal
formatter fixtures reach length and byte search. These are affected-dependency
checks, not a Date presentation, historical stream or career campaign.

## Validation and protected delivery

One regeneration produced 1,953,157 indexed instructions on 1,922 pages,
zero missing/pending targets and 60 replacements. The consistent Release ARM64
build rebuilt all dependent generated objects after the `State` diagnostic
layout extension; it completed without compiler warnings or errors.

| Executed check | Result |
| --- | --- |
| `modern_string_core_equivalence` | PASS: 314 comparisons, 62 unmapped-access faults, complete architecture/memory/ordered effects |
| Own return-boundary controls | PASS: fourteen entry-specific mutations rejected; nested children and diagnostic reset checked |
| Normal string dispatch | PASS: fourteen nonempty entries, zero original-selected executions in normal witnesses |
| Mixed dependency modes | PASS: four wrapper/comparator or duplication/length/copy combinations |
| Generated descriptor vector | PASS: wrapper and comparator dispatch through the existing indirect transition |
| Background/collection/string/RNG compositions | PASS: two nonempty pictures with original/mutable tables and identical RNG consumption |
| Affected Date/formatter fixtures | PASS: six fallback copies and two normal formats with literal suffixes reaching byte search and length |
| `modern_collection_lookup_equivalence` | PASS: existing adversarial/alternative-comparator fixtures retained |
| `modern_background_selection_equivalence` | PASS |
| `original_background_selection` | PASS: original selector and null-getter-guard regression |
| `original_decimal_formatting` | PASS |
| `python3 tools/test_string_core_audit.py` | PASS: five fingerprint/body/hook/edge/manifest/dependency tests, no private-index skips |
| `python3 tools/test_collection_lookup_audit.py` | PASS: five tests, only manifest extension expectations changed |
| `python3 tools/test_background_selection_audit.py` | PASS: four tests, only manifest extension expectations changed |
| Focused UBSAN | PASS: all 314 comparisons and fourteen negatives, empty stderr |

The first five-CTest run passed the four existing regressions. The new harness
exposed two fixture issues in the formatter witnesses: sentinel FPSCR bytes
selected non-IEEE arithmetic, and formats ending immediately after `%f` did not
reach the scanner's trailing-text length branch. The fixtures now set valid IEEE
status and use `%.2f units` / `%08.2f!`. Only the new harness target and failed
CTest were rebuilt/rerun. No production adapter correction or repeat of the
already successful background/collection/decimal campaigns was required.

UBSAN instruments exactly `string_core_adapter.cpp` and
`string_core_checks.cpp`, with C++23/ARM64, `-O1`, `-ffp-contract=off`,
`-fsanitize=undefined` and `-fno-sanitize-recover=all`, linked to the consistent
engine libraries. It is neither whole-engine instrumentation nor ASan. No full
CTest/Python suite, Date/RNG/allocator campaign, historical replay or live UI
validation was run. Acceptance closes at the explicitly focused boundary.

### Protected installation

The pre-delivery installed app was copied once to the new private
`.local/string-core-reconstruction/reference/installed.app` and fully compared.
Its ARM64 executable SHA-256 is
`bb3ddaf091677f05124a9b82ab9afe4c2c65edb1f849d5d07abe6d6e17ebab8d`.
Both earlier background/collection installed references are preserved in full.
The baseline covers 809 Game files, five Runtime files, 46 Runs files and
18 retained prompt hashes, including the thirteen untracked handoffs.
Content, resource forks, Finder metadata, extended attributes, modes,
modification times and flags are protected. Original segment hashes are retained.
The presentation preference was absent and remains absent.

The compiler/linker bundle is retained separately as `source-linker.app`.
Original payloads were bundled before ad-hoc signing the build source; its
signed candidate is retained as `candidate.app`. The protected
`python3 tools/install_native.py` ran **once**, on October 5, 2026, from
21:34:57.953 to 21:34:58.636 UTC (18:34:57.953–18:34:58.636 in São Paulo).
Its normal existing-data and English club-index checks passed.

| Artifact | Architecture | Executable SHA-256 |
| --- | --- | --- |
| Retained compiler/linker output | ARM64 | `2356e64790f0f8bea5b89d191ae301ed7fdd7232766a6fd6bbe03b74ebab8d98` |
| Signed build source and candidate | ARM64 | `733a5fca03917f5e74fdd35109de93caac221b522cd8b8098b0c9f509fe0e721` |
| Installed app | ARM64 | `733a5fca03917f5e74fdd35109de93caac221b522cd8b8098b0c9f509fe0e721` |
| New prior-installed reference | ARM64 | `bb3ddaf091677f05124a9b82ab9afe4c2c65edb1f849d5d07abe6d6e17ebab8d` |

Source, candidate and installed bundles pass `codesign --verify --deep --strict`.
All four bundled original files match the existing private inputs in each.
Production source/installed plists are identical, without diagnostic/validation
keys or `LSEnvironment`. The one postinstall preservation receipt confirms
identical personal content and metadata, unchanged original segment/prompt
hashes, unchanged new reference and both earlier reference inventories.
No live app session or career was opened. Installation identity does not add
fresh gameplay, UI or persistence evidence.

Only product code, focused checks and these technical reports are delivered.
Private handoffs, proprietary payloads, saves, snapshots, screenshots, logs,
fixtures, candidate bundles and receipts remain untracked under their existing
protection. Ignore and LFS rules are unchanged. Publication uses the existing
private `origin` / `main`, reconfirmed with ADMIN permission, followed by a fresh
remote-SHA check.
