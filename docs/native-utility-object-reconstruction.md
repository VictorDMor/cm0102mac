# Basic utility-object lifetime reconstruction

## Production scope

Eleven complete Mac bodies now execute named C++23 operations for owned byte
nodes, linked lists, buffers and descriptor-object lifetimes. These operations
retain guest word32 addresses, big-endian fields and actual allocator, copy,
reporting and CFM dependencies. No original body of a replaced entry is replayed
by the semantic implementation. The tree algorithm at `0x1074ea30`, aggregate
constructor `0x1073c1b0`, and other collection methods remain separate generated
dependencies; this delivery does not claim their reconstruction.

Complete generation appends eleven `utility_object_adapter` records after the
accepted 559 records, giving **570** semantic replacements. The first 559 records
have canonical digest `2d2d5079bbf41b2456c5d6c68aa30c2a4f1207d739c6c659ec1b3f2b88c9fc25`
and remain identical. The full digest is
`7c46bda5a1297c19d312785558d5b5dbec2beff2c1c06fd9e11a1aa5cbbb7256`.
All 1,953,334 instructions, 1,922 pages and five accepted source fixups remain;
there are zero missing or pending targets. The full index rejects exterior
entrances into any replacement's interior. Destination hooks precede original
operations and cover local, cross-page and CFM dispatch.

## Complete boundaries

Intervals are end-exclusive. Fingerprints use the established ordered address,
assembly, ops, flows and fallthrough serialization. Unindexed `0x10738cbc` is
padding skipped by the original clear-list branch, not an invented instruction.

| Entry | End | Instructions | Operation | SHA-256 |
|---|---|---:|---|---|
| `0x10738b20` | `0x10738b9c` | 31 | Byte-node construction | `5dd0cef3ad945f186baf35781eda3938de4eca589dbf5f2d15db8f24002c051d` |
| `0x10738ba0` | `0x10738bf4` | 21 | Byte-node deleting destruction | `33c8158664065066acd738d7a97314a5d946a75e24951795f9f20529bf757da8` |
| `0x10738c00` | `0x10738c18` | 6 | Linked-list initialization | `eba97f101d2fb53ed56cdc1cefe2b75d61a63e68d35a63bba9e2842dcad236f1` |
| `0x10738c20` | `0x10738c94` | 29 | Linked-list deleting destruction | `625be6bc6372f8aaa3141a3bca6f315c465b8f2e49ed70d2cd0babb772dccd07` |
| `0x10738ca0` | `0x10738d08` | 25 | Linked-list clear | `1b2b683af072ce940f2f7bacf7ad0d839ce373a50fe5cd751083e14888aac7f9` |
| `0x1073ae10` | `0x1073aed4` | 49 | Owned-buffer construction | `b2702d43aed9a18e50bc90774ffb5290b8d6455ce67a38cf31454d10d34d3d36` |
| `0x1073aee0` | `0x1073af44` | 25 | Owned-buffer deleting destruction | `1b5071a5eb5ef2576764275b9a3ea88cbdaeede4ad6ef8028c03e851294c056b` |
| `0x1074dec0` | `0x1074ded8` | 6 | Descriptor-object construction | `c0570316ee9b1ea590fe1f71a8794a7d11b0d356818b129a071bc57ae0454379` |
| `0x1074dee0` | `0x1074df30` | 20 | Descriptor-object deleting destruction | `a9fee06e559a8f30375fec0145b22f6edc8642218df6911d36de07b34a5c2a45` |
| `0x1074dfc0` | `0x1074dffc` | 15 | Descriptor-object reset | `3d4f60c987c3bf6977fc10ae53c57f923d6d6b8303bbcd58d3ec4f3b9fffb317` |
| `0x1074e000` | `0x1074e030` | 12 | Descriptor cleanup facade | `f57b14c28954b5cf7ad14c19916bc37e957245b3bd8013bb95e07c22bbb22cf4` |

Total: **11 entries / 239 indexed instructions**, across four pages. Missing,
changed, reordered, duplicated or entirely absent bodies and misplaced hooks
have individual audit negatives. Private original records and generated sources
stay local.

## Preserved contracts

Node construction clears its two links, sign-extends the low 16 bits of length
for the existing array allocator and copy routine, stores the original low
halfword length, and returns the owner. List initialization clears three words
and its leading halfword while retaining the adjacent two padding bytes. List
destruction/clear reads the live head, caches the next node before destruction,
and republishes the head after each node. Clear resets all list fields;
destruction preserves the original remaining field residues.

