# Condition record capacity and lifecycle reconstructed

This report records the preceding 696-entry delivery. The subsequent
[condition range sampler](native-condition-biased-sampling.md) extends the component
to fifteen entries and the accepted manifest to 697.

Four complete operations now have named C++23 implementations. Their **388
original instructions** extend the accepted manifest from 692 to **696 unique
entry replacements**, preserving the previous 692 entries exactly. The combined
fourteen-entry attribute/condition component covers **3,230 original instructions**.
This completes a bounded functional block; the full game port remains unfinished.

## Scope and behavior

| Operation | Complete original range | Instructions | Frame |
| --- | --- | ---: | ---: |
| Raw or percentage capacity readout | `0x101877e0..0x101878d0` | 60 | 96 bytes |
| Capacity assignment and derived scalar adjustment | `0x10186a20..0x10186cd0` | 172 | 112 bytes |
| Active injury retirement and archival request | `0x1018b9f0..0x1018bb4c` | 87 | 128 bytes |
| Queue removal and condition record reset | `0x10186ea0..0x10186fb4` | 69 | 80 bytes |

`native/actor_condition_lifecycle.cpp` separates capacity conversion, depleted
scalar adjustment, random scalar assignment, retirement diagnostics/notification
and queued-update removal. The functions preserve original guest registers,
frames, flags, floating status, dependency arguments and continuation addresses.
They do not replay generated bodies. Guest State layout is unchanged; productive
counters remain private diagnostics.

Capacity queries retain nullable staff behavior, the original signed upper-index
guard, invalid-index fallback and live refresh before reading the record. Raw
capacity is a signed halfword. Percentage conversion keeps separate single
rounding, division and truncating conversion; it adds no rounding bias or 100 cap.

Assignment writes the capacity halfword before later profile reads or faults.
Derived scalar work retains signed traits, thresholds, depleted floating-point
adjustment, random pool advance/wrap/reseed, signed remainder and explicit biased
sampling dependency. Source-PC comparison caught and corrected the nonzero-draw
continuation at `0x10186b48` before acceptance. Floating contraction stays disabled.

Retirement preserves the null-staff diagnostic path, upper-index guard and `-1`
no-op. Injury type `0x51` skips archival. Optional club eligibility and notification
remain ordered dependencies. The type byte becomes `0xff` before the live staff
index is reloaded and its active injury slot becomes `-1`.

Reset removes every matching queued update while keeping unrelated entries,
retires any active injury, clears its history through the original dependency and
writes date/default/sentinel fields in original order. Only bit `0x80` of the
corresponding statistics flag is cleared. All 256 flag values have an independent
`flag & 0x7f` expectation.

The injury registrar (`0x10188cc0`), archive writer (`0x10748ce0`), notification
(`0x1018dcb0`), club eligibility (`0x1014c2e0`), queue iterator/removal
(`0x10738e00`, `0x10738e60`), history clear (`0x1018fdf0`) and biased sampler
(`0x101864a0`) remain explicit dependencies. Their execution in these checks does
not claim their own semantic reconstruction.

## Independent and production evidence

Pinned contracts retain all original instructions, page fragments and incoming
edges. Independent references disable semantic hooks. Every external indexed
incoming edge targets an entry. Generation retains **1,953,334 instructions**,
**1,922 pages**, no missing/pending targets and the exact 692-entry prefix. Only
`page_10186.cpp`, `page_10187.cpp` and `page_1018b.cpp` change among the 1,923 generated
C++ files; original bodies and unrelated hooks remain present.

Release and affected-unit UBSAN each pass **337,694 component comparisons**:
6,712 completions, 330,957 interruptions and 25 discriminating own/child negatives.
Every one of the **3,230 source sites** is covered. Full registers, generated
temporaries, mapped memory, ordered access, dependency ABI and instruction
accounting agree at returns, budgets, individual missing bytes and dependency
interruptions. The fixtures include signed/null bounds, all reset flags, duplicate
queue entries, live pointer/index mutation, retirement notification eligibility,
random wraps and exceptional floating-point values with enabled exceptions.

