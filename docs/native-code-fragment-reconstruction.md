# Native C++ code-fragment metadata reconstruction

This increment reconstructs five code-fragment registration, lookup and exception
metadata helpers as named C++23 bodies on the ARM64 production path. The complete
engine/runtime modernization goal continues beyond this bounded metadata block.

## Source and integration

The complete bodies contain 301 indexed original instructions on page `0x10771`.
Counts reflect actual instructions rather than spans including padding. Full-index
flow/fallthrough inspection finds zero external interior edges. The audit pins
original fields, bodies and destination hooks, including both generated
instruction-temporary storage modes and missing/duplicate/changed/misplaced
bodies/hooks and external-interior-flow negatives.

| Entry | End exclusive | Instructions | Role | SHA-256 |
|---|---|---:|---|---|
| `0x10771150` | `0x107712a4` | 85 | Register runtime code fragment | `b7a3d90f8ddc93317de754bc24be92b2103149d5e69d8bc469ee5a8e93e4750e` |
| `0x107712b0` | `0x107713d8` | 73 | Find cached code fragment | `0ef9fa3af79ddaf0ae84142bd2ba4ea7ceea9a3ba1eb0718a3ab77f267be1d9d` |
| `0x107713e0` | `0x107715f0` | 126 | Resolve exception metadata | `1dcacf0f3737338010b1505b729eec9c192c086a3f8be73514a432403e6b0e01` |
| `0x107715f0` | `0x10771614` | 9 | Read saved frame word | `dad91c6b4a36c5c12c01480e0a6c172037e6e74446573d207e23d037b56e4666` |
| `0x10771620` | `0x10771640` | 8 | Read exception action kind | `1784828c0cb21e31f11f266d06117a71c891dfcfb7d11f23de0456e865131782` |

The manifest appends five entries to the accepted 188-entry prefix, whose digest
is `71685ebd95d1c0fc78ebeaa5ec1d9da103cc49027e1b75cca68860c93cc976af`.
The 193-entry digest is
`33bf1c60bf7773bc46dd082587c78f6a7566c5f4b9139eadf3b058c52bdbcc50`.
Generation retains 1,953,157 instructions, 1,922 pages and zero missing/pending
targets. Generation passed in 57.83 seconds and the full consistent Release
rebuild passed in 657.27 seconds after State gained private code-fragment
diagnostics; every generated unit was rebuilt with the same layout.

The adapter owns default semantic dispatch, independent global/per-entry original
selection, calls/escapes, observers and reset. These are process-local and never
serialized. Diagnostic original mode uses a one-use entry resume; child selection
remains independent. Production attaches no fixture and uses the existing native
bridges. No reconstructed body is replayed; native CFM thunks and the guest heap
bridge remain explicit dependencies.

## Preserved contracts

Registration uses a 112-byte frame and complete r23..r31 saves/restores. It captures
seven parameters into those registers and probes the optional ppcf capability
only once. The probe-complete byte is written before querying. The original
signed-word status test controls reading the capability result, and only its
0x10 bit sets the optional-capability byte. An unavailable probe still marks
completion. No new host CPU policy replaces this behavior.

The live guest registry contains linked 0x384-byte blocks, each with 32 records
of 28 bytes and a next pointer at 0x380. Code-start zero is the free-slot marker.
Registration scans blocks in order, retaining a cumulative slot index and CTR.
Reused slots publish start/end/data/metadata-table fields in their original order.
When all slots are occupied, the optional native NewPtrClear allocates a zeroed
0x384-byte block. Linking the block precedes populating its first record; that
new-record publication order differs from reused-slot order. Unavailable/failing
allocation returns low-word -1 and preserves preceding probe/list effects.
All addresses, ownership maps and memory errors remain guest/native-bridge state.

Lookup first checks the cached record with unsigned half-open [start,end) bounds,
then scans the live linked blocks in the original groups of four records. The
unrolled scratch increment of three and its CTR effects are retained. Cold lookup
skips code-start zero, while cached lookup may accept a zero start if its cached
bounds include the target. This original asymmetry is preserved. A cold match
publishes the cache pointer before returning; a miss returns zero.

Metadata resolution uses a 64-byte frame and retains the requested PC and output
pointer in r30/r31. It clears only the descriptor/action pointers first, then
looks up the containing fragment. A region hit publishes code/data/TOC fields
before table search. The original signed multiply-high/shift/correction computes
an inclusive high index from the two guest table pointers. Binary search uses
unsigned PC-relative offsets, inclusive function ends, low-word wrap and original
arithmetic-shift CA effects. A missing record leaves unrelated output fields and
all preceding stores intact.

Each 12-byte function record carries a start offset, a length with its high-bit
inline-descriptor tag, and either an inline descriptor at record+8 or an offset
from the fragment's data base. Resolution reads these fields live, so output/table
aliases retain their original effects. The descriptor's flags select one of three
sentinel-terminated action formats: 32-bit starts with a four-times-short length,
extended short ranges starting at descriptor+4, or compact short ranges starting
at descriptor+2. All upper endpoints are inclusive. Action pointers are relative
to the descriptor and published only on a match; zero starts terminate scanning.

