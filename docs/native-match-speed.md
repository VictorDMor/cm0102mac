# Native match speed

The user's 2026-10-20 priority is a default Turbo choice inside the original
match-speed selector and faster native processing of watched and background
fixtures. General semantic reconstruction is deferred while this performance
work proceeds. The production engine is already statically recompiled ARM64
C++23; its semantic-owner count remains 963.

## Native speed selection

The original commentary-speed menu gains a sixth item, **Turbo**, using its
existing control constructor, event handler and exact text inset. A new installation or an
installation without a native speed preference defaults to Turbo. All five
localized original choices remain available. The separate macOS Match/Turbo menu
is removed.

The preference is stored in a versioned `MatchSpeed.plist` alongside the existing
native presentation preference. Invalid or absent preferences use Turbo. Loading
an older career retains the selected native speed. Original resources and save
layouts are unchanged.

Turbo uses value 0 for presentation and the original value 20 for simulation
cadence. Its commentary wait is capped at 20 ms. Original game clocks, sound
completion, match calculations, halftime and user pauses remain in effect.
Selecting an original speed retains its original commentary duration.

The native UI also enables the existing audited runtime shortcuts by default,
independently of presentation speed. Headless reference checks retain their
explicit runtime-policy toggle.

## Shared-engine work

Host sampling identified repeated memory reads, spatial-role queries and guest
call dispatch among the largest costs. The selected optimization keeps fixed
register operations together in the hot role-table and owner-role queries, and
replaces the spatial adapter's linear entry search with direct native dispatch.
The same guest checkpoints, repeated reads, dependencies and diagnostics remain.
No spatial result is cached across calls.

Two general memory-read experiments were rejected. Outlining diagnostics alone
reduced paired wall medians by only 0.09% and 0.26% (5/10 and 8/10 faster pairs).
Forcing all reads inline changed those medians to **0.19% and 1.54% slower**.
Both preserved complete final snapshots, but neither established useful speed.
The original memory implementation is retained.

Further spatial experiments were screened privately. Moving an override object
ahead of the original archive changed native code placement and introduced
branch islands; those layouts are not delivery measurements. Three alternating
pairs with archive order preserved supported testing the localized candidate
in the complete production build.

## Final production measurement

The baseline is revision `e263d3e483f8e1593ad74401d94c0a9e82512fab`.
The final candidate links the same production archives as the app. Each scenario
starts from an immutable complete snapshot, with seed 12345, and executes 600
consecutive advances through the shared match entry `0x101d3f60`. The opponents
are Manchester United (5677); home teams are Arsenal (677) and Cruzeiro (2262).
The headless runtime policy stays disabled in both builds to isolate computation
from presentation and the existing Turbo shortcuts.

Each scenario has ten alternating baseline/candidate pairs, two warmups and five
averaged samples per process. Only one benchmark runs at a time, with no build,
game or profiler active. Snapshot cloning, serialization and comparison are
outside the timed interval. The acceptance rule was recorded before this final
measurement: at least nine faster pairs and a paired median reduction above 1%
for both wall and CPU time in each scenario, with complete state agreement.

| Scenario | Baseline wall median (range), seconds | Candidate wall median (range), seconds | Paired wall reduction | Paired CPU reduction | Faster wall / CPU pairs |
| --- | --- | --- | --- | --- | --- |
| Arsenal | 0.419728 (0.401154–0.436241) | 0.399517 (0.396806–0.418574) | 3.07% | 2.90% | 10/10 / 10/10 |
| Cruzeiro | 0.385164 (0.375604–0.397616) | 0.375735 (0.371384–0.388816) | 2.26% | 2.42% | 9/10 / 9/10 |

Reduction is the median of `1 - candidate / baseline` within each pair, not the
ratio of the two independent medians. CPU medians were 0.417433 → 0.398480 seconds
for Arsenal and 0.383823 → 0.374558 seconds for Cruzeiro. This is a modest accepted
gain on these intervals, not a full-match or all-league speedup claim.

Every repeat preserved the full final state. Baseline and candidate snapshots
are byte-identical, each 126,967,858 bytes. Arsenal retained 151,918,413 original
instructions, 2,380 bridge calls and 101,062 sampler calls; Cruzeiro retained
141,446,757 instructions, 1,958 bridge calls and 94,167 sampler calls. Both had zero
clock imports in the measured interval. Final snapshot SHA-256 values:

- Arsenal: `8d91351c565e85ad26c656693c49936457e952ff5085d66649606fe4818f3356`
- Cruzeiro: `760a47c644b0d0ab8c7af24c8e9e75d1f08e3abd7d9cfe7022ab3deb0e512eb1`

Benchmark executable SHA-256 values:

- Baseline: `ffd1f31424409a6fcdaa8e6425e56e00de1b4a233f70a6aca25cb65ed349436f`
- Candidate: `4948816d69aca205830c5be0ee8682ceffa9daea581d4e65479d1a75e7449e5c`

## Validation

Source guards pin the original menu construction, getter/setter, commentary
initialization and cadence integration sites. Missing, duplicate, unselected
and changed instructions are rejected. The existing presentation audit isolates
its bounded fixture from the later whole-caller reconstruction.

The final production candidate passed twelve focused CTests covering the native
speed choice, original presentation and scheduler, memory bounds and observers,
spatial-role component/production/nonlocal-transfer behavior, register-context
transfer, watched/background caller equivalence, shared-engine equivalence,
actual entry composition and complete-match termination. The source audits pass
5 speed, 6 presentation and 7 match-engine checks.

An isolated real-game copy visually verified Turbo selected by default, switching
to Very Fast and Slow, returning to Turbo, halftime and full time. Cruzeiro beat
Uberlandia 1–0 on 10 February 2002; the other three afternoon fixtures completed.
A separate post-match save was written through the original save dialog, then
reopened with the 1–0 result and match report intact. Selecting Slow, exiting,
reopening and entering a new match restored Slow in the original selector.

The final production candidate was then copied to a separately identified
validation bundle. Signing that bundle changes its file hash; the native
`__text` section matches the production executable exactly. Starting without a
speed preference again selected Turbo. A complete match reached halftime 2–0
and full time at minute 92, Cruzeiro 5–0 Uberlandia. Goals were Oseas 29/64/85,
Leonardo 32 and Maicon 68; Cafe's injury at minute 25 remained in the report.
The other fixtures finished Caldense 0–1 URT, Mamore 0–1 Democrata and Villa Nova
(MG) 2–1 Guarani (MG).

The original save-before-exit dialog wrote a separate post-match save. After
normal exit and reopening, the calendar still showed 10 February 2002 and 5–0,
and the match report retained the goals and injury. The app then exited normally.
The 18,987,247-byte private save has SHA-256
`68110a462ee8784577ae33afbd953e2abdc6a63c0a75c39814f9d16b61e339c7`.
These UI observations establish behavior and persistence, not a timing comparison:
the original UI match uses live timing and produced different scores across runs.

## Initial installed delivery

The initial accepted ARM64 app was installed at
`~/Applications/CM 01-02 Native Prototype.app`. Its executable SHA-256 is
`0c6858b069de7eca8745b3a0265594fcba45cefd5bee8ce0acdd9bcb15c1102e`.
The compiled executable before bundle signing has SHA-256
`9ead96a70d6af55f42c3e304d8970e8b6a2dedd6f830cf72f9547ee04bf829f2`.
All 31 native code/data sections, their addresses, lengths and flags match
between the compiled executable, the private UI validation app and the signed
delivery. The installed bundle passes strict signature verification and retains
the production launch metadata. Its original main menu and normal quit were
observed after installation; the launched process path/hash were checked and
the process was absent after exit.

The previous installed app is retained privately as a rollback reference.
All 846 protected files and the four bundled original-program resources were
verified unchanged. The installed debugger, private saves and unrelated prompts
are preserved. The semantic-owner count remains 963; this delivery changes speed
selection and native execution efficiency, not reconstruction coverage.

## Measurement boundary

Deterministic engine intervals measure the shared simulation used by watched
and background callers. They do not measure a complete career, UI waits or all
playable leagues. Near-instant processing with all leagues is a target, not an
achieved result. Further work should follow measured full-round and season costs,
without skipping fixtures, lowering simulation detail or changing random draws.

Private executables, snapshots, runtime copies and measurement records are kept
under `.local/native-match-speed`. Original pre-match save SHA-256:
`ac167b19877e04f6998301acc145d571d010c6a1caa453a58e6c6c1b1b5becdc`.

Subsequent record-conversion and passive-drawing optimizations, their matched
measurements and the updated installed identity are documented in the
[background and league-loading report](native-background-performance.md).
