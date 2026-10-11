# Mac word utilities and comparator binary search

## Scope and current acceptance boundary

This block reconstructs four complete Mac routines in semantic C++23: two
independent absolute-word entry points, signed floor-style remainder, and
comparator-driven binary search. It does not reconstruct a sorting engine or
the adjacent assertion/termination wrapper. Gameplay, the ascending-year leap
quirk, allocator progress policy and original payloads remain outside this
change. Save/Load is **not retested**; no performance benchmark is claimed.

All four entries are delivered in default semantic C++23 production.
Complete generation contains 1,953,334 indexed instructions, 1,922 pages,
zero missing/pending static targets, and **268** semantic replacements. Its
first 264 entries exactly preserve the accepted register-context delivery.
The full expanded-runtime Release ARM64 build, six focused CTests, 21 affected
Python suites / 136 tests with zero skips, twelve-unit UBSAN and one protected
installation with short installed smoke pass. Component checks have 4,095
original/semantic comparisons, 583 exact-accounting cases, 569 exact budget
stops and four individual negatives. Production has 742 comparisons, 585 exact
budget stops, two mapping faults, four individual negatives and eight separate
profiling-selection comparisons. Thirty-two actual nonlocal search comparisons
include sixteen restore/resumed-handler faults and recursive searches.

| Entry | Component comparisons | Component fault/budget outcomes | Own negative | Normal adapter / generated / CFM calls |
|---|---:|---:|---:|---|
| `0x10775540` | 545 | 0 | 1 | 2 / 2 / 2 |
| `0x10775550` | 545 | 0 | 1 | 2 / 2 / 2 |
| `0x10775560` | 1,242 | 43 | 1 | 2 / 14 / 2 |
| `0x10775600` | 1,763 | 712 | 1 | 2 / 4 / 4 |

Component fault/budget totals include the explicitly expected division-zero,
mapping, callback, uncompiled-destination and budget outcomes; they do not
count as normal returns. Production coverage uses dedicated successful
semantic calls with zero original selection, diagnostic continuation or escape.
Separate global/per-entry selection has four checks per entry. Real generated
CFM composition also calls the already-semantic string comparator four times
and returns the original chosen element, without injected comparator results.

## Complete original contracts

Intervals are end-exclusive. Fingerprints serialize address-ordered records
containing `address`, `assembly`, `ops`, `flows`, and `fallthrough`, with sorted
JSON keys and compact separators. The 90 instructions are indexed records;
the gap at `0x107756bc` is not an invented instruction.

| Entry | End | Instructions | Operation | SHA-256 |
|---|---|---:|---|---|
| `0x10775540` | `0x10775550` | 4 | Absolute word | `0f500638018273f5fd6d10f499518290a68a3687b321300079d11100995064a6` |
| `0x10775550` | `0x10775560` | 4 | Absolute long word | `447493b5c44ecbc8cb16adcdc2d5cd38d0c49a243e80d497a3efe3c5cd532b6b` |
| `0x10775560` | `0x107755bc` | 23 | Floor-style word remainder | `e115afb1026c1f9a383f0c46ec48f39b31b40d4b6b1c20e4258bd7b7a28393e0` |
| `0x10775600` | `0x107756f0` | 59 | Comparator binary search | `af1d8266f781790cda7244658ead2f0d29138fc287628157cd56cb3b28857fc9` |

The full flow/fallthrough audit rejects external entrances into an interior.
All roots are destination-label hooks before the first original operation,
so local, generated cross-page, and CFM-indirect calls use the same adapter.
Missing, truncated, duplicated, reordered or changed complete bodies fail
closed. The original-only component reference disables all semantic hooks.
Original records, generated source, indices and payloads remain private.

Manifest array SHA-256, using the same compact sorted-key serialization:
`97d4e7d8d5633d4efc9c771a720a4ddedfa347de67e7600c20ceca91ac20fbec`.
Preserved first-264 array SHA-256:
`6e438c9dbfe73e5e4466f7ecb2375f14cd8e7ff61acc7838047b4dcb8826531c`.

## Word operations and architectural residues

Both absolute entries read r3 and return a word32 magnitude in r3, retaining
the sign mask in r4 and XOR intermediate in r0. INT_MIN remains `0x80000000`.
The arithmetic-shift carry byte is normalized to one only for negative inputs
with nonzero low 31 bits; INT_MIN has zero carry. CR, floating status and
unrelated register/temporary bytes are preserved, and return uses the live LR.

Remainder records signs in r6/r7, forms word32 operand magnitudes, uses signed
division and preserves the original scratch multiplications, CR and early
returns. For ordinary signed inputs its nonzero result has the denominator's
sign: `-7 / 3` leaves remainder 2, rather than truncating remainder -1. The
word32 INT_MIN magnitude and multiplication behavior is retained; this is not
a host `%` substitution or an overflow correction. Division by zero stops at
`0x10775588` with the complete preceding register/condition prefix. Successful
and division-fault instruction accounting is exact in production.

## Search contract and callbacks

The ABI supplies key/base/count/width/comparator descriptor in r3..r7. The
routine saves r24..r31 and LR, creates a 96-byte frame, then applies the five
nullable guards. Even a null argument performs the original frame accesses.
The key guard branches on the live condition field after frame publication;
observers may change it. The multiple-register save/restore address cursor
is live across each access, including observer mutations and fault prefixes.

