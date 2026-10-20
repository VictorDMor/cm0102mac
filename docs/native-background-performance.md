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

The all-league career subsequently finished creation and was saved through the
original UI. Loading, saving a new copy and reopening that copy passed in the
native candidate. An independent daily Continue profile also shows substantial
player/recruitment/list-transition work. The forced fixture entry alone is not
the complete processing bottleneck. A later six-record capture began with its
selected low-detail fixture already calculated; its 4,214,785 instructions and
41 native calls describe postprocessing, not a full detailed-match benchmark.

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

## Passive drawing write notifications

Scalar memory writes now check a passive observer's conservative address
interval before making its virtual callback. Native drawing maintains that
interval as surfaces are registered. Writes overlapping even one surface byte
still notify drawing; replacing or removing surfaces can retain a conservative
superset. General diagnostic observers keep every scalar callback by default.
Date diagnostics and bulk-memory notifications retain their original behavior.

Two real initial match states ran 600 shared-engine advances with the actual
native drawing observer and an 8-by-8 surface outside simulation data. This
isolates irrelevant write-notification cost; it excludes UI drawing and complete
rounds. Each case used ten alternating pairs, two warmups and five measured
samples per process, with no build or profiler. All ten pairs were faster on
both clocks in both cases, meeting the declared 9/10 and greater-than-1% rule.

| Scenario | Wall median, baseline / candidate | Paired wall reduction | Paired CPU reduction |
| --- | --- | --- | --- |
| Arsenal | 0.504969 / 0.494202 seconds | 2.08% | 2.12% |
| Cruzeiro | 0.469201 / 0.459789 seconds | 2.02% | 2.06% |

Complete final snapshots, original instruction counts, bridge calls and sampler
counts match. Final snapshot SHA-256 values are
`8d91351c565e85ad26c656693c49936457e952ff5085d66649606fe4818f3356`
and `760a47c644b0d0ab8c7af24c8e9e75d1f08e3abd7d9cfe7022ab3deb0e512eb1`.
The paired replay executable hashes are
`f279c69b4847cd11ae6fccb01fa583f27c63317862949766f4aa4c95be3a1a08`
and `8d36682fa4a9492a3f3cc20c8300493ee81d8e1439621cce818f87978ad5ae63`.
These percentages are separate component measurements and are not additive.

## Validation and delivery status

Eleven focused CTests pass: record conversion and profile fallback, match speed,
productive/background/shared-engine composition, nonlocal transfer, import
descriptors and register-context production/profile behavior. Source audits
pass five new contract tests, five match-speed tests, six presentation tests,
seven compiler-arithmetic tests, and the local-temporary suite (five passing,
one optional private-export check skipped; 2,080 differential fixtures pass).
Complete generation retains all 1,953,334 original instructions, with zero
missing static or pending targets.

After adding the observer filter, all 15 focused CTests pass, including memory
bounds, drawing, arithmetic production/profile and the preceding component
checks. The filter has 256 scalar byte-interest oracle cases, high-address and
fault-before-callback checks, general/date/bulk observer preservation, and real
surface expansion, overlap and removal checks.

The candidate loaded the 26-country career, saved and reopened a new copy.
A fresh watched match from the immutable pre-match career completed as Cruzeiro
3-1 Uberlandia. All other fixtures returned results. Save, normal exit, reopen,
calendar and player-result verification passed through the original UI. Turbo
was visibly selected by default. This manual match is functional evidence only;
its events differ from an earlier run and provide no matched speed comparison.
Creating another full all-league career and completing an all-league day with
the candidate are **not retested**.

Both native optimizations are installed. The source executable SHA-256 is
`fd5103d67aa8681154541bd91efcf7e54eaa11dae4b3c2c2be66901ffbdd17f7`;
the isolated tested executable is
`0fb67da30e9f8a0bc7a38684d4df9a5f8accf3f9240c1d18b9ca46e7d24ff933`;
the installed signed executable is
`1519dbc27cf297b675ed811e446b78fde5ab70d3094f16cf842ed2d9cc8987db`.
All native Mach-O sections are identical. Only the installed executable changed;
all 846 protected files matched their immediately preceding hashes. One personal
save had already changed before candidate preparation relative to an older
historical baseline; the newer file was preserved.

The broader performance goal remains active. Near-instant loading is not
achieved. Private programs, snapshots, saved careers, profiles, transcripts and
build artifacts remain under `.local/`.
