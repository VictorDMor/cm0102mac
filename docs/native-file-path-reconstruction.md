# Native Mac file-path reconstruction

This increment reconstructs seven Mac path/name helpers as named C++23 bodies.
All seven execute by default through the production ARM64 destination hooks. The complete engine/runtime goal
continues beyond this bounded path block.

## Source boundaries and integration

The seven bodies contain 537 indexed instructions on page `0x10778`. Full-index
flow/fallthrough inspection finds zero external interior edges. Complete bodies,
source fields and destination hooks are pinned by `tools/file_path_audit.py`;
missing, changed, duplicated and misplaced bodies/hooks and interior edges have
negative controls, including both generated temporary-storage modes.

| Entry | End exclusive | Instructions | Role | SHA-256 |
|---|---|---:|---|---|
| `0x107784c0` | `0x1077851c` | 23 | CString to Pascal copy | `30af00422f19a7ad460e6d93297f0721daa5bbbbd5890428aa401e66315ff7b6` |
| `0x10778520` | `0x107786f0` | 116 | Classic path to FSSpec | `16f7752ef5e153ed03f05d474565307470b479b8df43de3bd449368f02ea4bd5` |
| `0x107786f0` | `0x10778814` | 73 | Capability-aware path to FSSpec | `737d3ee17981dfc01236bbcb20c60ff293f7d97d8140aab7303a0b1f52ba9beb` |
| `0x10778820` | `0x1077886c` | 19 | Current script encoding | `7ddafbe933efaf3e103605c4e4cb008491a93930b03265f4673c119eeeae9895` |
| `0x10778870` | `0x107788cc` | 23 | Path to FSRef | `26f635b0d672d7190818d92c938bea0f720d8337e91eefc2a90a7f9a6f9c195a` |
| `0x107788d0` | `0x10778a78` | 106 | Text bytes to Unicode name | `627e4d76955cd109acc9670907da6643c651f603df838c5b3b40372959faa4d3` |
| `0x10778a80` | `0x10778d48` | 177 | Parent reference and leaf-name resolution | `7eaea9c34b6c770224d118c6d599c915d8de6692b303bdaf9344deb89f59d57d` |

The generated manifest contains 182 entries. Its first 175 entries preserve the
accepted order/content and SHA-256
`30ae42093b04caa9d10741d91b81a2663c45dfa6543b7fc7481cc7736e35290c`.
The 182-entry digest is
`21eeea757467d5a9e871fc5ac9b6f482f328e7132cb4dd91a4c8cdf0778abd2c`.
Generation retains 1,953,157 instructions, 1,922 pages and zero missing/pending
targets. Hooks precede the first original operation at each destination label,
covering local, cross-page and generated indirect calls.

`file_path_adapter` owns default semantic dispatch, independent global/per-entry
original selection, normal/original counts, escapes, observer boundaries and
reset. These are process-local and never serialized. Production has no fixture
observer and uses the existing native SDK bridge. No own-body replay, opcode
interpreter, host path normalization or replacement guest heap is introduced.
The capability helper, strings/memory, native FSRef/Unicode bridge and generated
heap/formatting glues remain dependencies with their established contracts.

## Preserved contracts

`file_path_names.cpp` owns CString-to-Pascal copying, script-encoding discovery,
path-to-FSRef composition and text-byte-to-Unicode-name conversion.
`file_path_specs.cpp` owns Classic and capability-selected FSSpec construction.
`file_path_resolution.cpp` owns parent-reference/leaf resolution, live colon
scanning and volume-prefix copying. All retain Mac ABI operations through the
shared `MacAbi`, including stack frames, register saves, scratch, CR/CA/CTR,
TOC restoration, caller PC and ordered guest accesses.

CString-to-Pascal copies all `strlen` bytes and publishes the low length byte
after the copy; it neither clamps to 255 nor copies a terminator. Overlap is
observable. Its frame is 80 bytes. Caller validation remains separate.

Classic FSSpec construction uses an 816-byte frame. Null, empty, over-255-byte
paths and empty/over-63-byte leaves return `-37`. It publishes the Pascal leaf
before catalog resolution, preserves the existing-file parent and directory
`-1302` result, and resolves a missing leaf's parent before the final volume
query. Errors retain preceding output stores. The last colon is selected by the
existing string helper; missing-parent truncation and inserted colon are live
writes, including their alias/fault behavior.

Capability-aware FSSpec construction uses a 1,056-byte frame and the live,
previously semantic capability cache. Its Classic branch delegates to the new
Classic body. Its modern branch validates the path, resolves a native reference
and extracts its catalog spec. Only missing-file `-43` invokes parent resolution,
volume/node catalog reads and `FSMakeFSSpec`; other native errors retain their
original return and output prefix.

Script discovery uses a 64-byte frame and `UpgradeScriptInfoToTextEncoding` with
`-1/-128/-128` script/language/region arguments. A nonzero full-word status stores
encoding zero. Path-to-FSRef uses a 656-byte frame, parent/Unicode-leaf resolution,
the signed-word result test, script encoding and `FSMakeFSRefUnicode`.

Unicode-name conversion uses a 112-byte frame and signed-word source length.
It creates a native text-conversion context, requests 510 output bytes with the
original twelve-argument PPC layout, publishes half the returned byte count and
disposes the context. Disposal status does not replace the retained conversion
status. Create/convert failure publishes the requested low-word length and, only
for a positive signed-word count, falls back to sign-extended source bytes in
UTF-16 slots. High bytes become `0xffxx`, preserving the legacy fallback rather
than applying a modern Unicode policy. Partial native output is overwritten by
that fallback in original order. Eight-byte groups and the final one-to-eight
byte tail preserve CTR, scratch and forward live alias effects.

