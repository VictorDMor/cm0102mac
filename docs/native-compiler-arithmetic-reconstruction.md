# Mac compiler arithmetic: semantic reconstruction

## Acceptance boundary

All eight complete compiler-arithmetic bodies now run named C++23 operations in
normal ARM64 production. The manifest contains **234 semantic replacements**,
with SHA-256
`9f3d57d5077117883869f644578eb0d91f972527febf4b742ab5598f54f043b0`.
The accepted first 226 entries remain exact, with SHA-256
`cd2bf157adfb6e441959706bdfc12f8f4998aa07683b390e03c1ca31d1cfad0c`.
The previous production baseline was commit
`69f5f8ce0c18fec51e1953ad83ff8c6ff167782e`.

Generation retains 1,953,334 instructions, 1,922 pages and zero missing/pending
targets. A fresh complete Release ARM64 build uses the expanded State layout in
every generated unit, runtime library, harness and application. Original input
records and initialized payloads remain unchanged. Historical build directories
were not reused for execution checks. This block advances the complete port;
the overall goal remains active.

## Complete pinned scope

End addresses are exclusive. Counts contain only indexed instructions;
gaps and padding are not counted. Each SHA-256 covers ordered canonical
`address`, `assembly`, `ops`, `flows`, and `fallthrough` JSON, with sorted keys
and compact separators. All eight bodies occupy page `0x10773`.

| Entry | End | Instructions | Named operation | SHA-256 |
| --- | --- | ---: | --- | --- |
| `0x10773780` | `0x107737d0` | 20 | Double to unsigned word | `decb1a8c06d57b4151543f7e1eba9803549371fddbee2a93482fb6defe06cb3b` |
| `0x10773870` | `0x1077395c` | 59 | Unsigned quotient | `97b2bbe0d7f7d13c545c90d312f0fdc825eed2b599eeac82b86518f507d243bb` |
| `0x10773960` | `0x10773a90` | 76 | Signed quotient | `2ba4b9094dc3889a72b1dfb0830e9b22864f3e5baf2c69c754a872a6a117eebb` |
| `0x10773a90` | `0x10773b70` | 56 | Unsigned remainder | `cbada6c54a27fa77a267379589790277462a9a23aacb4673f95b53bc625147e9` |
| `0x10773b70` | `0x10773c7c` | 67 | Signed remainder | `0a87c187fe59c2a6b52340a4f09f31e4f72a603d1806730ad80e91de3eab9718` |
| `0x10773c80` | `0x10773d28` | 42 | Signed pair to double | `732d8095f048bfb60440aed59c7e635dbe92dd367040526f948add5043f76096` |
| `0x10773d30` | `0x10773ddc` | 43 | Signed pair to single | `a20f7b0cfcb266db6c3e7b784df96bf24f2c748c1d99f1ec47f354580f280ac4` |
| `0x10773de0` | `0x10773ea4` | 49 | Double to signed pair | `044df206c652517be64942d3d93a0105f687a5dbe903bac08b44136461f1ca3c` |

Total: **eight entries / 412 instructions**. The full indexed edge audit rejects
any external flow into an interior address; the current index has none. Entry
flows and internal branches are allowed. Startup, suffix floating-register
save/restore, setjmp/longjmp and the neighboring lifecycle routines are outside
this scope.

The bounded generator emits all 412 original labels under private template and
dispatcher names. Both finite and unlimited wrappers select `Turbo=false`, so
the original reference cannot invoke the existing fast unsigned-divider shortcut
even when the process's production Turbo preference is enabled. Body mutations,
missing entries/instructions, duplicate/reordered instructions and external
interior edges fail closed. Original listings, exports and generated reference
sources remain private.

## Guest ABI and effects

All entries are leaf operations returning through the live guest LR. The stack
pointer and TOC are preserved, without a new frame or host allocation. Word
pairs place their high word first; guest memory accesses remain big-endian.
The table summarizes the active ABI; the harness compares the complete state,
including registers and scratch bytes that an individual branch leaves intact.