Buffer construction stores full word32 capacity, requests storage through the
actual array facade and preserves the allocation-failure reporting chain and
arguments. Destruction has the original payload-null guard and clears the root
word before testing the live deleting condition. All four deleting destructors
use the original signed low-halfword `flag > 0` test, including high-word residue
and negative low-halfword cases. Node payload deletion retains its distinct
unconditional facade call.

Descriptor construction stores two live vectors and clears root/count. Reset
calls the existing tree cleanup through the facade and clears count, while
retaining the original root word even after cleanup. The facade passes the live
root, callback vector and deleting argument to the existing tree algorithm.
Production fixtures exercise an actual allocated three-node tree and actual CFM
scalar deletion rather than substituting a cleanup implementation.

Caller frames, saves/restores, SP, TOC, LR, CR, XER, GPR/FPR bytes, temporary
residue, heap metadata, mapped bytes, fault PCs, ordered access prefixes and
finite budget stops are preserved. Named lifetime operations retain checkpoints
and live reads rather than interpreting opcodes. Nine framed bodies and two
leaf bodies participate in known nonlocal transfers; abandoned host callbacks
and epilogues never continue. Application exit is a separate nonstandard outcome
and unwinds original-selection guards with balanced boundaries.

## Owned selection and diagnostics

Opaque `RuntimeDiagnostics` now also owns utility selection/counters. Its
extension keeps the State header and layout unchanged; the production Release
build compiled all **2,027** State-dependent objects consistently. Const queries
allocate nothing, States remain independent, and utility reset preserves the
accepted initializer diagnostics. Explicit global/per-entry original selection
and PC profiling retain their own counters without recursive interception.
`CM_AOT_UTILITY_OBJECT_ORIGINAL` selects original bodies in the development host;
normal production uses semantic bodies.

## Focused acceptance

Nine CTests pass: four utility checks and the directly related initializer,
C++ object-storage and allocator checks. All 26 affected/new Python suites pass
**174 unittest tests with zero final skips**.

The component harness compares **991** cases, including **843** faults,
**399** exact budget stops and eleven dedicated ABI negatives. Its private
239-instruction reference runs with semantic hooks disabled. Controlled
allocation/copy/reporting/tree dependencies expose exact arguments and effects;
large signed copies stop at the fixture's explicit bound. The matrix includes
empty/null cases, signed flags and lengths, live SP/TOC/CR changes, owner/source/
stack/TOC aliasing, minimal mappings and individual accessed-byte holes.

Actual production fixtures use identical original prerequisites on independent
private native platforms and actual guest allocation, copy and CFM deletion.
They pass **66** direct/generated/CFM/global/per-entry/negative comparisons,
eleven original-profile comparisons, and an interrupted application-exit prefix
check. All eleven entries have dedicated default semantic dispatch evidence.
Another **79** known nonlocal comparisons include **44** restore/handler faults,
nine frames, two leaf continuations, mixed scalar-context selection and Turbo.
Original dependency accounting is exact; mixed scalar-context cases compare
utility accounting outside the explicitly measured scalar dependency.

Seven affected units are instrumented with UBSAN. Component, production,
profile and transfer runs have empty sanitizer stderr and exactly match their
corresponding Release output. Existing dependencies remain linked from the
validated production library; this is bounded affected-code sanitization.

## Installed delivery and retention

The signed ARM64 candidate, installed bundle and running executable share
SHA-256 `4b2fd9c202ef2293475e4f37a45b81ec0887e01787f5e4165b4490b22b0bc058`. Strict signature and all four bundled payload
checks pass. The installed menu/chooser/cancel/normal-exit smoke records **388**
semantic utility calls and returns, with zero original/profile calls, faults,
budget stops, transfers, terminations or escapes. Ten entries occur in the smoke;
list clear has zero smoke calls and is proved by the dedicated production checks.
No saved game is loaded or deleted. The owned process exits normally.

Original game files, saves, payloads, all 23 handoffs, reference metadata and
194 prior application bundles remain protected. Intentional run logs/preferences
are allowed to change. The previously delivered scaled-content window behavior
is retained. Save/Load is **not retested** and performance is **not benchmarked**;
the complete engine/runtime reconstruction goal remains active.

Reproducible objects, old test executables and static build archives are cleaned
at the end of each accepted goal cycle. The latest validated build, active phase,
original inputs, reference source, receipts and historical apps are retained.
