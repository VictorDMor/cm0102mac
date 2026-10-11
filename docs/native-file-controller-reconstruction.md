# Native Mac file-controller reconstruction

Twenty-six Mac file controllers now execute named C++23 bodies by default on
ARM64. The [native FSRef/Unicode services](native-file-references.md), the existing
allocator and stream-storage/item-I/O blocks remain their separately owned
dependencies. This increment does not complete the engine or the complete port.

## Scope and source boundaries

The exact table, counts and SHA-256 pins live in
`tools/file_controller_audit.py`: 26 entries and 1,421 indexed instructions.
The open backend crosses pages `0x10776` and `0x10777`; the whole 231-instruction
body and its 152/79-instruction fragments are pinned. The complete source-index
audit finds zero external flows/fallthroughs into the interiors. Missing whole
bodies/fragments, changed instructions, new interior edges and missing/duplicate
or misplaced destination hooks have discriminating audit negatives. Both local
and persistent generated-temporary layouts are audited.

The ordered production manifest contains **175** entries. Its first 149 entries
are identical to the accepted manifest, with serialized-array SHA-256
`6a9f68dbb88b2fd876157371d39098f2e9746aff781342ddfb5442139b98f9cb`.
The 175-entry digest is
`30ae42093b04caa9d10741d91b81a2663c45dfa6543b7fc7481cc7736e35290c`.
The generated program still contains 1,953,157 instructions on 1,922 pages,
with no missing/pending targets. The generator hooks each destination label
before the first original operation, covering local goto, page and indirect
transfers. Dependencies are not counted as newly reconstructed entries.

## Semantic contracts

`file_controller_state.cpp` owns mode parsing, the live capability cache,
signed-word record traversal, alias policy, live creator/type overrides,
tell/get-position/seek/set-position and EOF/error/stdio initialization.
`file_controller_streams.cpp` owns filename facades, fopen/reopen, Classic Finder
metadata and shared read/write request construction/completion.
`file_controller_paths.cpp` owns Classic/FSRef open/create/truncate/remove/rename.
`file_controller_forks.cpp` owns signed-64-bit/Classic positioning and temporary
record close/delete/unlink/disposal.

These are named operations and ordinary branches/loops, without an instruction
interpreter, own-body replay, host FILE substitution or a replacement host heap.
`MacAbi` preserves the Mac frames, saves/restores, architectural scratch/condition
results, caller PC and live TOC loads alongside ordered guest accesses. Imports
continue through the real native SDK bridge in production. Unknown transfers and
original diagnostic selection have separate process-local counts.

Mode parsing preserves stores before validation, source/output alias effects,
legacy prefix acceptance and retained unrelated bits. Append positioning tests
permission bit 5, after the original rotation to value 4; it does not accidentally
use retained bit 2 from poisoned stack storage. Reopen initializes stdio, closes
and clears the descriptor, parses and initializes storage before opening; failure
clears class and releases owned storage in original order. Append retains the
original return policy even if the positioning callback rejects.

Capability discovery and caching retain byte values and the native `0x1302`
production result. Classic and FSRef branches retain their permission/metadata,
missing-read/create/truncate and error behavior. Read publishes actual count,
accepts positive short EOF reads, and returns callback status 2 on empty EOF;
write preserves its separate error policy. Pending requests yield through the
generated CFM function vector until the signed result becomes nonpositive,
restoring TOC after callbacks. Signed-64-bit end-relative seeks preserve carry,
borrow, sign rejection, extension and low-word publication. Classic seeks retain
the original signed-word behavior.

Temporary records remain guest allocations. Close deletes the recorded file only
after a successful native close, but unlinks and releases bookkeeping records
even on close/delete failure. Head, middle and tail links and allocator maps are
compared; the host bridge's retained mappings are not hidden or normalized.

## Equivalence and independent evidence

The final harness passed **607 full-state/effect comparisons**, including
**111 deliberately unmapped access-prefix faults**, and rejected **31 boundary
mutations**. Every architectural register/temporary byte, PC, mapped guest byte,
allocator map/error, native-call count and ordered memory effect is compared.
Import boundary records include registers, PC, heap state and mappings;
callback arguments and file-dependency history are compared separately.
Only instruction accounting and private diagnostics are excluded.

All 26 entries have direct and generated indirect fixtures for both capability
branches. One own negative per entry and five child negatives reject at their
own boundaries before the parent can return: mode before fopen, open before
reopen, capability before read, tell before relative seek, and remove backend
before its facade. Ten selected parent/child original-mode compositions verify
independent per-entry selection. Diagnostic reset preserves other modules.

The matrix covers valid/invalid mode prefixes and aliases, cached and real native
capability discovery, metadata overrides, signed-reference lookup, descriptor
classes/states, buffered and callback positioning, sticky errors, null/rejected
reopen, existing/missing files, read/write/append modes, creation/truncation,
open/truncate errors, remove/rename rejection, pending completion with a volatile
callback result, record unlink/disposal failures and minimal mappings/fault prefixes.

