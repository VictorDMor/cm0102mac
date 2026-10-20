# Native background processing and league loading

Status: active investigation. Turbo is already a native default choice in the
original match-speed selector; see [the installed delivery](native-match-speed.md).
The immediate priority now includes post-match background fixtures and league
loading/new-career creation. General semantic reconstruction remains deferred.

The game is already compiled as native ARM64 C++23. Many routines still express
the original register operations, so a language-standard change alone does not
remove their computational costs. This work changes native execution while
retaining simulation detail, random-number consumption and the save format.

## Whole-game observations

The original background entry `0x101cd5c0` already runs the selected pending
fixture to completion when called in forced mode. Its original one-in-three and
one-in-six scheduling gates apply to incremental mode. Raising that forced flag
again would not establish a performance improvement.

An isolated single-league match reached the final forced call with no pending
detailed fixture: the other fixtures had completed during the watched match and
user-interface pauses. That call took approximately seven microseconds and is
not a useful background-queue benchmark.

A second isolated career selected all playable leagues and the Brazil 2002
start. Creation remained CPU-bound for more than 40 minutes. Its profile changes
between record/attribute preparation, sorting and path resolution, and shared
match simulation. This is diagnostic evidence, not a clean elapsed-time
comparison: profiling, builds and a documented suspension for isolated timing
overlapped the run.

That run captured an actual forced-call manager with 16 detailed fixture records.
A standalone replay cannot yet reproduce the complete host context: post-match
processing also accesses the original live window and filesystem identities.
The incomplete replay is rejected as timing evidence. No matches or native
effects are skipped to make it finish.

## Bounded record conversion optimization

Four audited record-field leaves now have generated native scalar-register
versions. Values remain in host integer variables across the complete leaf;
the compiler can eliminate repeated register byte conversions. Memory reads and
writes retain their original order and bounds checks. Stack unwinding publishes
the exact registers, program counter and instruction count even after a fault.

Specialization requires an unlimited instruction budget, safe counter headroom,
and no tracing or active state-observing diagnostics. The final native drawing
observer is eligible because its write callbacks do not inspect or reenter the
guest state. Finite execution, original-PC profiling and other observers retain
the original generated body. Hooks cover actual entry labels, including
intra-page branches; one leaf spans two original code pages.

The four complete instruction contracts are fingerprinted before generation.
Changed, missing, duplicate or unselected instructions fail generation. The
semantic-owner count remains 963; this is a compiler execution optimization.

The initial byte-array shadow-register prototype preserved behavior but was
approximately 2.4 times slower in an isolated leaf test and was rejected. The
scalar implementation passed 1,920 complete-state comparisons, including 880
matching fault cases, aliases, partial writes and native drawing updates.

## Measurement boundary

The measured world operation is the original `0x100c2520` attribute normalization
over a freshly loaded, resolved database containing 153,796 staff and 11,019
clubs. The original loader and normal original dependencies prepare that world;
no saved career is edited. A private capture retains complete initial and final
states. Every timed run must reproduce all final bytes and the same
1,711,959,706 original instructions. This operation makes no native OS calls.

This is a world-processing subsystem measurement. It excludes new-career UI,
league/calendar setup, disk latency, complete rounds and season progression.
The separate raw-database loader screening showed no useful gain from these
four leaves. Near-instant all-league loading and a comparison against Windows
patches remain unproved.

Six alternating baseline/candidate pairs use one warmup and two measured samples
per process, with no build or profiler active. The long-running diagnostic app
was suspended for this interval and resumed afterward; its process CPU time
remained unchanged throughout the timing runs.

| Clock | Baseline median (range), seconds | Candidate median (range), seconds | Median paired reduction | Faster pairs |
| --- | --- | --- | --- | --- |
| Wall | 1.979920 (1.972131–1.989437) | 1.812573 (1.804508–1.833413) | 8.46% | 6/6 |
| Process CPU | 1.964081 (1.957866–1.967743) | 1.797455 (1.780695–1.801167) | 8.53% | 6/6 |

Every final snapshot has SHA-256
`8d6dc447e8fea27dfebf9f17f6cae2c6f504e098ac545747c64d4fe7f913d17f`.
The baseline replay executable is
`56cbbcdfd6b37f6a05857e61ae56f66f7027b8e75327078d660a06a08487f02d`;
the candidate linked against the production archive is
`38352ef7fe2da3a2c77527da2be2c94f8bbc0a4abf15461bfd1d94e92f99b289`.
Both meet the predeclared threshold of at least five faster pairs and more than
1% median paired reduction for both clocks.

## Validation and delivery status

Eleven focused CTests pass: record conversion and profile fallback, match speed,
productive/background/shared-engine composition, nonlocal transfer, import
descriptors and register-context production/profile behavior. Source audits
pass five new contract tests, five match-speed tests, six presentation tests,
seven compiler-arithmetic tests, and the local-temporary suite (five passing,
one optional private-export check skipped; 2,080 differential fixtures pass).
Complete generation retains all 1,953,334 original instructions, with zero
missing static or pending targets.

The built app executable is
`10d1c4bd8a0b118dc61927ade614f92181992c1cccc53e9122cd7ad32d15954d`.
It is **not installed**. The installed Turbo delivery remains
`0c6858b069de7eca8745b3a0265594fcba45cefd5bee8ce0acdd9bcb15c1102e`.
Live candidate new-career, whole-round and persistence checks are **not retested**.
The broader performance goal remains active. Private programs, snapshots, saved
careers, profiles, transcripts and build artifacts remain under `.local/`.
