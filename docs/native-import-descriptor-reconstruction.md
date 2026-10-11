# Mac import descriptors and CFM tail calls

## Scope

This block replaces all 259 Mac import stubs and the generic CFM descriptor tail
with named semantic C++23 operations. Import entries form the complete grid
`0x1077fad8 + 24 * n`, for `0 <= n < 259`; the generic entry is `0x10781320`.
The import bodies contain six indexed instructions each and the generic body
contains five, for 260 entries / 1,559 instructions on three pages. Ten import
stubs lack a named glue symbol in the private export and are still included.

The new manifest appends all 260 entries after the accepted 276, without
reordering or changing the earlier records. Complete generation contains
1,953,334 instructions, 1,922 pages, zero missing/pending targets and **536**
semantic replacements. Its canonical sorted compact JSON SHA-256 is
`c4f5be3bde9d551349be581de51abab54a2c88d8f7a4ed6375f75b490aa2e2c4`;
the preserved first-276 digest is
`ba418ebeea27f3e546fe4f02aed4ddaa192bbcafa009d0c8552411a739b4ebb2`.

This reconstructs descriptor resolution and tail transfer. Native services,
CFM metadata registration, callbacks, exception ownership, gameplay, allocator
policy, Date's ascending-year leap behavior and persistence keep their existing
implementations. The complete C++23 engine/runtime objective remains active.

## Ordered live descriptor semantics

Each import reads its signed displacement from the caller's live r2 TOC to
resolve a descriptor pointer into r12, saves the live caller TOC at live r1+20,
reads the function word into r0, reads descriptor+4 into r2, publishes r0 to CTR
and tails to live CTR. The 259 ABI displacements are distinct four-byte slots
covering -32768 through -31736, inclusive; slot order follows the pinned import
contracts in `tools/import_descriptor_audit.py`. Descriptor addresses, function
words and callee TOCs remain mutable guest memory, rather than host constants.

The generic CFM tail starts with the descriptor already in r12. It reads the
function before saving caller TOC, then publishes CTR before reading callee TOC.
Both differences are observable when the descriptor aliases the saved TOC or
when access observers mutate registers. The implementation preserves these
orders and performs each subsequent access from live architectural state.

Pointer arithmetic wraps as word32. Guest words remain big-endian and unaligned
accesses retain existing memory semantics. Null, unsupported and interior
function destinations are not validated or corrected by the glue: a completed
tail only establishes the next PC. The caller dispatcher subsequently executes
the guest destination or native service, or reports its own fault. The adapter
does not run the destination internally or count its success as a glue return.

These leaf tails allocate no frame, change no LR and create no GuestNativeScope
or callback owner. The existing dispatcher retains ownership of downstream
scalar context capture/restoration; a nonlocal transfer abandons the same host
continuations as before. Unrelated GPR/FPR, CR/XER, FPSCR, register-extension bytes,
generated scratch, heap state and native-call accounting remain untouched by
the tail itself, aside from explicitly observed memory mutations.

## Contracts, hooks and diagnostics

Every complete body is individually fingerprinted over address-ordered address,
assembly, ops, flows and fallthrough records, using sorted compact JSON. The
full index is also checked for external flows/fallthrough into body interiors.
Missing complete bodies and changed, truncated, duplicate or reordered records
fail closed. Root hooks precede the first original operation at destination
labels, covering local goto, cross-page dispatch and indirect entry.

Only `0x10780ff0..0x10781008` crosses a page: four indexed instructions on
`0x10780` and two on `0x10781`. Both fragments have independent pins in the audit,
and the full six-instruction body has its own pin. The isolated private original
reference emits every one of the 1,559 labels with all semantic hooks disabled.
Original listings, generated source, program payloads and instruction indices
remain private.

The semantic core uses explicit access checkpoints. Successful tails, memory
faults and budget stops preserve exact original instruction accounting and PC.
Global/individual original selection, profile selection and insufficient-entry
budget selection have separate process-local counters. Guarded original
execution stops at the tail boundary, including a destination inside its own
body; the crossing body executes exactly its two generated page fragments.
Normal semantic execution never replays its own original body.

Calls, tails, memory faults, dynamic budget stops, context transfers and missing
original dispatch are counted separately for every entry. Reset clears only
this block's diagnostics, selections and observer. None is serialized or added
to production bundle payloads. Profile-mode tests establish diagnostic selection;
there is no claim of original-PC sample coverage on these pages.