A component-only dependency model supplies explicit stable guest API outputs and
failure/pending states; it does not execute original instruction records. That
observer is explicitly attached by the harness and is absent in production.
Separate comparisons use **82 actual owned native forks** across Classic/modern,
synchronous/asynchronous read/write/seek, positive and empty EOF, invalid origins,
extension and six sparse end-relative seeks around 4 GiB. Each original/semantic
pair uses exactly the same SDK reference and restores size/position through the
same SDK. Host reference numbers and guest state are never normalized.

The related native-reference harness passed **420 independent assertions**,
17 production routine dispatches and 28 real imported services. It observes
normal semantic counts for 18 controller entries, zero original controller
selection and zero escapes while reading, creating, writing/flushing, truncating,
renaming/removing and opening MacRoman filenames through the real native bridge,
including Classic creation/metadata/close. This evidence is separate from the
component model and from menu smoke.

Normal semantic calls recorded by the component harness follow. Fault-only,
original-selection and escape calls do not establish normal witnesses.

| Entry | Operation | Normal semantic calls |
|---|---|---:|
| `0x10776690` | Remove facade | 9 |
| `0x107766c0` | Rename facade | 8 |
| `0x107768f0` | Open stream facade | 7 |
| `0x10776940` | Reopen controller | 45 |
| `0x10776a40` | Mode parser | 71 |
| `0x10776bc0` | Fork-capability cache | 279 |
| `0x10776c30` | Classic Finder setter | 10 |
| `0x10776cd0` | Temporary-record lookup | 35 |
| `0x10776d10` | Alias policy | 36 |
| `0x10776d20` | Creator selector | 29 |
| `0x10776d60` | Type selector | 29 |
| `0x10776da0` | Open backend | 49 |
| `0x10777140` | Read callback | 25 |
| `0x107772e0` | Write callback | 20 |
| `0x10777430` | Seek callback | 70 |
| `0x107777d0` | Close callback | 24 |
| `0x10777990` | Remove backend | 17 |
| `0x10777a60` | Rename backend | 16 |
| `0x10777c40` | Tell body | 85 |
| `0x10777cc0` | Tell facade | 7 |
| `0x10777ce0` | Get position | 7 |
| `0x10777d20` | Seek body | 133 |
| `0x10777f10` | Seek facade | 16 |
| `0x10777f30` | Set position | 7 |
| `0x107784a0` | Clear EOF/error | 51 |
| `0x107784b0` | Stdio initialization | 52 |

## Focused verification

A consistent Release ARM64 regeneration/build rebuilt every generated and native
object after State gained process-local diagnostics. Seven related CTests passed
in 14.79 seconds: controller equivalence, native file references, native fork I/O,
native filesystem, native bridges, stream item I/O and stream buffering. After
adding the four real empty-EOF fixtures, the affected controller CTest passed
again in 4.64 seconds; unchanged green regressions were not repeated.
All thirteen affected Python audit suites passed. UBSAN instrumented all five
controller implementation units, the modified dispatch runtime and both affected
harnesses, linked with the final consistent Release libraries. Both binaries
passed with empty stderr; the final controller harness includes all 607 cases.

## Installed acceptance

One protected final installation delivered the signed source/candidate/installed/
running ARM64 executable with SHA-256
`6ca38d4eb7c44ca73f098b758e9a65b6ebd0274b529052321d42b75783943515`.
The unsigned linker artifact was
`1dc13158273227d46b3f9891455a969fa7b193976fb10dc3f908069f77cee776`.
Strict signatures, normal production plist and all four original payload hashes
passed. The prior installed app is preserved with forks, attributes and metadata
at the accepted FSRef/Unicode SHA-256
`b3c376d2cb730bef9173890da3f5dc5d73d996107539269fc379611984103c88`.

The installed app rendered Prepare Game, responded to the original named Load
Game command, displayed the existing file chooser and returned after Back or
Cancel without loading a career. The original Quit command showed its in-game
confirmation; Return confirmed normal exit. Process 1655 was confirmed closed,
with no fatal diagnostics and zero controller original selection/escapes.
The exit log records 28,902,749,561 recompiled instructions. Menu evidence is not
used to claim execution of all 26 entries.

Before installation, Game (809), Runtime (5) and Runs (65) files were protected
with full content/fork/attribute/metadata inventories. Immediate post-install
verification passed. Post-smoke verification preserved Game, existing saves,
the workspace save snapshot, all 23 handoffs, original payloads, the installed
reference and all 139 earlier private bundles. Runtime/Runs and presentation
preferences may legitimately change during UI; their post-smoke byte identity is
not claimed. Private logs, receipts, original material and bundles are not
versioned.

Save/Load remains **not retested**. No career, day, match or performance benchmark
belongs to this acceptance. Startup/menu smoke establishes only the observed
application behavior; per-entry evidence comes from the harnesses. Remaining
engine and runtime routines continue under the complete-port goal.
