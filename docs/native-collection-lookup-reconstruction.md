# Semantic Mac collection lookup core

## Scope and original contracts

This delivery reconstructs five related collection queries in semantic C++23:
list indexing, preorder tree indexing and descriptor lookup. It preserves the
original Mac results, traversal order, callback behavior and architectural
effects. Collection lifecycle operations and descriptor comparison remain
generated. This is one collection-query block, not a reconstruction of the
whole engine or of collection loading, insertion, removal and release. No
visual-quality or performance improvement is claimed.

The starting checkout is `main` at
`b2b696610f4b993f818c517d0bd6b0c81d599bd3`; a fresh remote query confirmed that
revision on `git@github.com:VictorDMor/cm0102mac.git`. The twelve untracked
feature handoffs are preserved and excluded from the product commit. The
original private Mac instruction index is read-only; no new Ghidra export,
oracle or proprietary listing is published.

| Entry | Exclusive end | Instructions | Contract |
| --- | --- | ---: | --- |
| `0x1074dd40` | `0x1074dd94` | 21 | Read list head, then follow the requested signed32 index |
| `0x1074df60` | `0x1074df8c` | 11 | Store a mutable position in the guest caller area and invoke preorder lookup |
| `0x1074df90` | `0x1074dfb8` | 10 | Load collection root and invoke descriptor search |
| `0x1074e150` | `0x1074e5a4` | 277 | Preorder traversal with wrapped position decrement and four unrolled levels |
| `0x1074e5b0` | `0x1074ea30` | 288 | Mutable-vector comparator search with four unrolled levels |

List lookup receives the collection in r3 and signed32 index in r4. It reads
the head at collection `+0` before deciding whether the index is positive.
Zero and negative indices return that head. Positive indices follow node
`+4` exactly as requested, in groups of eight followed by the remainder. The
list count is ignored. A null head can return zero for a nonpositive index;
following a null link faults at the original access site.

The positional wrapper uses a 64-byte frame and stores r4 at oldSP `+28`.
This remains guest memory with observable aliases. The helper receives the
collection, root and address of that mutable position. A null root returns
zero without reading the position. Position zero selects the current node;
otherwise the helper writes a 32-bit wrapped decrement before reading the
children at node `+8` then `+12`. Exhaustion returns zero with the residual
position retained. Four unrolled levels and 80-byte recursive guest frames
remain observable.

Descriptor search does not call the comparator for a null root. For each
present node it reloads the transition vector at collection `+4`, passes the
key at node `+0` and the requested descriptor, and follows child `+8` for a
signed negative result or `+12` for a positive result. Zero returns the node.
The selected child is loaded after the comparator returns. Comparators may
change their vector, children and context; their register clobbers remain
visible. The existing `0x10781320` transition stub, callback stack slot and TOC
protocol execute through the existing guest ABI. Search retains four unrolled
levels and 80-byte recursive frames.

The generator pins the normalized original `address/assembly/ops/flows/`
`fallthrough` records, in address order, using sorted compact JSON and UTF-8:

| Entry | SHA-256 |
| --- | --- |
| `0x1074dd40` | `6616dd13a4bb0987aea301ef1223f923e4ba1eb9c6ff19dd4f8be72015e23780` |
| `0x1074df60` | `2f1b87f82d1b17d062d211efef96e018ca1037a33ecbf7949cc798bdecc1fab1` |
| `0x1074df90` | `cc4f5a4343d038fa57d92058142c9b659d3d975729e068973098227a5936d5d0` |
| `0x1074e150` | `7d90da28bd1aa7a1b61e53cd33f2b9a4b25040330a8b92df582e341f3f45582c` |
| `0x1074e5b0` | `aac339e33d1c6f7e3d8ec5e88d6ce4e915cd9d00317e6f4e21a76b88785882e5` |

## Production integration

Destination-label hooks on generated pages `0x1074d` and `0x1074e` intercept
the five entries before their original first operation. Background selectors
continue to call the same guest addresses. Same-page calls/gotos, inter-page
dispatch, recursive calls and transition-vector callbacks reach these hooks.
Normal production uses semantic implementations without executing their
original bodies. The manifest appends five entries to the preserved 41 Date,
RNG and background entries, for a total of 46.

All indexed flows and fallthroughs were checked in the read-only instruction
database. There are no external static edges into these five interiors. External
entry sources are `0x10712c14` for list indexing, `0x1071248c` for the positional
wrapper, and `0x10711ec8`/`0x10713a20`/`0x10739118` for the search wrapper. The
helper entry sources outside their own bodies are their wrappers at
`0x1074df78` and `0x1074dfa4`. Recursive helper branches remain internal to their
respective bodies. Static evidence does not classify unknown dynamic entries.

