# Mac clock, calendar and daylight runtime

## Scope and acceptance

Eight complete Mac routines are reconstructed in semantic C++23. The block
covers the zero-status leaf, years since 1900, seconds-to-calendar conversion,
field initialization, the optional-output time facade, shared local fields,
Mac clock epoch conversion, and daylight status. It preserves the existing
native GetDateTime/ReadLocation services and generated import-descriptor glues.
It does not change Date's ascending-year leap behavior, the allocator progress
guard, gameplay, serialization, or the host clock/time-zone policy.

All eight entries are delivered in default semantic C++23 production. A complete
consistent Release ARM64 build, six focused CTests, 22 Python suites / 144 tests
with zero skips, thirteen-unit UBSAN and the protected final installation pass.
The initial recursive-transfer fixture used an invalid native-import TOC; the
accepted fixture uses the real CFM descriptor transition. Its corrected focused
check passes; that failed attempt is excluded from passing evidence. The complete generation has
1,953,334 instructions, 1,922 pages, zero missing/pending targets, and **276**
semantic replacements. Its first 268 entries exactly preserve the accepted
word-utility and comparator-search delivery. Save/Load is **not retested**;
no career, day, match, season, performance benchmark or broad suite is claimed.

## Original contracts and production hooks

Intervals are end-exclusive. Pins serialize address-ordered records containing
address, assembly, ops, flows, and fallthrough using sorted JSON keys and compact
separators. Counts are indexed instructions, including only actual labels.

| Entry | End | Instructions | Operation | SHA-256 |
|---|---|---:|---|---|
| `0x1077f6d0` | `0x1077f6d8` | 2 | Zero-status leaf | `c08bd868583cc19a2eb098e924ed9a61d2c60ce985afbd20caf94c7a4488e8a5` |
| `0x1077f6e0` | `0x1077f754` | 29 | Leap-year predicate for a year offset from 1900 | `7470642964c75709c692ec058c3e3ad21dc00913a60df9511f64686c4fd3f6ee` |
| `0x1077f760` | `0x1077f8b8` | 86 | Seconds to calendar fields | `635c07671a24d37e679357c2c000749931619ce0455e12fd2c4a52578d87eab5` |
| `0x1077f8c0` | `0x1077f8f4` | 13 | Calendar-field initialization | `a70545ff29a422a30a08139b07eb9b660674c6c24847e6d6c5896a7bf6fe56f6` |
| `0x1077f900` | `0x1077f938` | 14 | Time value and optional output | `f7b6ec8ccb77019e9dab4641488801b3aa66ebb1c422f919a36b93447eee33c8` |
| `0x1077f940` | `0x1077f994` | 21 | Shared local calendar fields | `502fb1ba8b5ae5c2f9997494b87e077a0f7a457c32776b26aae48778c15e675d` |
| `0x1077f9a0` | `0x1077f9d4` | 13 | Mac clock epoch conversion | `8f97d91bbc7a6a0254c67130476ebf1a4630a15362fbbd062d40defe2dc23fb9` |
| `0x1077f9e0` | `0x1077fa48` | 26 | Machine-location daylight status | `18617496cd991b9de25bc0378e8b1794362d546bb7bda5232934b6ff40a39746` |

The eight bodies contain 204 instructions on page 0x1077f. Full-index audit
rejects external edges into their interiors. Root hooks precede the first
original operation at each destination label, covering local, generated and
CFM-indirect dispatch. Missing/changed/truncated/duplicate/reordered contracts
fail closed; an original-only reference contains all 204 labels without
semantic hooks. Original instructions, generated source, indices and payloads
remain private.

The complete 276-entry manifest SHA-256 is
`ba418ebeea27f3e546fe4f02aed4ddaa192bbcafa009d0c8552411a739b4ebb2`.
The preserved first-268 digest is
`97d4e7d8d5633d4efc9c771a720a4ddedfa347de67e7600c20ceca91ac20fbec`.

## Clock and calendar semantics