Parent/leaf resolution uses a 160-byte frame and the complete `stmw/lmw` ABI.
It queries the current volume/directory first; a failed query clears those fields
but continues the original resolution policy. Leading-colon paths use the current
parent; absolute volume prefixes become live Pascal names including the colon.
Colon-separated nonterminal components resolve references in place. The trailing
component becomes the Unicode leaf. A trailing colon suppresses traversal of
that last converted component; a volume-only path has an empty final leaf.
Errors stop through the original signed-word status tests. Empty names, doubled
colons and null inputs follow the characterized original behavior; no new path
validation, parent traversal or locale normalization is added.

## Verification and installed acceptance

The final harness passed **277 full-state/effect comparisons**, including
**21 deliberately unmapped access-prefix faults**, and rejected **12 boundary
mutations**. Every architectural register/temporary byte, PC, mapped guest byte,
allocator map/error and native-call count is compared. Ordered accesses and
import ABI/dependency histories must match. Only instruction accounting and
private diagnostics are excluded; no guest ABI values are normalized.

All seven entries have direct and generated-indirect witnesses with both
capability settings. One own negative per entry and five child negatives reject
at the child boundary before its parent returns: Pascal copy before Classic spec,
reference resolution before compatible spec, parent/leaf before FSRef, Unicode
conversion before parent/leaf, and script encoding before Unicode conversion.
Ten selected original-parent/original-child compositions verify independent
selection. Normal unselected paths have zero original calls and zero escapes;
reset preserves the file-controller diagnostics.

The matrix covers null/empty and length guards, absolute/relative/nested paths,
trailing/doubled colons, existing/missing names, unbounded Pascal copying and
length-byte wrap, conversion group/tail boundaries, signed-word/high-word length
cases, full-word script statuses, create/convert/dispose failures, partial output,
forward aliases, relocated live TOCs, minimal mappings and fault prefixes. The
relocated fixture copies only mapped original TOC extents, including the live
CFM import slots; unmapped gaps are not treated as valid source memory.

A component-only import observer supplies deterministic guest API outputs and
explicit service errors; every injected service must actually be called. It is
absent in production. Separately, **54 comparisons use the real native SDK**,
covering both capability branches, existing/missing files, the Classic directory
`-1302` versus modern catalog result, nested and MacRoman names, script discovery and actual MacRoman-to-Unicode conversion. Each pair
uses fresh native platforms with identical history and the same temporary files,
so guest context tickets, FSRef bytes and catalog identities compare exactly
without rewriting returned references.

The real-OS file-reference composition passed **428 independent assertions**,
17 production dispatches and 28 real imported services. It retains read/create/
write/flush/truncate/rename/remove and Classic metadata/close compositions, and
requires nonzero normal counters for all seven path entries with zero original
selection and zero escapes.

| Entry | Normal equivalence calls | Real-OS composition calls |
|---|---:|---:|
| `0x107784c0` | 155 | 4 |
| `0x10778520` | 42 | 1 |
| `0x107786f0` | 38 | 1 |
| `0x10778820` | 256 | 23 |
| `0x10778870` | 55 | 8 |
| `0x107788d0` | 175 | 12 |
| `0x10778a80` | 96 | 12 |

One full regeneration and consistent Release ARM64 rebuild completed all 1,922
generated units after State gained process-local path diagnostics. The full
build took 657.59 seconds; later fixture-only corrections rebuilt only their
harness. Seven focused CTests passed in 21.85 seconds: path and controller
equivalence, native file references, native filesystem/bridges, strings and
memory primitives. All fourteen affected Python audit suites passed (81 tests).
After adding the two native directory cases, only the affected path CTest and
its instrumented harness were repeated (the CTest passed in 2.16 seconds); the other green checks were retained.
UBSAN instrumented all four path implementation units, the modified dispatch
runtime and both affected harnesses, linked against the final consistent Release
libraries. Both completed with empty stderr and the same final coverage counts.

One protected final installation delivered the signed source/candidate/installed/
running ARM64 executable with SHA-256
`ecb56a3ecf166793c88189d5c1ff10af049686c049e21182739220327b28d6da`.
The unsigned linker artifact was
`cff90a4e09dfa0f4810a9b89b2e69e3cf88a7a4f9c0f851e893496451402aab6`.
Strict signatures, normal production plist and all four original payload hashes
passed. The prior installed controller artifact is preserved with forks,
attributes and metadata at SHA-256
`6ca38d4eb7c44ca73f098b758e9a65b6ebd0274b529052321d42b75783943515`.

The installed app rendered Prepare Game, responded to the original named Load
Game command, displayed the original file chooser and returned after Back or
Cancel without loading a career. Original Quit displayed its confirmation;
Return confirmed normal exit. Process 47033 was confirmed closed. A subsequent
UI state query automatically reopened the app; that owned session (48320) was
also closed normally through original Quit/confirmation. Both exit logs contain
zero original path selection/escapes and no fatal diagnostics. The first smoke
records 7,919,239,206 recompiled instructions. Its UI counts cover only encoding,
FSRef, Unicode and parent/leaf helpers; all-seven coverage comes from the harness.

Before installation, Game (809), Runtime (5) and Runs (66) files were protected
with full content/fork/attribute/metadata inventories. Immediate post-install
verification passed. Final post-smoke verification preserves Game, existing
user/workspace saves, all 23 handoffs, original payloads, the installed reference
and 142 older private bundles. Runtime/Runs and presentation preferences may
change during UI; their post-smoke byte identity is not claimed. Logs, receipts,
original material and bundles remain private and unversioned.

Save/Load remains **not retested**. Career/day/match campaigns, performance
benchmarks and the complete historical suite are outside this increment. Menu
smoke establishes only its observed UI response; per-entry evidence comes
from the focused harness and native composition.
