# Spatial-role query performance

The game now uses cheaper C++23 position and tactical-role queries while retaining
the complete 23-entry [spatial-role contract](native-match-spatial-role-reconstruction.md).
The production manifest remains at **682 entries**. No generated entry selection,
game rule, role table or query result was changed.

## Measured reason for selecting this subsystem

CPU samples from the accepted 682-entry build attributed 12.12% and 12.75% of
main-thread samples exclusively to spatial-role implementation, adapter and
diagnostic functions in Arsenal and Cruzeiro scenarios. Shared memory and
dispatcher costs were excluded from those percentages. In one 600-tick interval,
the current-position query executed 2,796,038 and 2,610,528 times respectively.
These were prioritization measurements, not a predicted speedup.

## Changes and preserved behavior

Twelve short field operations execute the same body with or without a native
continuation scope. The scope remains present whenever either memory observer
can invoke a callback. Unobserved accesses retain every original instruction
checkpoint, address wrap, width, sign extension, register residue and memory
fault. Independent read tracing still works.

Within the spatial-role subsystem, current/previous position dependencies can
call their existing adapter directly when no memory or boundary observer is
active, no original/profile mode is selected and two instruction slots remain.
All other calls keep the general dispatcher. The existing adapter continues to
own call/outcome counters. Custom dependency dispatchers retain precedence.

Role-table queries still perform each of their original position reads in order;
no value is cached across calls. Cell membership, marking relations, goalkeeper
repair and replacement rules retain their complete implementations.

## Correctness evidence

- **4,090 component comparisons per Release/UBSAN mode:** 2,216 completions,
  1,826 interruptions and 48 own/child negatives. All 23 entries are compared
  both with callback observation and without it. Added cases cover independent
  read traces, wrapped field addresses and partial-mapping faults.
- **364 production comparisons:** direct/generated/CFM calls, original controls,
  23 negatives, 55 budget stops, selected-original position children,
  boundary-only observation and three unobserved position-child memory faults.
  The sanitizer run covers the 361-case matrix plus those three focused faults.
- **69 profile controls** select original bodies.
- **196 nonlocal comparisons**, including 112 restore/handler faults, preserve
  callback and frame abandonment. Both independent memory-observer channels
  are exercised, including all five optimized field-writing operations.
- A paired complete Arsenal/Manchester United match, seed 12345, completes
  **798 advances** with identical final state, intermediate state checks,
  per-tick instruction accounting and import ABI. It executes 3,675,990 current
  position, 1,098,781 current-role and 987,076 owner-role queries. Release and
  UBSAN runs agree.

All four focused CTests and seven spatial-role audit tests pass. UBSAN instruments
the spatial implementation, adapter, diagnostics, component reference and
harnesses; existing shared archives are not newly instrumented. Sanitizer stderr
is empty. Generated code and the public `State` layout are unchanged.

## Timing

The baseline is the preserved pre-optimization library from commit
`0314d56f3cb08d334ae1c76a0e5e6a9d51bc1a06`. Separate baseline/candidate executables
avoid selected-original wrapper overhead. Each scenario uses ten measured pairs
after warmup, with five identical 600-tick runs averaged per sample and alternating
execution order. Complete 126,967,858-byte snapshots, instruction counts and
host inputs agree.

| Home team against Manchester United | Median paired runtime reduction | Faster pairs |
| --- | ---: | ---: |
| Arsenal | 5.14% | 10/10 |
| Cruzeiro | 4.14% | 10/10 |

The shorter preliminary run measured 8.09% and 10.14%; the larger confirmation
above is the delivery result. These timings cover deterministic match intervals,
not the management UI, season advancement or every match configuration.

## Delivery and limits

Only **CM 01-02 Native Prototype.app** was updated. Its signed ARM64 candidate and
installed executable have SHA-256
`8266c5e05dbe65cb783b5686323e2d6acdb1dca0149c5cf0174a0ddc1a59bf5f`.
All four original program resources agree, the previous game build is retained
privately, and all 845 protected files were unchanged through installation.

The installed Match Debugger remains at the preceding build, verified unchanged
by executable hash. Its application/UI source was not modified. Runtime evidence
for this cycle comes from the shared-engine comparisons above; the installed
game's management UI was not separately exercised. Save/Load is **not retested**.
The full native-port objective remains active.