The clock routine calls the live descriptor glue for GetDateTime into a
four-byte stack output, restores the caller TOC, and subtracts 0x7c25b080 modulo
word32. The outer time facade stores that value only for a nonnull output
pointer; null does not suppress the clock call. Both retain guest frames,
scratch registers, access order, fault prefixes and imported service effects.

Calendar conversion takes the timestamp in r3 and destination in r4. It saves
r27..r31 and LR, reads the live month-table pointer at r2-0x5a28, records the
destination condition, and creates an 80-byte frame before the nullable guard.
The branch uses the live condition field after those accesses. A null result
preserves the original frame and table-pointer effects.

The unsigned elapsed seconds are the timestamp minus 0x7c558180 modulo word32,
representing the original 1900 epoch. Named operations split the day and weekday,
find the year, select the month, and publish clock fields. Original reciprocal
products, scratch residues and word32 overflow remain observable; there is no
host date conversion in production. Year offsets start at zero for 1900. The
leap predicate calls the already-semantic floor remainder with 4, 100 and 400;
its century residue comparison is 100 because of that year-offset convention.
Negative and extreme words retain original signed remainder behavior.

The cumulative-month table contains two live rows of thirteen signed big-endian
halfwords, at byte offsets zero and 26. Reads retain signed extension followed
by the original unsigned comparison. Mutated rows are not replaced with host
constants, and malformed rows preserve original traversal, fault and finite
budget behavior. Calendar publication order is weekday, year, day of year,
month, day, hour, minute, second. DST at offset 32 is untouched by this routine.

The initializer publishes all nine words in the original interleaved access
order: default 1900-01-01 Monday, zero clock fields, and DST -1. The local-fields
facade reads the shared output pointer at r2-0x3b20. A null input initializes it;
a nonnull input loads the timestamp and converts it. Both then query daylight,
overwrite DST, and return the live shared pointer. Input/output aliasing and
mutable TOC slots retain ordered original effects.

ReadLocation writes the original twelve-byte stack record through its live
import descriptor. An all-zero latitude/longitude/GMT record returns -1.
Otherwise daylight is the sign-extended high GMT/DST byte reduced to a nonzero
boolean; other nonzero record words do not imply daylight. The full-word tests
short-circuit in their original order. The service's native geographic, GMT
and daylight encoding remains outside this reconstruction.

## ABI, interruptions and diagnostics

Every architectural byte, including GPR/FPR, CR/XER, LR/CTR, TOC, FPSCR and
generated scratch, is compared with the original at returns and faults. The
multiple-register save/restore cursor remains live across each access, including
observer mutations. GuestNativeScope owns each semantic frame. Calls retain
original return addresses and descriptor/register effects; faults do not run
an invented epilogue. Scalar context transfers abandon all interrupted semantic
frames and resume the live dispatcher owner.

Successful and budget-stop instruction accounting is exact. Semantic phases
charge original counts and resume the guarded original continuation when the
remaining finite budget cannot fit a phase. Mapping faults within a precharged
phase retain architecture and ordered effects; their instruction count is not
claimed exact. Diagnostic continuations and original/profile selection have
separate counters and do not count as successful normal semantic coverage.

The eight-entry selection arrays, call/return/transfer/fault counters, budget
continuation diagnostics and observer remain process-local and unsaved. Reset
is independent of earlier modules. Original selection guards only the selected
root while allowing ordinary dependency calls; default normal production
executes the new bodies and does not execute its own original body.

## Evidence and installed artifact

Component acceptance has **3,025** original/semantic comparisons, 2,833 exact
accounting comparisons, 419 exact budget stops, eight own negatives, minimal
mappings, endpoint mapping faults, live CR/frame/cursor/destination effects,
mutated signed month rows, malformed traversal, and clock/location import faults.
Independent expectations cover Gregorian fields, leap offsets, word32 epochs,
optional outputs, all-zero location, and signed daylight bytes.