| Entry | Arguments and result | Active observable effects |
| --- | --- | --- |
| `0x10773780` | `f1` double to `r3` unsigned word | Live TOC slot `r2-0x3b7c`; three threshold loads; `r4`, `f0/f2/f3/f4`, CR0/6/7 and floating status; scratch store at `sp-8` and word read at `sp-4` on the conversion branch |
| `0x10773870` | Unsigned `r3:r4 / r5:r6` to quotient `r3:r4` | Divider scratch `r0/r7-r10`, CR0, XER carry and CTR; no guest memory access |
| `0x10773960` | Signed pairs to quotient `r3:r4` | Magnitude normalization and sign restoration; two sign stores/live reloads at `sp+24/+28`; divider scratch and CR0/carry/CTR |
| `0x10773a90` | Unsigned pairs to remainder `r3:r4` | Divider scratch, final subtraction, CR0/carry/CTR; no guest memory access |
| `0x10773b70` | Signed pairs to remainder `r3:r4` | Signed magnitude normalization, dividend sign in CR7 and conditional pair negation; divider scratch and CR0/carry/CTR; no guest memory access |
| `0x10773c80` | Signed `r3:r4` to double `f1` | Normalization/rounding scratch `r3-r10`, CR0/carry; word stores at `sp+24/+28` followed by a live eight-byte load |
| `0x10773d30` | Signed `r3:r4` to single rounded into `f1` | Same guest publication as double; original double-then-single stage and bytewise floating-result status |
| `0x10773de0` | Double `f1` to signed `r3:r4` | Eight-byte store at `sp+24`, separate high/low reloads and `r5-r10` extraction/shift/saturation scratch, CR0/carry |

## Named operations and observable contracts

`compiler_arithmetic_integer.cpp` computes nonzero quotients and remainders with
unsigned 64-bit arithmetic, retaining the software divider's final subtraction,
scratch words, CR0, carry and CTR residues. Signed operands are normalized and
negated as word pairs, preserving `INT64_MIN / -1` without host signed overflow.
Signed quotient stores its two sign words at guest stack offsets 24 and 28 and
reloads them live after division. Signed remainder retains the original CR7
dividend-sign decision. Early returns leave the original untouched scratch
registers intact.

Zero divisors follow the characterized software behavior rather than host
division or an invented trap. For a dividend with `z` leading zeros, the loop
shifts in up to `65-z` quotient bits, saturating the 64-bit pair with ones. A
top-bit dividend causes the original initial count of -1 to clear both word
lanes; its remainder is zero. Other dividends survive as the remainder.
Examples checked independently include `0/0 -> quotient 1`, `1/0 -> quotient 3`,
unsigned `UINT64_MAX/0 -> quotient UINT64_MAX, remainder 0`, and signed
`INT64_MIN/0 -> quotient 1, remainder 0`. These are compatibility observations,
not mathematical division rules.

`compiler_arithmetic_conversion.cpp` reconstructs signed-pair normalization,
tie-to-even rounding, exponent/mantissa packing and the guest stack publication.
The single result retains the original double-then-single rounding stage.
PowerPC word shifts mask counts to six bits and clear a word for counts 32–63.
The conversion back to a signed pair uses the live stored exponent/mantissa,
truncates small magnitudes and retains sign-dependent overflow/Infinity/NaN
saturation. Guest stack writes and later reads remain separate operations.

The unsigned-word conversion reads all three thresholds through the live
relocated TOC slot, performs its original comparisons and high-range subtraction,
and delegates integer conversion to the accepted `original_fctiwz` policy.
Ordered threshold/scratch accesses, signed zero, NaNs, sticky flags and enabled
invalid/inexact exceptions are preserved. Legacy floating-result status retains
the export's bytewise flags and enabled-summary XOR; this stage does not replace
that behavior with a new host rounding or exception policy.

Every memory operation carries the original instruction PC for fault-prefix
comparison. Guest memory observers can mutate sign slots, assembled floating
words and thresholds before their subsequent reads. The components do not
allocate host/guest objects or alter native heap bookkeeping.

## Component verification

The harness compares every architectural register byte and unique-space byte,
PC, all mapped memory and mappings, ordered reads/writes including their bytes,
heap maps/end/error and native-call counts at returns and expected faults.
Nonzero initial sentinels cover untouched state; heap bookkeeping is populated.
Instruction accounting and process-local diagnostics are excluded from the
architectural snapshot, as in the other semantic component harnesses; the
unsigned quotient has a separate exact-cost assertion. The raw bodies leave `executed` unchanged.