## Acceptance evidence

Nine focused CTests pass: component equivalence, production integration, profile
selection, time-runtime production, time-runtime nonlocal transfer, comparator
search nonlocal transfer, register-context nonlocal transfer, CFM metadata
production and the existing sixteen-case allocator replenishment regression.
All 1,922 generated units and the native core/UI/audio/probe/app were rebuilt in
one fresh consistent Release ARM64 tree after the State layout extension.

The 23 affected Python suites pass 153 tests with zero skips. Three incidental
initial invocations of older unrelated audit scripts omitted their required
index argument and ran no tests; those invocation errors are excluded from
acceptance. Nine UBSAN units instrument the new core, adapter, harness, original reference,
three changed generated pages, shared dispatcher and context-transfer support.
Component, production and profile runs have zero exits, empty stderr and output
identical to the final consistent Release executable. The instrumented harness
was refreshed after its final incoming-CFM, access-snapshot and interior-tail
additions. Final review removed a redundant original continuation from the
generated-path test driver; both selected-original and semantic cases now prove
that an interior destination, including the crossing page boundary, is left for
the caller dispatcher. Only the three affected descriptor CTests and their
UBSAN/Release comparisons were repeated; the six unaffected regressions retain
their accepted results.

Component acceptance has **11,434** original/semantic comparisons, all with
exact instruction accounting including faults. The 1,819 budget stops and
5,975 combined mapping/budget outcomes are expected interruptions, not normal
success. Each entry has one own negative. Independent assertions cover function/
CTR/PC, caller TOC saves, live callee TOCs, unaligned words, signed slot offsets,
word32 address wraps and the different alias/publication orders of import and
generic tails. Every accessed byte is removed individually from otherwise
minimal guest mappings, preserving fault prefixes. Register images and original
PC are compared at every successful memory access, in addition to full final
architecture, mappings, ordered effects, heap state and native-call accounting.

Production acceptance has **8,580** exact comparisons and one own boundary
negative for each of the 260 entries. Every entry has successful direct and
generated semantic calls and separate native/guest destination compositions.
All 259 imports also have a real generic-CFM → import-stub → controlled-native
composition with two independently counted semantic tails. Global/individual
original selection, unrelated selection, generated interior-tail destinations,
generated mapping faults, finite and
dynamically reduced budgets, complete independent reset and 260 separate
profile selections pass. The 520 downstream native-service/uncompiled-target
faults occur after completed tails and are not recorded as glue faults.

The public harness reports an individual row for every entry and rejects any
missing positive path or negative. Its `normal` field counts successful
comparisons, including explicitly selected diagnostic cases; semantic coverage
is established separately by direct/generated/composed/indirect counters and
zero original/profile/budget selection in those successful normal paths.