Production acceptance has **2,036** comparisons, 713 exact budget stops,
268 diagnostic-continuation cases and nine own/child boundary negatives. Every
entry has nonempty successful direct, generated and CFM calls with zero original,
profile, budget continuation or escape in normal coverage. Separate global and
individual original selections, unrelated selection, dependency compositions,
reset independence, mapping/import faults and eight profile selections pass.
There is no claim of original-PC sample coverage on this non-hot page.

| Entry | Component comparisons | Component fault/budget outcomes | Own negative | Normal direct / generated / CFM |
|---|---:|---:|---:|---|
| `0x1077f6d0` | 204 | 2 | 1 | 2 / 2 / 2 |
| `0x1077f6e0` | 1,027 | 81 | 1 | 2 / 2 / 2 |
| `0x1077f760` | 612 | 244 | 1 | 2 / 2 / 2 |
| `0x1077f8c0` | 223 | 31 | 1 | 2 / 3 / 2 |
| `0x1077f900` | 232 | 55 | 1 | 2 / 2 / 2 |
| `0x1077f940` | 273 | 121 | 1 | 2 / 3 / 2 |
| `0x1077f9a0` | 224 | 34 | 1 | 2 / 2 / 2 |
| `0x1077f9e0` | 230 | 43 | 1 | 2 / 2 / 2 |

Fault/budget totals are explicitly expected outcomes, not normal success.
The nonlocal harness has **128** original/modern/mixed comparisons and 64
restore/resumed-handler faults across four clock/local-time roots, recursive
CFM calls, normal/turbo dispatch and four scalar-context selections. Known
ownership, zero unknown transfers, abandoned frames/callbacks, unchanged markers
and complete architectural/access/import effects are checked.

The six passing focused checks are time_runtime_component_equivalence,
modern_time_runtime_production, original_time_runtime_profile,
modern_time_runtime_transfer, modern_stdlib_core_production, and
modern_register_context_transfer. The corrected transfer check is the final
acceptance for that fixture; other passing checks did not require repetition.
The thirteen explicitly instrumented UBSAN units cover the core, adapter, three
harnesses, runtime/transfer/register dependencies, generated pages 0x1077f/10780/
10781, and the original reference. Component, production, profile and transfer
runs have zero exits, empty stderr, and identical Release/UBSAN output.

The accepted signed installed app is
`/Users/victordmor/Applications/CM 01-02 Native Prototype.app`, ARM64 executable
SHA-256 `c21cad895a67e5ff3142cbd532bcfad1b9d5716b2d68553358bb2f759b333405`.
The unsigned linker executable is
`35e4b48507ea54df8ca3e0c039ac5b605102b627e329f95a39970bfb9dad491c`.
The previous installed/source reference is
`1f08d21e93d6c1be300a244a3a081db4e91eed7d32cf794425a0fdcfcd484bde`.
Candidate, installer source and installed inventories match; strict signature,
normal production plist and the four unchanged legacy OriginalProgram payloads
pass. An initial packaging attempt failed before source promotion and invoked
the installer against the previous app. That unchanged-artifact attempt is
excluded from delivery; the corrected final installer used the verified new
source hash before the installed identity was accepted.

The exclusively owned smoke PID 47389 rendered the start menu, opened the
original Load Game chooser without loading a career, returned via named Back or
Cancel, and exited through the original quit confirmation. Its log records
normal exit after 7,117,799,885 statically recompiled instructions. Semantic
calls/returns are 48 for 0x1077f6d0 and seven each for 0x1077f900/1077f9a0;
other time roots have zero UI calls. All time original/profile/budget/resume/
transfer/fault/escape diagnostics are zero. Complete individual coverage is
established by the harnesses, not inferred from the smoke.

Game content and Mac metadata, user/workspace saves, payloads, 23 handoffs,
installed/source references and 175 older private bundles are preserved.
Runtime/Runs and preferences may legitimately change through the smoke.
Persistence remains **not retested**, and the whole C++23 engine/runtime goal
remains active. Component fixtures use controlled clock and
location imports, not a nondeterministic host wall clock. Independent calendar
expectations use C++ chrono only in tests. Dedicated entry counters establish
individual runtime coverage; a startup menu establishes only the observed smoke.
