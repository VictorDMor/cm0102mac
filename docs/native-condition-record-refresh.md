# Daily condition record refresh reconstructed

This report records the preceding 692-entry delivery. The subsequent
[capacity and lifecycle block](native-condition-record-lifecycle.md) extends the
component to fourteen entries and the accepted manifest to 696.

The complete daily record refresh at `0x10187a40..0x10188488` now has a named
C++23 implementation. Its **658 original instructions** extend the accepted
manifest from 691 to **692 unique entry replacements**, preserving the previous
691 entries exactly. The ten-entry attribute/condition component now covers
**2,842 original instructions**. This is a bounded functional delivery; the whole
engine and native port remain unfinished.

## Scope and behavior

The implementation in `native/actor_condition_refresh.cpp` separates daily cache
initialization and invalidation, injury-day partitioning, team/calendar selection,
training draws, condition damping and record capacity restoration. It retains the
224-byte frame and original nineteen integer saves (`r13..r31`). Live pointers,
call arguments, continuation addresses, flags and source accounting remain guest
state; helpers do not replay generated bodies or bypass original callees.

One-time globals and three cached date objects initialize before the nullable
record guard. A null record, signed out-of-range index, missing profile or current
date takes its original early return, with preceding effects preserved. Negative
indices retain their original arithmetic; no new lower bound is imposed. Injury
indices outside the original limits reset to `-1` before subsequent reads.

Elapsed injury and calendar intervals retain their signed arithmetic and original
partitions. Team dates and cached club/date eligibility are reloaded across the
same dependency boundaries. Training byte counters preserve the original limit
and wrapping writes. Random draws retain pool advance, end wrapping, reseeding,
signed remainders, wide-bound sampling and the explicit correction dependency.

Condition and capacity updates retain the 7,000 / 9,000 / 9,850 damping tiers,
10,000 caps, fallback values and signed trait/scalar behavior. Floating products
and conversions preserve separate rounding under `-ffp-contract=off`, live
constants, enabled exception behavior and original status bits. Final writes keep
their order: the two condition halfwords followed by year, day and date flag.

The accepted date/calendar and random core retain their existing owners. Injury
retirement (`0x1018b9f0`), team selection (`0x102ae9c0`, `0x102af650`), club calendar
lookup (`0x101929b0`) and the biased sampler (`0x101864a0`) remain explicit original
dependencies. Their callers execute through the production dispatcher; this
report does not claim those larger services are reconstructed.

## Independent and production evidence

Source contracts pin all 658 instructions and both page fragments (368 and 290
instructions). Complete-body, source-field, incoming-edge and hook audits reject
changed or incomplete originals. Every indexed external incoming edge targets
the public entry. Independent references retain all 2,842 component source sites
with semantic hooks disabled. Only `page_10187.cpp` changes among 1,923 generated
C++ files: the second page keeps its original body. Generation retains 1,953,334
instructions across 1,922 pages with no missing or pending targets. Guest State
layout is unchanged; counters remain private diagnostics.

Release and affected-unit UBSAN each pass **299,010 component comparisons**:
5,685 completions, 293,308 interruptions and 17 discriminating own/child negatives.
All **2,842 source sites** are covered, including the complete new operation.
Full registers, temporaries, mapped memory, ordered effects, dependency ABI and
accounting agree at returns, budget stops, individual missing bytes and dependency
interruptions. Fixtures include guard initialization, cache hits/misses, signed
indices, date/injury boundaries, dependency writes, relocated globals, random
wraps, tier limits, exceptional floats and enabled conversion exceptions.

The **21,424 structural comparisons** include wide-bound training draws and
three source-condition branches excluded by ordinary predicate invariants.
Those three branches are exercised by identical source-site condition-bit
mutations in the original and C++23 versions; they are not claimed as natural
match events. Component fixtures isolate explicit dependency boundaries, while
production checks use the actual shared engine and original remaining callees.

A complete paired **798-advance match** reaches full time with identical full
state, host input/import ABI and every tick's instruction accounting. Daily
refresh executes naturally **13,799 times**, with matching returns and no original
selection, fault, budget stop or dispatch escape. The previous nine roots retain
their productive counts: 17,728 refresh, 70,912 maximum, 117 recovery, 17,490
periodic, 13,119 consumption, 80 restoration, 80 scalar, 22 availability and
22 effective-condition calls.

A separate **controlled next-day scenario** advances the original live date
through its accepted constructor, then invokes the actual availability caller
using captured original player/controller data. Both versions complete the daily
record writes with identical full state, effects, import ABI and accounting.
This establishes a productive cache miss separately from the naturally frequent
cache-hit path; it is not labeled a natural match date rollover.

Actual caller captures and that completed cache-miss capture pass **181 production
comparisons**: 110 returns, 71 budget stops and eleven own negatives across direct,
generated and CFM calls, including global/per-entry original controls. Nine
separate null-first-argument faults retain exact fault state/effects/accounting.
**33 separate original-profile controls** pass, including the updated record.

**322 nonlocal comparisons**, including **184 restore/handler faults**, pass.
The new root is interrupted during its initial multiple-register save, after its
live cache reads, and at the final condition write through both memory channels.
Its 224-byte frame and old host continuation are abandoned correctly: later
stores and epilogues do not execute after transfer. Existing scalar-read,
integer/floating-save, original-context and turbo controls remain included.

All four focused CTests pass in **80.71 seconds**. Nine contract/reference/table/
hook audits and 48 historical manifest checks pass. Eleven affected units are
instrumented with UBSAN; all three sanitizer runs pass with empty stderr. Generated
pages and unchanged archives retain the consistent Release build; this is not a
whole-engine sanitizer claim. A fixture that initially copied full State on every
cache hit was superseded: detailed write capture now runs only in the controlled
rollover scenario. This changed observation cost, not product behavior.

## Delivery and remaining work

The signed ARM64 game executable is installed with SHA-256
`119af966b7bf8f5e71fd5aa45d9b99cd9b3ef32532c1d6c523782352f45d12db`.
Candidate/installed identities, strict deep signatures and all four original
payloads agree. The previous 691-entry game is retained as a private application
reference. All 845 protected original/save/handoff files and the installed Match
Debugger retain their hashes. No debugger source or installed debugger changes
are part of this delivery.

The installed game reaches Prepare Game, shows the original exit confirmation
and exits normally after 4,425,904,003 statically recompiled
instructions. This UI smoke establishes startup and normal exit; the separate
match/rollover harnesses establish daily-record execution. Save/Load is **not
retested**. No new speed measurement or optimization claim is made. Accurate
conversion remains the priority, and the complete-port goal stays active.

Cleanup removes six reproducible check executables
(864,667,928 logical bytes) and the superseded fixture's
11 copied files (16,553,842 bytes).
Objects, archives, recipes, application references, original inputs and evidence
remain. Observed free space after cleanup is 75,798,507,520 bytes.
Private receipts, source contracts and reproduction scripts remain under
`.local/condition-record-refresh`. The raw capacity query, injury retirement and
biased sampler are characterized candidates for subsequent conversion, not
accepted implementations in this report.
