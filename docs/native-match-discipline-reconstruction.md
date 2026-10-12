# Match fouls and referee decisions reconstructed

## Production scope

Six complete original entries now have structured C++23 bodies: contact and
single-actor foul outcomes, referee incident response and their three match
facades. This block contains **1,078 indexed instructions**. The ordered
manifest advances from 649 to **655**, preserving the complete previous prefix.
Its canonical SHA-256 is
`f3a1af9069a320a56844c8e5a097b91ee4f3edd6eac49994d4cc726305d59877`.
The previous prefix SHA-256 remains
`0c6129f2f85ee549705905c8bf14a6300745601458170dd2172957f79ad869be`.

The game and independent debugger link the same production implementation.
Normal execution uses the complete new bodies without replaying their own
original instructions. Original selection, per-entry selection and profiling
remain diagnostic controls. Calls, returns, faults, budget stops, nonlocal
transfers, termination and dispatch escapes have separate per-entry counters.
The new metadata extends the private diagnostics object; State layout does not
change.

| Entry | End exclusive | Instructions | Role | Source SHA-256 |
|---|---|---:|---|---|
| `101f5110` | `101f5180` | 28 | Contact outcome facade | `e33c3207298656fa995e0f9cf77a8d0573d9dc544e2e88fbf9dc2c69dd0257b7` |
| `101f5180` | `101f51e0` | 24 | Single-actor outcome facade | `6ef24ee6579d01008c5cbb4dadfd5f0df5f3da506b1176a1b6b3f86827a897d0` |
| `101f51e0` | `101f5230` | 20 | Referee incident facade | `3274ce7971e69e18a96ab3de8ac3fced00f965be4b5de62615223b805decb89e` |
| `1021bec0` | `1021c254` | 229 | Single-actor foul outcome | `72b80ba67c08c1bb985561ef1371e361521cfd5279ce067fb0a704126b99f627` |
| `10221e00` | `10222290` | 292 | Contact foul outcome | `76d8fed64694d71aeb8750500fe29d0950645c18c50bb3c5caa96cd46875e8a1` |
| `10223100` | `10223894` | 485 | Referee incident response | `1b79c0902b04c8b888fd472d7fba5d6845d1a25bfdf4d442d19b0d453addbf26` |

The audit pins eight page fragments, all source fields and exterior edges.
External static entrances into a replaced interior are rejected. Independent
original references disable all semantic hooks and retain every original
checkpoint. Private listings, instruction JSON, references, runtime snapshots,
original program data and bundles are excluded from version control.

## Preserved contracts

All core methods receive the match's discipline context at `match+e030`.
Context word zero points to the match; word four caches the referee. Contact
outcomes receive actor/opponent, an in/out decision byte, reason, severity,
special-case flag, float consequence output and a ninth stack flag. Single-actor
outcomes omit the opponent and ninth flag. Referee response receives actor,
decision output, incident kind and an eligibility flag.

Match facades preserve their own 64-byte frames, argument spills, byte narrowing
and context arithmetic. The contact facade passes the ninth argument through
the original big-endian stack byte. Core frames are 128, 128 and 144 bytes.
Integer/floating registers, generated temporary residues, condition bytes,
link/count registers, FPSCR, TOC, PC, memory access order and fault prefixes are
observable and retained. Guest arithmetic remains word32 and big-endian.

The live random stream advances by two bytes, compares the live limit, wraps to
the buffer and invokes the existing reseed dependency when necessary. Seed,
float consequence constants and wide conversion constants remain live TOC
reads. Booking rating changes use the original clock conversion, single-
precision rounding and float-to-integer boundaries. Compiler-emitted wide-count
sampling and clamp branches are retained even where ordinary byte fields cannot
produce their inputs.

Contact processing distinguishes unavailable actors, decision seven, invalid
phase/decision, special incidents and regular severity thresholds. It preserves
team fouls, actor fouls, opponent fouls suffered, rating effects, yellow count,
first/second reasons and the optional actor flag. Single-actor processing records
team/player fouls before its decision/phase guards and keeps its different
special, direct-red and booking thresholds. Its regular direct-red branch does
not acquire a reason store absent from the original.

Referee response combines sampled prior fouls, the cached referee's strictness,
incident severity, caution toward already booked players, direct-red eligibility
and booking eligibility. It preserves the duplicate original branch checkpoints,
signed narrow counters and reason-only updates for some unavailable actors.
The original unguarded referee access in the already-booked path remains an
unmapped-access fault when the cached referee is null.