The private collection-only reference selection is independent of Date, RNG
and backgrounds. Per-entry selections support mixed wrapper/helper checks;
`CM_AOT_COLLECTION_LOOKUP_ORIGINAL` selects the private reference in the probe.
Optional observers and counters are process-local diagnostics outside saves
and snapshots. Existing memory and native drawing observers remain intact;
diagnostics add no guest reads.

The list/tree query adapter retains big-endian guest accesses, wrapped
arithmetic, signed branch decisions, PC/LR/CTR/TOC, condition and exception
flags, register and temporary residues, backchains, spills, reloads and fault
prefixes. Compatibility details are explicit alongside named traversal
operations. Generated dependencies run through `run_until` without artificial
restoration of their clobbers.

The background descriptor vector observed at `r2-0x5c38` is `0x108bd664`,
with entry `0x1074de10` and TOC `0x108bba90`; its wrapper calls `0x107393f0`.
These remain generated and are fixture observations, not fixed comparator
choices in the semantic search. Loading/releasing/inserting/removing
collections, other accessors, background drawing and the RNG sampler
`0x107124e0` also remain generated. Club/competition null-getter guards at
`0x10713aec` and `0x10713b3c` retain their existing contracts. The 34 Date,
three RNG and four background semantic entries remain selected, including
Date's original ascending-year leap asymmetry.

## Focused validation and limits

The consistent Release ARM64 build passed, rebuilding all 1,922 generated
pages and their dispatcher after the `State` layout extension. The listing
contains 1,953,157 indexed instructions, zero missing/pending static targets
and 46 semantic replacements.

| Executed check | Result |
| --- | --- |
| `modern_collection_lookup_equivalence` | PASS: 263 comparisons; own list/position-wrapper/search-wrapper/preorder/search cases 23/57/53/60/52; 27 faults |
| Own collection boundary controls | PASS: five entry-specific mutations rejected at their own first return; diagnostic reset verified |
| Nonempty production dispatch | PASS: five entries, both recursive helpers, five callbacks and five generated indirect transitions; original-mode counts zero in normal witnesses |
| Semantic background compositions | PASS: eight nonempty picture/descriptor/RNG compositions |
| `original_background_selection` | PASS: original club/competition/default selections, RNG consumption, null-getter guards and independent getter faults |
| `modern_background_selection_equivalence` | PASS: existing 163 comparisons, 18 faults, four own negatives and callback/indirect dispatch |
| `python3 tools/test_collection_lookup_audit.py` | PASS: five contract/hook/manifest/edge/dependency tests, no skipped private-index cases |
| `python3 tools/test_background_selection_audit.py` | PASS: four tests, preserved original block and extension to 46, no skipped private-index cases |
| Focused UBSAN | PASS: all 263 comparisons and five own negatives; empty stderr |

The first focused run passed both existing background groups. The new harness
then exposed one faulty test expectation: position one may select an unmapped
child pointer without dereferencing that node, since its residual position is
zero. The fault fixture now requests position two, forcing the child-link load.
Only the harness target and its failed CTest were rerun; the final check passed
in 3.15 seconds. No production adapter correction or repeated successful
background campaign was required.

UBSAN instruments only `collection_lookup_adapter.cpp` and
`collection_lookup_checks.cpp`, with C++23/ARM64, `-O1`,
`-fsanitize=undefined` and `-fno-sanitize-recover=all`, linked to the consistently
rebuilt engine libraries. It passed the complete new harness with zero
diagnostics. This is neither whole-engine instrumentation nor ASan.

The shared target is `cm-collection-lookup-checks`, registered as
`modern_collection_lookup_equivalence`. Original and semantic runs start from
identical state while Date, RNG and backgrounds remain semantic. Comparison
covers every architectural register and temporary byte, PC, every mapped memory
byte, allocator/import state and interleaved read/write/map/clear effects,
including fault prefixes. Instruction accounting and private diagnostics are
excluded from behavioral equality.

Independent fixture expectations check returned nodes, list link order,
preorder link and decrement order, residual positions, descriptor key order,
comparison count, repeated pointers and RNG cursor movement. The 31-node tree
uses reversed key ordering to distinguish the original negative/positive
branches and extends beyond the four expanded levels. Cases include empty and
singleton trees, a small asymmetric tree, signed32 boundaries with wrapped
decrement, count-inconsistent lists, eight-link boundaries, null/exhausted links,
minimal accessed-byte mappings, relocated TOC and selected position/root/vector/
spill aliases. Controlled null-head faults cover extreme positive indices
without iterating billions of links.

The mutable-comparator fixture runs the actual generated transition wrapper
and string comparison. An explicit adversarial memory observer injects vector,
child and volatile-register mutations during the generated comparator's key
read, equally in both runs. It checks the vector's own TOC, the next vector's
changed TOC, child loading after return and restored caller TOC. This is a
fixture intervention, not a new production callback implementation. Separate
cases cover unmapped descriptor/key/vector dependencies and repeated key/vector
pointers without deduplicating reads.