Search calls the comparator on element zero first. That first call retains
the live r3/r4 arguments rather than restoring the saved key/base. Equal
returns the saved r28 address; negative returns zero; positive starts the
closed interval `[1,count-1]`. Later probes form a word32 sum of lower/upper,
shift it right, multiply by the live width, and add the live base. A safe
host midpoint formula would change overflow behavior and is not used.
Equal returns r31; signed comparator results update the corresponding bound.
Duplicate entries therefore retain the original chosen midpoint, and do not
promise the first occurrence.

Each call runs the real generated CFM descriptor stub at `0x10781320`, loading
the live function and TOC, saving caller TOC at frame+20, publishing CTR and
retaining its register/access effects. Search reloads caller TOC after each
callback. Comparator side effects, changed saved registers, table/descriptor
mutations and faults remain observable; ordinary production never replaces
the comparator with a host predicate. The epilogue restores the original
saved words and the live saved LR through the original access sequence.

`MacAbi` owns the semantic search frame through `GuestNativeScope`. A scalar
context transfer thrown by a callback must abandon interrupted search and
callback continuations and resume the recorded live dispatcher owner. A
recursive search must also abandon its nested frame. Restore faults and
faults in the resumed handler are distinct outcomes in the focused harness.

## Accounting and diagnostics

Absolute entries charge four steps. Remainder uses an exact sign/branch charge
and a checked division step, selecting original execution when the full
23-instruction bound does not fit. Search charges exact semantic phases around
guest calls. If a later phase cannot fit after a callback, it resumes the
original at that phase's destination rather than approximating a partial
architectural prefix. Nonprogress and malformed states remain bounded by the
original instruction budget; exhaustion is a diagnostic outcome, not success.

Global/per-entry original selection, profiling selection, initial budget
selection and later search continuations have separate counters. Normal
returns, context transfers, faults and original-continuation returns are also
separate. A normal return cannot count as semantic coverage after a diagnostic
continuation. Reset affects only this module's private, process-local State
diagnostics. None of these fields enter guest buffers or save files.

Complete architectural bytes, generated temporary residues, PC, mappings,
guest bytes, heap/import state, ordered accesses and comparator import
boundaries are compared without normalization. Instruction accounting and
private diagnostics are excluded from raw architectural snapshots; successful
and finite-budget production execution adds explicit accounting assertions.
Precharged search phases do not claim original instruction counts at a
mapping fault inside that phase. The fault's architectural/access prefix is
still compared exactly.

## Focused validation targets

`stdlib_core_component_equivalence` compares an isolated original reference
with all four components, including signed extremes, carry/condition poison,
small and random remainder cases, five null guards, duplicate/first/missing
searches, multiple widths and counts, comparator mutations/faults, minimal
maps, unaligned addresses, removed accessed bytes, word32 wrap and finite
budgets. Every entry has its own result negative.

`modern_stdlib_core_production` checks direct adapter, generated and CFM root
dispatch; independent selection/reset; own boundary negatives; exact normal
and budget accounting; variable callback cost; and distinct diagnostic paths.
`original_stdlib_core_profile` checks original-only profiling selection.
`modern_stdlib_core_transfer` checks actual captures/restores through original,
semantic and mixed register-context modes, abandoned callbacks/searches,
recursive search, restore faults and resumed-handler faults.

The directly affected manifest audit suites preserve old module order/slices
and verify the appended four roots. UBSAN instruments both affected generated
pages, the semantic core/adapter, original reference, three harnesses, dispatcher,
host context-transfer code and register-context implementation/adapter. Its
component, integration, profile and transfer outputs match Release exactly;
all four runs have empty stderr. Earlier fixture-expectation and counter-name
compile attempts are retained privately and excluded from acceptance.

## Protected installed artifact

The accepted expanded-layout libraries are newly compiled together. The legacy
build-source bundle and old installed app are preserved with forks, xattrs and
metadata before source promotion; no old-layout libraries are relinked into the
new executable. Linker, signed candidate, source and installed identities are
recorded separately. The installed/source/candidate ARM64 executable SHA-256 is
`1f08d21e93d6c1be300a244a3a081db4e91eed7d32cf794425a0fdcfcd484bde`.
Strict code-signature and normal production plist checks pass. Four original
bundle payloads remain byte-identical to the fixed installation inputs.

Before smoke, full content/metadata checks preserve 809 Game files, five Runtime
files, 75 existing run files, the save snapshot, 23 handoffs and 170 historical
private bundles. Post-smoke checks preserve Game, saves, payloads, handoffs and
references. Private runtime preferences and new run logs may change; Runtime
and Runs are not claimed byte-identical after smoke.

Owned installed PID 89203 rendered the start screen, accepted named Load Game,
opened the original chooser, returned to the start screen through named Back or
Cancel, and exited through original Quit/confirmation. Its log records a normal
exit after 33,346,227,472 recompiled instructions. This count describes the
observed UI session and is not a performance measurement. The initial coordinate
cancel did not return from the chooser; the named original command did.

Installed UI counters show seven semantic absolute-long-word calls/returns and
63 semantic binary-search calls/returns, with zero original/profile/budget
selection, continuation, transfer, fault or dispatch escape. Absolute-word and
floor-remainder UI counts are zero; their individual evidence is the harness.
The existing scalar-context subsystem records 337 captures and one restoration,
with zero fault, original selection, escape or unknown transfer. Installed smoke
does not substitute for individual harness coverage. Save/Load remains
**not retested**; gameplay, benchmarks and whole-port completion are not claimed.