These outcome methods decide a sanction; they do not themselves perform the
later dismissal or increment the red/dismissal counter. Existing original callers
and the previously delivered dismissal operation own those consequences.
The [debugger card controls](native-match-debugger-cards.md) use this same contact
outcome method, then execute commentary/dismissal and retain their history and
pitch re-entry controls.

## Focused validation

Hook-free original component comparisons cover **3,337 cases**: 1,604 returns,
1,724 interrupted outcomes and nine discriminating own/child negatives. Every
entry returns normally and has its own negative; each facade also rejects an
altered child. Comparisons include complete architectural/temporary state,
all mapped memory and allocator/import metadata, ordered reads/writes,
dependency arguments/returns and instruction accounting.

The field and boundary matrix reaches **917 source sites**. Ten explicitly
labelled diagnostic register perturbations exercise unreachable wide/zero-count
compiler fallbacks; combined characterization reaches all **1,078 source sites**.
Those diagnostics are not claimed as natural match coverage. Cases include
severity/sample boundaries, phases/decisions, first and second yellow, direct
red, signed counts, ratings before/after 45 minutes, special incidents, referee
fields, inactive/flagged actors, refill, relocated TOC and representative aliases.
Each entry's normal path is interrupted at every instruction budget and every
actually accessed byte hole. Independent expectations check card/foul outputs,
rating changes and the absence of a premature dismissal.

Six focused CTests pass: component equivalence, actual production, original
profiling, nonlocal transfer, debugger cards and debugger completion. Production
comparison provides **72 cases** (60 returns, 12 budget stops, six own negatives),
with another **18 original-profile cases**. Every entry uses direct, generated
and CFM paths, global-original and per-entry-original controls. Integrated
captures come from real match ticks for contact/single-actor outcomes and from
one directed invocation of the complete original incident caller `10288230` for
referee response. Its real match actor and actual nested facade/core ABI are
captured, rather than synthesizing child arguments. That incident invocation is
not claimed as a naturally occurring match event. Each entry records a dedicated
productive call and return, with no selected original, fault or dispatch escape.

Nonlocal validation provides **42 comparisons**, including 24 restore/handler
faults, all six live framed scopes, original/semantic register-context dependencies
and a turbo control. Interrupted frames and callbacks are abandoned; the surviving
caller receives the original result without finishing an interrupted return.
Complete effects, architectural state and import boundaries are compared.

Seven discipline audits and **39 changed manifest-only assertions** pass.
Audited generation retains 1,953,334 instructions, 1,922 pages and zero
missing/pending targets. Only four of the 1,923 generated C++ files change:
`page_101f5.cpp`, `page_1021b.cpp`, `page_10221.cpp`, `page_10223.cpp`.
The reusable build and all other generated units remain available.

UBSAN instruments the new semantic bodies/adapter, private diagnostics and all
three affected harnesses, including the independent original component reference.
Component, actual production and nonlocal runs pass with empty sanitizer stderr.
Unchanged archive dependencies are not newly instrumented.

## Installed acceptance and boundaries

Both strict ad-hoc signed ARM64 applications are installed through
metadata-preserving APFS clones and atomic exchanges. Prior signed bundles are
retained privately. Candidate and installed executable hashes agree:

- Game: `b40fb1fc53c6f7490d76ebe2c3f937bedb48e9eab550788cfaba4974714d4941`.
- Debugger: `50f6f17601199c1de875c81ffe7f84e9907c351e002ed5fce04a061bf433e26b`.

All four packaged original-program files in each application are verified. The
845 protected game/runtime/save/handoff files agree before and after installation.
The current Reading save remains intact. Running GUI/worker identities map the
installed executables and match the signed candidates.

The installed debugger's real worker passes card add/remove, first/second yellow,
direct red, dismissal/re-entry, malformed controls, undo/redo/time travel and
following productive movement, then exits with empty stderr and removes its own
backing. A short AppKit check observes real club/player names, the 00:05 clock,
Thierry Henry's yellow count changing 0 -> 1 -> 0, the original card event and
its cancellation while preserving selection. The game renders its original
800x600 menu and exits through the original confirmation. An accessibility read
after that exit reopened an owned game session; it was also quit normally.
All owned GUI/worker processes exit and debugger backing is removed.

The full match engine and whole port remain unfinished. Outcome decisions do
not establish complete career/persistence coverage. Save/Load is **not retested**;
performance is **not benchmarked**. No in-game goal buttons are implemented: the
user's question about that feature was theoretical. Work stops at this delivery
for the user's requested goal pause; no next subsystem is started.

The accepted cycle removes 31 validated owned disposable artifacts, recovering
2.11 GB. Reusable production objects/libraries, generated sources, signed apps,
prior signed references and private evidence remain available. All 845 protected
files, references and installed/candidate identities are rechecked after cleanup.