| Entry | Component comparisons / expected fault-budget outcomes | Own negatives: component / production | Successful semantic direct / generated / native-or-guest composed / incoming CFM |
|---|---:|---:|---:|
| `0x1077fad8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077faf0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fb08` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fb20` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fb38` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fb50` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fb68` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fb80` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fb98` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fbb0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fbc8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fbe0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fbf8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fc10` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fc28` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fc40` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fc58` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fc70` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fc88` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fca0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fcb8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fcd0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fce8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fd00` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fd18` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fd30` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fd48` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fd60` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fd78` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fd90` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fda8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fdc0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fdd8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fdf0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fe08` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fe20` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fe38` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fe50` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fe68` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fe80` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fe98` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077feb0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fec8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fee0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077fef8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077ff10` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077ff28` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077ff40` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077ff58` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077ff70` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077ff88` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077ffa0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077ffb8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077ffd0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x1077ffe8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780000` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780018` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780030` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780048` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780060` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780078` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780090` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107800a8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107800c0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107800d8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107800f0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780108` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780120` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780138` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780150` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780168` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780180` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780198` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107801b0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107801c8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107801e0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107801f8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780210` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780228` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780240` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780258` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780270` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780288` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107802a0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107802b8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107802d0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107802e8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780300` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780318` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780330` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780348` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780360` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780378` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780390` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107803a8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107803c0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107803d8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107803f0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780408` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780420` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780438` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780450` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780468` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780480` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780498` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107804b0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107804c8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107804e0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107804f8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780510` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780528` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780540` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780558` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780570` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780588` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107805a0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107805b8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107805d0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107805e8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780600` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780618` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780630` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780648` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780660` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780678` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780690` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107806a8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107806c0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107806d8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107806f0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780708` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780720` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780738` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780750` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780768` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780780` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780798` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107807b0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107807c8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107807e0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107807f8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780810` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780828` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780840` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780858` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780870` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780888` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107808a0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107808b8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107808d0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107808e8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780900` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780918` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780930` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780948` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780960` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780978` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780990` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107809a8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107809c0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107809d8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107809f0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780a08` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780a20` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780a38` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780a50` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780a68` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780a80` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780a98` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780ab0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780ac8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780ae0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780af8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780b10` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780b28` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780b40` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780b58` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780b70` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780b88` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780ba0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780bb8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780bd0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780be8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780c00` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780c18` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780c30` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780c48` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780c60` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780c78` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780c90` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780ca8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780cc0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780cd8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780cf0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780d08` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780d20` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780d38` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780d50` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780d68` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780d80` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780d98` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780db0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780dc8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780de0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780df8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780e10` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780e28` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780e40` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780e58` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780e70` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780e88` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780ea0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780eb8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780ed0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780ee8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780f00` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780f18` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780f30` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780f48` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780f60` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780f78` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780f90` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780fa8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780fc0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780fd8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10780ff0` | 44 / 23 | 1 / 1 | 10 / 6 / 2 / 1 |
| `0x10781008` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10781020` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10781038` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10781050` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10781068` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10781080` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10781098` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107810b0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107810c8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107810e0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107810f8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10781110` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10781128` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10781140` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10781158` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10781170` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10781188` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107811a0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107811b8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107811d0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107811e8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10781200` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10781218` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10781230` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10781248` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10781260` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10781278` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10781290` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107812a8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107812c0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107812d8` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x107812f0` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10781308` | 44 / 23 | 1 / 1 | 10 / 5 / 2 / 1 |
| `0x10781320` | 38 / 18 | 1 / 1 | 9 / 6 / 2 / 0 |

For the generic entry, incoming-CFM is zero because it is the dispatcher itself;
its direct/generated and native/guest descriptor compositions execute normally.
Generated totals include successful finite-budget cases. Selected diagnostics
and expected interruptions are excluded from those successful path counters.

## Protected installation and installed smoke

The previously installed signed ARM64 app and the installer's prior source were
preserved privately with content, resource forks, extended attributes and
metadata before promotion. The raw linker output was copied to a new candidate,
the four unchanged public original program payloads were added, and the candidate
was signed and strictly verified before source promotion and one final installer
call. No user game session was running. Source, candidate and installed
executable identity and full candidate/installed inventories agree.

The raw linker executable SHA-256 is
`0081bd3367ef7dc0b135027177c21c2d92255b4f75cc1687adf5d8bc768ce00a`.
The accepted installed ARM64 executable SHA-256 is
`7d78391d80e68f4de4e911cf6c66c17dfa3dcf7309c6b8a29e8062cbd4ccc452`.
It is installed at
`/Users/victordmor/Applications/CM 01-02 Native Prototype.app`.
Its production plist has no diagnostic environment and its signature and four
original payload hashes pass verification.

The installed start menu was visibly rendered. Named Load Game opened the
original chooser showing Reading; named Back or Cancel returned to the menu.
No career was loaded. Original Quit and its confirmation ended the owned process
normally after **14,352,921,809** statically recompiled instructions. Its dedicated
log records **37,360,301** semantic calls/tails across **115** individual entries,
including **5,442,294** generic-CFM tails. Every original/profile/budget selection,
own fault, budget stop, context-transfer and dispatch-escape count in this block
is zero. These UI observations establish those 115 entries only; all 260-entry
coverage comes from the focused harnesses.

Protection passes immediately after installation and after smoke for game
content, user saves, original payloads, snapshot, 23 handoffs, references and 180
older app bundles. Logs/preferences may legitimately change during the owned
smoke; Runtime/Runs are not claimed byte-identical afterward. Save/Load remains
**not retested**. No career, day, match, season, performance benchmark or broad
C++ suite is included in this increment. Other native services and the remaining
engine/runtime reconstruction are still incomplete.