The saved-frame-word helper dereferences the descriptor header, applies the
original encoded frame subtraction and optional extra eight bytes, then reads
the preceding guest word. The action-kind helper guards a null action pointer
and otherwise returns the low seven bits of its first byte. No new validation,
exception ABI, array/container policy or corrected legacy bounds is introduced.

Startup/root `0x10773710`, exception execution/unwinding `0x10771640` and later,
nonlocal contexts `0x10773eb0/0x10773f30` and heap glues
`0x10778d50/0x10778d80` remain outside this increment. This block is not the whole
C++ exception runtime. Date's ascending-year leap behavior and the allocator's
productive decreasing-retry guard remain unchanged.

## Validation and delivery

The preliminary unchanged-State component build passed 124 full-state/effect
comparisons and five expected mapping faults against the accepted 188-entry
library. Those calls are component evidence, with generated children; they are
not production-hook coverage. Production checks add direct/CFM entry, own and
lookup-child negatives, independent mixed selection, diagnostic reset, relocated
TOC, sparse fault prefixes, live aliases and a registration-to-metadata sequence
using actual native Gestalt/NewPtrClear services. Every architectural byte, PC,
mapped guest byte, allocator map/error, native-call count, ordered access and
import ABI/history is compared. Only instruction accounting/private diagnostics
are excluded.

The production harness passed **165 comparisons**, including **23 expected
mapping faults** and **six own/child negatives**, in 3.53 seconds with empty
stderr. Five own mutations are rejected at their entry boundaries; the lookup
child mutation is rejected before metadata resolution returns. Mixed selection
works independently for that child and its parent. Direct and generated CFM
dispatch, observer-free original mode and architectural isolation of diagnostic
reset are exercised for every entry.

| Entry | Normal semantic calls in the production harness |
|---|---:|
| `0x10771150` | 49 |
| `0x107712b0` | 72 |
| `0x107713e0` | 23 |
| `0x107715f0` | 9 |
| `0x10771620` | 11 |

These counts exclude fault cases. Default cases require zero original calls and
zero dispatch escapes. The matrix covers all 32 registration holes and lookup
lanes, native empty-head/full-block growth, unavailable/failing allocation,
once-only capability probing, cached/cold range boundaries, all three action
formats, inline descriptors, saved-word headers, action flags, signed empty
tables, live aliases, relocated TOC, minimal mappings and ordered fault prefixes.
The five-stage registration/lookup/resolution/kind/frame composition uses real
native Gestalt and NewPtrClear, compares complete state and cumulative effects
after every stage, and independently checks the new record, allocation, action
pointer, kind and saved word.

Sixteen Python audit suites passed **95 tests** in 40.21 seconds. The three
related CTests `modern_runtime_lifecycle_equivalence`, `native_bridges` and
`original_application_shutdown` passed in 6.34 seconds. The new harness was run
directly; its registered CTest was not repeated. UBSAN instrumented all four new
implementation units, the modified `aot_runtime.cpp` and the harness, linked
against the final consistent Release libraries, and passed in 4.66 seconds.
Its output matches Release exactly and stderr is empty. No sanitizer claim is
made for unchanged library internals.

One protected final installation delivered
`/Users/victordmor/Applications/CM 01-02 Native Prototype.app`. Signed source,
candidate, installed and running ARM64 executable identity is
`cf03e10179523d539f03ff222af074560c82eee58f1cfd84cd18b14acdb832b3`.
The retained unsigned linker output is
`7bdeff74a7690c1fa83b5e00a067dfac9943bc2498890f16c61b4ec474e7a081`;
the previous installed reference is
`fdcd54f2fdb4386a8d9dd2350491c0e15cd071c9964cbae031060e70533be34e`.
Strict signatures, normal production plist, absence of LSEnvironment and all four
original payload identities passed. A fresh process check found no live game
before installation. The running process's open executable path and ARM64 hash
match the installed candidate.

The installed smoke rendered Prepare Game, dispatched named `Quit CM 01/02`
through the original action and displayed the original confirmation. Owned
process 29445 closed normally after Return, and the new private log
`app.aW5ASu/startup.log` records 4,532,133,418 statically recompiled instructions.
Registration ran once semantically with zero original calls/escapes. The other
four entries had zero calls in this UI; their coverage comes from the production
harness. There was no fatal AOT error. The smoke log also contains nonfatal
catalog -120/-43 diagnostics; no clean-catalog claim is made.

Immediately after installation, before launching the smoke, full content/forks,
xattrs, modes, timestamps and flags were unchanged for Game (809 inventory
entries), Runtime (5), Runs (69), save snapshots, 23 handoffs, the installed
reference, original payloads and 148 older private bundles. After smoke, Game,
existing saves, snapshots, payloads, handoffs and retained bundles remain
protected; run logs and preferences may legitimately change. Receipts and
reference/candidate/linker bundles remain private under
`.local/code-fragment-reconstruction`; no original/generated payload or save is
published.

Save/Load: **not retested**. Performance: **not benchmarked**. No career, date,
season or match campaign was added for this bounded runtime increment.