The **24,540 structural comparisons** include the previously documented branch
controls and explicit wide-bound assignment controls after the original signed
halfword invariant. These equal original/C++ source-site mutations exercise
otherwise unreachable source branches; they are not natural gameplay evidence.

A complete paired **798-advance match** reaches full time with identical full
state, host imports and every tick's accounting. The previous ten roots retain
17,728 refresh, 70,912 maximum, 117 recovery, 17,490 periodic, 13,119 consumption,
80 restoration, 80 record-scale, 22 availability, 22 effective-condition and
13,799 daily-refresh calls. **The four new roots have zero natural calls in this
match.** Their runtime evidence is the separate controlled actual-data scenario.
The controlled next-day scenario still completes daily record writes.

Four original branch-and-link sites (`0x101309d4`, `0x101309f8`, `0x10186f28`,
`0x102e4720`) invoke the new C++ roots using actual player/controller objects and
original remaining callees. Preparation uses original reset/assignment, actual
injury registration and an original queue with duplicate matching entries plus
an unrelated entry. Injury registration's second argument is its type index;
its ninth argument uses the original caller stack slot. These are deliberately
prepared source-callsite scenarios, not a natural injury event or full caller
function traversal.

Caller captures, percentage variation and the preceding scenarios pass **266
production comparisons** per Release/UBSAN mode: 160 returns, 106 budget stops and
16 own negatives across direct, generated and CFM paths with global/per-entry
original controls. Thirteen separate null-first-argument faults agree. **48
original-profile controls** pass in Release.

History tests use private files, with original bytes restored in place between
fresh platform instances so their opaque native filesystem identities agree.
An original probe discovers missing history filenames; the compared runs start
with those files pre-existing and empty. Carbon assigns different process-global
fork numbers on successive opens, so an explicit test-only host-input fixture
supplies stable guest fork numbers while performing actual native reads/writes.
It preserves guest memory observation and compares full import ABI, ordered
effects and final file bytes. This proves the warm-file history path under those
controlled host inputs; cold file creation is not claimed by this cycle.

**490 nonlocal comparisons**, including **280 restore/handler faults**, pass in
Release and UBSAN. Both memory channels interrupt each new root before its frame,
in its body and near its final write. Existing integer/floating saves, original
context and turbo controls remain. Reset also proves completed nested retirement
before a later parent transfer. Abandoned continuations and later writes do not
resume after transfer.

All four focused CTests pass in **95.84 seconds**. Nine subsystem audits and 48
historical manifest checks pass; the latter changes contain only the global
692-to-696 count updates. Twelve affected units are instrumented with UBSAN;
component, production and transfer runs pass with empty stderr. Other archives
and generated pages remain consistent Release builds. This is not a whole-engine
sanitizer or performance claim.

## Delivery and remaining work

The signed ARM64 game is installed with executable SHA-256
`f627cef77acc0940c73a68bc77bcdff579c2c744ef3b07fc505310af3a3054ef`.
Candidate/installed hashes, strict deep signatures and the four original payloads
agree. The previous 692-entry game is retained privately. All **845 protected
files** and the installed Match Debugger retain their hashes. The installed game
renders **Prepare Game** and exits normally after the original confirmation.
Save/Load is **not retested**. No speed benchmark is added; historical timing
reports retain their original measurements.

The six reproducible linked check executables were removed after validation:
**865,657,288 logical bytes**. Build objects, archives,
applications, original inputs, private references, snapshots, recipes and saves
remain available. Each paired IO fixture removes only its own temporary copy.

The full goal stays active. The biased sampler and remaining queue/injury record
operations are candidates for the next coherent block; they require their own
original contracts and runtime acceptance.
