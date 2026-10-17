# Condition range sampling reconstructed

The complete condition sampler at `0x101864a0..0x10186a18` now has a named C++23
implementation. Its **350 original instructions** extend the accepted manifest
from 696 to **697 unique entry replacements**, preserving all previous entries
exactly. The fifteen-entry attribute/condition scope covers **3,580 original
instructions**. This completes the sampler block; the full port remains unfinished.

## Scope and behavior

`native/actor_condition_sampling.cpp` separates initial/inline pool advancement,
signed small-range selection, scaled wide draws, three inlined correction levels,
recursive correction and the final upper clamp. The 128-byte frame, twelve saved
integer registers (`r20..r31`), link state, flags, floating status, source sites and
dependency arguments retain their original behavior. The implementation does not
replay generated instructions or delegate its own body to the original.

The five TOC references load before the zero-bound branch. Zero returns zero
without advancing the pool, retaining those preceding reads and frame effects.
The ordinary signed range is `[-65535,65535]`: positive bounds retain the signed
remainder; negative bounds negate it. No new absolute-value normalization or lower
clamp is introduced. Sample-plus-salt arithmetic wraps at 32 bits.

Wide draws retain signed multiplication/division arithmetic, live double constants,
stack conversion scratch, separate floating operations and `fctiwz` status. Each
correction uses its original draw, signed rounding and upper-bound clamp. The last
inlined level calls the same reconstructed entry recursively through the shared
production dispatcher. The existing random core owns reseeding at `0x1077b770`.

Initial advancement reloads the cursor and limit in its original order. Later
inline draws advance the pointer left in `r3` by the preceding draw. That cached
pointer is observable: fixtures place the cursor slot inside conversion scratch,
which overwrites the slot before the next inline advance repairs it. Base pointers,
salt accesses, wrap/reseed order and callback-visible register residues remain live
at the original boundaries. Floating contraction remains disabled.

## Independent and runtime evidence

Pinned contracts cover all 350 instructions on `page_10186`, with 30 external
indexed calls and no external interior entry. Source/reference/hook audits reject
changed or incomplete originals. Generation retains **1,953,334 instructions** on
**1,922 pages**, with no missing/pending targets. Only `page_10186.cpp` changes among
1,923 generated C++ files. The exact 696-entry manifest prefix is preserved.

Release and affected-unit UBSAN each pass **366,713 component comparisons**:
7,268 completions, 359,417 interruptions and 28 own/child negatives. The new sampler
adds **29,019 comparisons** to the preceding block. Every one of the **3,580 source
sites** is covered, including the sampler's full 350-site body. Full registers,
temporaries, mapped memory, ordered accesses, dependency ABI and accounting agree
at returns, budgets, individual missing bytes and dependency interruptions.

Fixtures include signed/extreme/zero bounds, sample and salt overflow, all pool
wrap levels, relocated TOC, aliased slots/scratch, exceptional floats, enabled
exceptions and discriminating own/reseed/recursive controls. Independent signed
remainder and zero/no-draw expectations supplement original-reference comparison.

The **36,400 structural comparisons** include preceding controls and equal
source-bound/condition mutations for deeper inline corrections. The ordinary
wrapped correction-bound arithmetic stays within the small-range interval; its
otherwise unreachable deeper wide branches are opened explicitly in both versions.
These controls prove source structure, not natural gameplay frequency.

A paired **798-advance match** reaches full time with identical state, import ABI
and per-tick accounting. The preceding ten naturally used roots retain 17,728
refresh, 70,912 maximum, 117 recovery, 17,490 periodic, 13,119 consumption, 80
restoration, 80 record-scale, 22 availability, 22 effective-condition and 13,799
daily-refresh calls. Capacity, assignment, retirement, reset and the new sampler
have **zero natural calls** in this match. Controlled next-day record writes and
the preceding actual-data lifecycle scenarios still pass.

The sampler's production evidence uses the original BL at `0x10186c34`, captured
live-world TOC/pool state and prepared small, wide, zero and pool-wrap arguments.
This is a controlled source-callsite invocation, not complete parent traversal or
a naturally occurring wide assignment. A separate structural control opens all
three deeper inline bounds and requires **two C++ calls and returns**, including
the recursive child, through direct, generated and CFM paths. Full state, effects,
imports and instruction accounting agree with the original.

These five sampler variants contribute **85 production comparisons**. Final
aggregate Release/UBSAN evidence is **351 comparisons**: 210 returns, 141 budget
stops and 21 own negatives, with global/per-entry original controls. Thirteen
separate null-first-argument faults from preceding entries still agree. The sampler's
zero argument is valid and is tested as a normal return. **63 original-profile
controls** pass in Release, including the structurally recursive variant.

**546 nonlocal comparisons**, including **312 restore/handler faults**, pass in
Release and UBSAN. The sampler contributes 42 ordinary wide-input checks and
14 structural recursive checks. Both memory channels cover its initial save,
conversion input and conversion result. Recursive controls interrupt the child's
initial multiple-register save and require both active C++ scopes to report
transfer or restore fault, with no returns, escaped owners or later continuations.
Original-context and turbo controls remain included.

All four scoped CTest targets pass. The component run and final production/profile/
transfer runs provide the counts above; the unchanged component was retained when
adding dispatcher recursion cases. Nine subsystem audits and 48 historical manifest
checks pass. Historical test edits contain only the global 696-to-697 count update.
Thirteen affected units are instrumented with UBSAN; all three final runs pass
with empty stderr. Generated pages and other archives remain consistent Release
builds. This is not a whole-engine sanitizer or speed claim.

History IO retains the preceding private warm-file/stable-fork-number fixture and
byte comparison. It applies only to retirement/reset; the new sampler uses the
captured live random state directly. Cold history file creation is not retested.

## Delivery and remaining work

The signed ARM64 game is installed with executable SHA-256
`1a90aed13a465ebdf58d3ed0d50e87faeefcb5c26072e6fa7b9b8f4ff6fe8dc3`.
Candidate/installed identity, strict deep signatures and all four original payloads
agree. The previous 696-entry app is retained privately. All **845 protected
files** and the installed Match Debugger retain their hashes. The installed game
renders **Prepare Game** and exits normally after its original confirmation.
Save/Load is **not retested**; no new performance measurement or speedup is claimed.
Historical timing reports remain unchanged.

The six reproducible linked check executables were removed after validation:
**866,335,928 logical bytes**. Objects, archives, applications,
original data, private references, snapshots, recipes and saves remain available.
The full goal stays active. Remaining queue/injury-record operations and the
larger engine still need their own complete reconstruction and acceptance.
