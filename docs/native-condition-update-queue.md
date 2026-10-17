# Native condition update queue

This report records the preceding 699-entry delivery. The subsequent
[post-match condition application](native-post-match-condition.md) extends the
component to eighteen entries and the accepted manifest to 700.

This block reconstructs the complete queue merge/insert (`0x10186cd0`) and
borrowed lookup (`0x10186e10`) functions in C++23. The queue allocator,
constructor, append and iterator remain explicit original dependencies. Post-match
application of queued requests remains original and is outside this block.

## Original contracts

| Entry | Exclusive end | Indexed instructions | Original external caller |
| --- | --- | ---: | --- |
| `0x10186cd0` | `0x10186e0c` | 78 | `0x101f69e4` |
| `0x10186e10` | `0x10186e94` | 32 | `0x10397d58` |

The pinned index has no external interior entrances. The skipped alignment words
at `0x10186d24` and `0x10186e4c` are separately checked as `0x60000000` against
original executable bytes. Both executed loop-entry NOPs remain accounted for.
All old semantic manifest entries retain their exact order and metadata.

Merge accepts a controller, staff record, kind-byte pointer and amount-byte
pointer. It initializes a missing queue, resets the iterator, and changes only
the first matching staff identifier. Equal signed kinds add signed amounts and
cap only the upper limit at 100; negative sums wrap into the stored byte.
Different kinds retain the existing item unless the incoming signed amount is
greater. Retained kind and amount are then reread and written to the caller in
original order, including when pointers alias the item or each other.

Absent identifiers append an eight-byte stack record. Its final two bytes retain
the original stack contents. Failed allocation stores the null pointer and then
faults at the original queue read; no new recovery behavior is introduced.

Lookup resets and advances the same iterator, returning the first matching item
pointer directly. It returns null for a missing queue or identifier. The staff
pointer is read only after a nonempty iterator result. No item is copied or
removed, and later duplicates remain untouched.

## Independent and production evidence

The accepted manifest increases from **697 to 699 entries**, with its exact prior
prefix intact. Seventeen attribute/condition entries now cover **3,690 indexed
original instructions**. Complete regeneration retains 1,953,334 instructions,
1,922 pages and no missing/pending targets; only `page_10186.cpp` changes among
1,923 generated C++ files. Named C++ bodies own both roots without replaying their
original body or falling back into it. Nine scope/source/hook audits pass, along
with 50 historical manifest-only methods; the historical test-file changes only
update the current global count from 697 to 699.

Release and affected-unit UBSAN each pass **377,174 component comparisons**:
9,465 completions, 367,677 interruptions and 32 discriminating own/child negatives.
All 3,690 source sites are covered. The queue block adds 10,461 comparisons and
covers every one of its 110 indexed sites. The 36,400 structural comparisons
belong to preceding condition/sampler branches; queue coverage requires no forced
branch or register controls.

Full registers, temporaries, mapped memory, ordered reads/writes, instruction
accounting and dependency input/output ABI agree with independent original
references. Cases include signed kind/amount boundaries, upper cap and negative
wrap, first/late/missing/duplicate keys, original append padding, null arguments,
allocation success/failure, aliased kind/amount/item/stack pointers and caller
bytes changed during an iterator callback. Every budget cut, each individual
missing accessed byte and every dependency interruption are compared. Independent
merge expectations check retained kinds and amounts; lookup checks first-match
pointer identity. A failed-allocation fixture explicitly requires the null queue
store followed by the original fault at `0x10186d18`.

Shared-engine evidence uses a captured real controller/staff world, original
allocator, queue constructor, append and iterator, plus prepared original calls
at `0x101f69e4` and `0x10397d58`. These are controlled source-callsite invocations,
not traversal of the complete parent functions. Nine prepared variants cover a
matching duplicate, cap, retained/replaced kinds, negative incoming amount,
missing-queue allocation, borrowed lookup, absent identifier and null queue with
null staff. Both roots have dedicated nonzero C++ call/return counters.

Direct, generated-dispatch and CFM calls, global/per-entry original controls,
budgets and own negatives contribute **153 new production comparisons**. Aggregate
Release and UBSAN each pass **504 comparisons**: 300 returns, 204 budget stops and
30 negatives. Fifteen separate null-first-argument fault comparisons also agree.
**90 original-profile controls** pass in Release, including the seven extra queue
variants. Original/profile controls require zero modern calls.

A paired **798-advance match** reaches full time with identical full state,
imports and per-tick accounting. The preceding naturally used roots retain
17,728 refresh, 70,912 maximum, 117 recovery, 17,490 periodic, 13,119 consumption,
80 restoration, 80 record-scale, 22 availability, 22 effective-condition and
13,799 daily-refresh calls. Queue merge/insert has **nine natural calls and
returns** in this match; lookup has **zero natural calls**. The lookup runtime proof is the controlled actual-data
invocation above; merge also has that independent controlled matrix.
Earlier daily rollover, record lifecycle and structural sampler recursion still
pass; no whole-career or population-wide frequency claim is made.

**630 nonlocal comparisons**, including **360 restore/handler faults**, pass in
Release and UBSAN. The queue roots add 84 comparisons. Both memory channels cover
original frame saves and cursor writes; merge additionally interrupts the caller
amount write, while lookup's secondary channel interrupts the staff-key read.
Lookup's primary-only third site is its link save because the body has no later
item/output write. Original-context and turbo controls verify complete state,
ordered effects/import ABI, exact accounting, resumed handlers and abandonment of
the interrupted C++ continuations. No transferred root falsely reports a return.

All four focused CTests pass. Fourteen affected units are instrumented with
UBSAN, including the new body, harnesses, independent reference, adapter and
runtime diagnostics; all three runs finish with empty stderr. Their recorded
source hashes match the reviewed sources. Other generated pages and archives
remain consistent Release builds. This is not a whole-engine sanitizer or speed
claim.

## Installed game and retention

The signed ARM64 game executable is installed with SHA-256
`a1ca827870a6a896986db5ac00d5b249d2747b730715ed682674905691327624`.
Candidate and installed identities agree; all four bundled original
program payloads match the protected input. Installed **Prepare Game** rendering,
original exit confirmation and normal exit pass. This visual smoke is separate
from the queue runtime comparisons.

All **845 protected files**, the preceding installed-game reference and the
installed Match Debugger remain unchanged. Cleanup removes only the six reviewed
linked Release/UBSAN checks (866,599,496 logical bytes),
retaining app bundles, libraries, objects, original inputs, snapshots, reference
sources, recipes and evidence. Free space after cleanup is
80,939,003,904 bytes.

Save/Load: **not retested**. Performance: **not benchmarked**. The full conversion
goal remains active; post-match queued-request application and other original
routines still require separate reconstruction and acceptance.
