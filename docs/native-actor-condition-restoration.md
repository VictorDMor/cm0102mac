# Actor condition restoration and record queries reconstructed

This delivery extends the [periodic condition/consumption block](native-actor-condition-reconstruction.md)
with four complete C++23 operations. The manifest contains **691 unique semantic
entry replacements**, retaining the preceding 687 entries exactly. The nine-entry
attribute/condition component now covers **2,184 original instructions**; this
cycle adds 534. These counts describe accepted scope, not whole-port completion.

## Scope and original contracts

| Entry | Complete body, end exclusive | Original instructions | Operation |
| --- | --- | ---: | --- |
| `0x10239790` | `0x10239790..0x10239d10` | 352 | Restore or project an actor's condition |
| `0x10187650` | `0x10187650..0x10187680` | 12 | Read the signed condition record scalar |
| `0x10187680` | `0x10187680..0x101877dc` | 87 | Classify the live record's availability |
| `0x101878d0` | `0x101878d0..0x10187a1c` | 83 | Return effective condition, raw or as a percentage |

`actor_condition_restore.cpp` expresses scale selection, role restoration,
projected-condition interpolation, availability floors and final drift as named
operations. `actor_condition_records.cpp` owns the three record queries. The
existing spatial-role operation retains its accepted owner. Daily record refresh
at `0x10187a40` remains an explicit original dependency; its date, random and
management dependencies are outside this delivery.

Source fingerprints pin every original instruction field; complete-body,
fragment, incoming-edge and generated-hook checks reject changed or incomplete
contracts. Independent references contain all 2,184 original steps with semantic
hooks disabled. Only `page_10187.cpp` and `page_10239.cpp` change among 1,923
generated C++ files. Generation retains 1,953,334 instructions, 1,922 pages and no
missing or pending destinations. The guest State layout is unchanged.

## Preserved behavior

Restoration retains the 112-byte frame, five integer saves, saved floating
register, original call arguments and continuation addresses. Scale values above
1,000 trigger the original second live read. Signed actor flags select primary,
returning or projected restoration, including the strict 45/65 interpolation
limits. The original fixed 6,500 availability floor, 10,000 caps and unconditional
final projected drift remain intact. A null profile takes its original early
return; other positive null dereferences still fault after their original frame
and earlier effects.

The scalar query preserves its signed upper-bound check and has no added lower
bound. Availability preserves the null-record diagnostic chain and global error
flag write, the daily-cache call before reloading record fields, and the original
0/1/2 date classification. Effective condition preserves wrapped signed products,
original division, the 9,000 availability cap, minimum 100, and the signed byte
percentage flag. Invalid indices and null records retain their different original
returns. Percentage conversion reads the live four-byte divisor and uses the
original floating conversion, rounding bias, exception flags and upper cap.

All floating operations retain original source-step boundaries, memory order and
separate product/subtract rounding under `-ffp-contract=off`. Signed zero,
subnormal inputs, infinities, NaNs and enabled conversion exceptions are included.

## Acceptance evidence

Release and affected-unit UBSAN each pass **154,185 component comparisons**:
4,955 completions, 149,215 interruptions and 15 discriminating own/child negatives.
All 2,184 original source sites are covered. Comparisons include complete register
and temporary state, mapped memory, ordered effects, dependency ABI, instruction
budgets, individual missing bytes, dependency mutations and interruptions,
relocated tables and signed/flag/float boundaries. The 10,995 prior paired
structural cases remain included.

The independent reference caught an eight-byte divisor read where the original
loads four bytes. The implementation now performs the original single-precision
load. Coverage also required explicit returning-actor caps and the second
projection's availability floor; these are now exercised. Final acceptance uses
the corrected sources and complete coverage.

A **798-advance complete paired match** reaches full time with identical full
state, host import ABI and every tick's instruction accounting. The new roots
execute naturally from original callers: restoration **80**, scalar **80**,
availability **22**, effective condition **22**. Existing productive counts remain
17,728 refresh, 70,912 maximum, 117 recovery, 17,490 periodic and 13,119 consumption
calls. Dedicated counters show matching returns with no original selection,
fault, budget stop or dispatch escape in the complete match.

Actual captured caller states pass **147 production comparisons**: 90 returns,
57 budget stops and nine own negatives across direct, generated and CFM paths,
with global/per-entry original controls. Eight separate null-first-argument fault
comparisons retain complete fault state, effects and accounting. **27 separate
original-profile controls** pass.

**280 nonlocal comparisons**, including **160 restore/handler faults**, cover all
nine roots except the register-only maximum leaf, whose budget controls remain
separate. The record scalar is interrupted through the read observer; framed
roots also exercise the primary write channel. Restoration abandons saved
floating/integer state correctly; the old host continuation, its later stores
and epilogue do not run after transfer. Surviving context owners, imported handler
ABI and dedicated outcomes agree with the original references.

All four focused CTests pass in **53.03 seconds**. Nine subsystem contract/reference/
table/hook audits and 48 historical manifest checks pass. Ten affected units are
instrumented with UBSAN; generated pages and unchanged archives use the consistent
Release build. All three sanitizer runs pass with empty stderr. This is affected-unit
sanitizer coverage, not a claim that the whole engine was instrumented.

## Installed delivery and remaining scope

The signed ARM64 game executable is installed with SHA-256
`3737b26d609455d1d128b59c3eb17e295e526550c9ae3d00f28d38753dc9893a`.
Candidate and installed identities, strict deep signatures and all four original
payloads agree. The preceding 687-entry game remains a private application
reference. The installed Match Debugger is unchanged. All 845 protected original,
save and handoff files retain their hashes.

After the user unlocked the Mac, the preceding 687-entry game's deferred smoke
reached Prepare Game and exited normally. The newly installed 691-entry game also
reaches Prepare Game, presents the original exit confirmation and exits normally.
Runtime identity and normal-exit logs are recorded separately from the UI
observation; the complete-match harness establishes new-root runtime coverage.
Save/Load is **not retested**. No new timing or speedup is claimed;
accurate conversion remains the priority and the complete-port goal stays active.

Cleanup removes 6 reproducible check executables
(863,453,112 logical bytes). Objects, archives, recipes,
private references, app bundles, original inputs and receipts remain. Observed
free space after cleanup is 7,534,874,624 bytes. Receipts and
reproduction scripts remain private under `.local/actor-condition-restoration`.
The next candidate is the daily condition record refresh dependency; its private
characterization is not an implementation or acceptance claim.