A separate fixture transition vector targets generated leaf accessor
`0x1077d9e0`, which returns a key's `+4` word. Its keys supply zero, `-7`,
`INT_MIN`, `+19` and `INT_MAX` to distinguish signed branch direction from a
boolean or normalized comparator. The descriptor is deliberately unmapped and
unused by this callback. This fixture does not change the real background
descriptor dependency.

Mixed selections exercise original wrappers with semantic helpers and semantic
wrappers with original helpers. Normal witnesses exercise nonempty production
dispatch of each entry, both recursive helpers, callbacks and generated
indirect transitions, requiring zero original-mode counts. Eight compositions
retain semantic background selectors, real descriptor comparison, nonempty
picture payloads and identical RNG effects. Each entry has an own return-boundary
mutation rejected before any caller can mask it; wrapper controls are separate
from helper controls. Diagnostic reset and rejection of changed, partial and
duplicated original contracts are checked.

Save/Load and persistence are not retested at the user's direction on October
5, 2026. They are neither a PASS nor a required pending gate. No whole suite,
Date/RNG campaign, day/week replay, persisted-career comparison or serialization
audit is inherited from historical reports. Harness dispatch is distinguished
from a live app session. No application session was opened for this delivery;
UI behavior is not claimed as freshly exercised.

Executed commands from the project root:

```sh
python3 tools/test_collection_lookup_audit.py
python3 tools/test_background_selection_audit.py
python3 tools/compile_recomp.py .local/recomp-input .local/recompiled --root 0x10773710 --all-analyzed
cmake -S native -B .local/aot-build -DCMAKE_BUILD_TYPE=Release \
  -DCM_GENERATED_DIR="$PWD/.local/recompiled" -DCM_ORACLE_DIR="$PWD/.local/recomp-input"
cmake --build .local/aot-build -j 4
ctest --test-dir .local/aot-build --output-on-failure -V \
  -R '^(original_background_selection|modern_background_selection_equivalence|modern_collection_lookup_equivalence)$'
# Affected fixture correction only; no regeneration or full rebuild.
cmake --build .local/aot-build --target cm-collection-lookup-checks -j 4
ctest --test-dir .local/aot-build --output-on-failure \
  -R '^modern_collection_lookup_equivalence$'
```

## Protection and delivery

Before rebuilding, the current installed ARM64 app was copied to the new private
`.local/collection-lookup-reconstruction/reference/installed.app`. Its executable
SHA-256 is `c5850fc5cb5ba228fbd30d2251bab27f386e5644f9452473539097d09b42a11a`.
The earlier background reference remains preserved. The new personal baseline
covers 809 Game files, five Runtime files and 45 Runs files, including content,
resource forks, Finder metadata, extended attributes, modes, modification times
and flags. Presentation preference and retained handoff hashes are recorded.
Private fixtures, receipts and diagnostics remain under `.local`; personal
careers are not opened for installation validation.

The existing `tools/install_native.py` ran once on October 5, 2026, from
18:55:18.422 to 18:55:19.058 UTC (15:55 local time). The compiler's linker-signed
output is separately retained as `source-linker.app`. Original program payloads
were included before ad-hoc signing the source bundle; the signed candidate is
retained as `candidate.app`. Source, candidate and installed bundles all pass
`codesign --verify --deep --strict`.

| Artifact | Executable architecture | Executable SHA-256 |
| --- | --- | --- |
| Retained compiler/linker output | ARM64 | `c0b5d4d3fdf5690438d1334d47c0d2699c2d058ca50863500610d18e7484a7ed` |
| Signed build source and private candidate | ARM64 | `bb3ddaf091677f05124a9b82ab9afe4c2c65edb1f849d5d07abe6d6e17ebab8d` |
| Installed app | ARM64 | `bb3ddaf091677f05124a9b82ab9afe4c2c65edb1f849d5d07abe6d6e17ebab8d` |
| New pre-delivery installed reference | ARM64 | `c5850fc5cb5ba228fbd30d2251bab27f386e5644f9452473539097d09b42a11a` |

All four bundled original program files match `.local/recomp-input` in source,
candidate and installed bundles. Installed and source production plists are
identical, without diagnostic environment or validation keys. The postinstall
receipt confirms identical personal Game/Runtime/Runs content and metadata,
all 17 retained prompt hashes, unchanged original segment hashes and the new
reference's full inventory. The prior background reference also matches its
retained full inventory. The production presentation preference was absent
before installation and remains absent; no preference was created by validation.

Only product code, focused checks and technical reports are published. The
twelve untracked handoffs, original payloads, saves, fixtures, logs, screenshots,
snapshots and private references remain excluded; ignore and LFS rules are
unchanged. Publication uses the verified existing `origin` on `main`, followed
by a remote revision check.