| Entry | Comparisons | Mapping-fault prefixes | Enabled conversion traps | Own rejected negative |
| --- | ---: | ---: | ---: | ---: |
| `0x10773780` | 1,155 | 49 | 2 | 1 |
| `0x10773870` | 2,120 | 0 | 0 | 1 |
| `0x10773960` | 2,129 | 8 | 0 | 1 |
| `0x10773a90` | 2,120 | 0 | 0 | 1 |
| `0x10773b70` | 2,120 | 0 | 0 | 1 |
| `0x10773c80` | 1,227 | 8 | 0 | 1 |
| `0x10773d30` | 1,227 | 8 | 0 | 1 |
| `0x10773de0` | 1,101 | 4 | 0 | 1 |

**13,199 comparisons / 77 mapping-fault prefixes / two enabled traps / eight
rejected output negatives.** Coverage includes magnitude/sign/power boundaries,
fixed zero-divisor results, varied divider iteration lengths, conversion ties,
subnormals, NaNs, infinities, relocated TOC, unaligned stack, minimal mappings,
stack/threshold aliases and live memory mutations. Each observed memory access
is probed by removing its first or last byte. Bytewise FPSCR sentinel cases also
retain the legacy status semantics.

## Production dispatch and accounting

All eight generated destination labels invoke the semantic adapter before the
first original step. The page counter publishes its deferred count before the
boundary and adopts the resulting count on both return and exception. The old
page-level unsigned-divider shortcut cannot bypass these hooks or diagnostics.
The full-index edge audit found no external interior entrances and no same-page
external callers to these roots. Generated CFM dispatch is exercised separately.

Global and individual original selection, modern/original return/fault counters,
entry/leave observers, reset and dispatch-escape detection are process-local and
unpersisted. Budget and PC-profile original executions have separate counters;
they are not reported as normal semantic coverage. Normal direct, generated and
CFM cases require zero original selections, budget/profile selections and escapes.
The private `CM_AOT_COMPILER_ARITHMETIC_ORIGINAL` switch retains an original
comparison path; it is absent from the normal production plist.

The unsigned quotient preserves its existing exact full-call instruction charge.
The named cost calculation uses leading-zero counts, quotient population count
and branch costs, without replaying the restoring loop. A zero divisor can have
65 successful subtractions although the returned quotient pair holds only 64
bits; that extra subtraction is included. All 2,120 quotient component cases
also compare the calculated count against the isolated original body.

The other seven operations retain the established semantic entry charge rather
than claim original full-body instruction totals. Conservative original bounds
are 20, 780, 760, 771, 42, 43 and 49 respectively. If the available budget does
not cover the relevant bound, the original body executes before any semantic
architectural or memory change. This preserves partial stopping PCs, failing
steps, counts and access prefixes, including counter overflow. Profiling likewise
uses the original bodies to retain actual original-PC samples.

| Entry | Native normal calls | Generated normal calls | CFM normal calls | Diagnostic-original calls | Own production negative |
| --- | ---: | ---: | ---: | ---: | ---: |
| `0x10773780` | 2 | 2 | 2 | 4 | 1 |
| `0x10773870` | 2 | 2 | 2 | 4 | 1 |
| `0x10773960` | 2 | 2 | 2 | 4 | 1 |
| `0x10773a90` | 2 | 2 | 2 | 4 | 1 |
| `0x10773b70` | 2 | 2 | 2 | 4 | 1 |
| `0x10773c80` | 2 | 2 | 2 | 4 | 1 |
| `0x10773d30` | 2 | 2 | 2 | 4 | 1 |
| `0x10773de0` | 2 | 2 | 2 | 4 | 1 |

Every normal path is exercised with Turbo off and on. Production acceptance has
**7,422 comparisons / 4,895 exact budget stops / ten mapping faults / two enabled
traps / eight rejected negatives**, plus eight reset-isolation checks included
in the comparison total. It covers mixed selection, observer outcomes, every
fixture budget prefix and additional indirect quotient prefixes/overflow cases.
A separate profiling process passes **16 comparisons** and emits actual original
PC samples for all eight entries. Those samples are expected diagnostic stderr,
not sanitizer errors.

## Focused validation

Ten focused CTests pass, with zero failures or skips:

- `compiler_arithmetic_component_equivalence`
- `modern_compiler_arithmetic_production`
- `original_compiler_arithmetic_pc_profile`
- `original_unsigned_division`
- `original_float_conversion`
- `modern_integer_parsing_equivalence`
- `modern_formatted_output_equivalence`
- `original_match_arithmetic`
- `modern_exception_production_integration`
- `modern_file_controller_equivalence`

Integer parsing and formatted output are actual arithmetic callers. The existing
unsigned-divider helper and floating conversion policy remain regression oracles.
Match accounting and exception integration are checked against the fresh layout.
The complete build exposed an SDK `Vector128`/`pascal` collision in the existing
file-controller harness; moving the SDK include before guest aliases and
undefining its calling-convention macro fixes that compilation issue. The related
file-controller check passes; its runtime implementation is unchanged.

Twenty affected Python audit suites execute **127 tests with zero skips**. The
new suite has seven tests covering all complete pins, missing/changed/duplicate/
reordered bodies, interior-edge negatives, hook placement/counter ownership,
preserved manifest prefix and isolated-original completeness. Prior manifest
assertions and the old fast-divider hook expectations are updated. The exception
audit uses the complete recovered private input; an initial unavailable-input
run with skipped checks is excluded from acceptance.

Fresh ARM64 UBSAN instruments ten units: both arithmetic bodies, adapter,
component/production harnesses, isolated original reference, actual generated
page, reused floating conversion policy, unsigned-divider harness and modified
file-controller harness. Four instrumented executables pass five checks,
including the separate profiling process, against the consistent final libraries.
Every output matches Release. Ordinary stderr is empty; profiling stderr contains
only the expected original-PC sample report. ASAN was not run.

## Protected installation and installed smoke

The prior installed and fixed installer-source bundles are preserved separately
with resource forks, extended attributes and file metadata. The consistent new
private build is promoted to the installer's fixed source path without reusing
historical objects. Original bundle payloads are copied, then the source/candidate
is signed and strictly verified. `tools/install_native.py` runs once with no
live user game session. Source/candidate and installed full inventories agree.

| Artifact | Internal executable architecture | SHA-256 |
| --- | --- | --- |
| Private Release build and preserved linker output | ARM64 | `b82f284b0703be27688ae19e1034ec7a24d85a85dad02fa045b46197edd9163e` |
| Signed source, candidate and installed application | ARM64 | `6814c65faad2615a547a5d3ce97a3b6f840ec20c58bd09c7cac8b7a76ae3beb3` |
| Protected previous installed/source references | ARM64 | `8454b2568e865c76675c4fa73ca5601bdd4cb910366bc947ae0fdc18e4d592a5` |

The installed path is `/Users/victordmor/Applications/CM 01-02 Native Prototype.app`.
Strict signature, normal production plist and all four original payload hashes
are verified. Before smoke, content and metadata remain exact for 809 Game
files, five Runtime files, 72 existing Runs files, presentation preferences,
project save snapshot, all 23 feature handoffs, the new reference and 159 earlier
private bundles. After smoke, Game, existing user saves, payloads, snapshot,
handoffs and all references remain protected. Run logs and preferences may change;
Runtime/Runs byte identity is not claimed after UI use.

Owned installed PID 51361 has the signed hash above. Prepare Game renders; named
Load Game opens the original chooser; named Back or Cancel returns to the menu;
original Quit and its confirmation produce a normal exit. The completed log
reports 25,214,309,342 statically recompiled instructions and no fatal error.
The unsigned quotient records 12,125,476 modern calls and returns, with zero
original/budget/profile selections, faults or dispatch escapes. The other seven
UI counters are zero. The production harness supplies coverage for all eight
entries; this UI count does not imply a benchmark or gameplay campaign. An observation automatically reopens the application;
that additional owned PID 53871 is also closed normally without further gameplay.

Save/Load is **not retested**: opening/canceling its chooser is command-response
smoke, not persistence acceptance. No career, date advance, match or performance
benchmark is run. No measured speed gain or complete game/runtime reconstruction
is claimed. Original Date behavior, allocator progress policy and neighboring
runtime blocks remain preserved. Listings, input indices, generated sources,
bundles, logs, receipts and private handoff prompts remain unversioned.
