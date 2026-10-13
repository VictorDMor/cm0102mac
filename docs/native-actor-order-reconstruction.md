# Actor processing order reconstructed

Five complete operations replace generated bodies with named C++23 functions:
the active-actor queue builder, its comparison callback and three values used
by the comparison. The block contains 202 original instructions. The complete
port remains an active objective; the generic sorting dependency remains
separate from these football operations.

## Selection

A fresh native CPU sample of the preceding accepted executable contains 10,160
main-thread samples, including 9,681 beneath shared match progression. Generated
page `101d9`, which contains this queue and comparator, contributes 214 leaf
samples, the highest generated-page leaf count in this measurement. This is
page-level selection evidence, not an exact CPU percentage for the five entries.

The sample also identifies heavier existing C++ spatial queries. An experiment
specializing those 23 entries alone passes their component comparison but gives
mixed paired timings; it is discarded. Their public source remains unchanged.

| Operation | Entry | Original instructions |
|---|---|---:|
| Secondary ordering byte | `101d5670` | 2 |
| Primary ordering byte | `101d56b0` | 2 |
| Build active processing queue | `101d9080` | 73 |
| Compare queued actors | `101d91b0` | 123 |
| Current fitness value | `101d93a0` | 2 |

## Preserved behavior

The queue builder scans twenty slots on each team. For every slot it draws a
side from the existing range sampler, inspects that side first, then inspects
the other side. A nonnegative signed position byte admits an actor. The live
byte count and pointer array are updated in original order; the implementation
does not clamp positions or cache a count across dependency calls. After the
scan, the match pointer is stored in the original callback-context slot and
the existing sort is called with four-byte elements and the live comparator
descriptor. Even an empty queue reaches that dependency.

The comparator reloads its context at each original access. A nonzero match
interruption byte enables the first priority pointer; the second priority
pointer is checked afterward. Each tier checks the left operand first. Equal
actor pointers can therefore return minus one when that actor has priority.
This is not replaced with a host strict-order comparator or a different sort.

Ordinary comparison uses both signed ordering bytes, live single-precision
fitness, the signed halfword modifier and the live constants table. All
single/double rounding stages, integer conversion, floating status flags and
wrapping score subtraction are retained. The byte getters themselves return
unsigned values; narrowing belongs to their callers. The fitness getter
promotes the stored single-precision value to the original floating return
register. NaNs and exceptional conversion retain the established original
floating-point policy.

The queue frame is 80 bytes; the comparator frame is 144 bytes with its
original floating and integer saves. The three value queries are unframed.
Spills, alias-sensitive reloads, scratch registers, LR/TOC/PC, source order,
instruction charges and fault prefixes remain observable. No normal body
replays its original instructions. Dedicated diagnostics, original/profile
controls and generated destination hooks remain available for each entry.

## Component evidence

The independent original reference covers all 202 source sites. **3,529
comparisons pass:** 1,163 normal returns, 2,359 interruptions and seven
discriminating own/dependency negatives. Complete architectural state, mapped
memory, ordered effects, dependency ABI, source order and accounting agree.

The matrix checks the independently expected queue order and weighted result,
empty/full eligibility, all byte values, signed narrowing, unusual sampler
results, count changes during callbacks, reversed sort effects, changed fitness,
live priority-context retargeting, equal-priority pointers, signed modifiers,
nonfinite values/constants, stack/table/argument aliases, relocated TOC,
unaligned/null pointers, every budget stop and every accessed-byte hole on
representative paths.

## Shared-engine evidence

All five entries are captured naturally from shared match progression. The first
capture interval records 240 secondary-byte calls, 240 primary-byte calls, one
queue build, 138 comparisons and 262 fitness queries, each with a normal return.
The production matrix passes **60 comparisons**, including 50 returns, ten budget
stops and five discriminating negatives. Each entry runs through direct,
generated-destination and CFM descriptor paths, with global and per-entry original
controls. Fifteen additional profile-mode comparisons prove original selection.
Complete state, ordered memory effects, instruction charges and import boundaries
agree; no body-scope residue or dispatch escape remains.

Forty-two nonlocal comparisons include 24 restore/handler faults. Every root is
interrupted, and the queue builder also reaches the actual original sort and its
nested comparison callback. Both original and modern context restoration abandon
the interrupted frames and callback continuations without executing their old
epilogues. Original-context accounting agrees exactly; the already-reconstructed
context dependency is accounted for separately in its modern mode.

Five focused CTests pass, including the full-time shared-engine regression:
100 Arsenal/Manchester United ticks and a Cruzeiro/Manchester United match with
789 ticks, full time at 5,558 seconds, equal original/C++23 final state and
instruction counts, fault negatives, undo/redo and completed-step rejection.
Seven boundary audits and 43 updated historical manifest assertions pass. UBSAN
instruments the two new implementation units, diagnostics, all three harnesses
and the independent reference; all three checks pass with empty stderr. Unchanged
linked archives are not sanitizer-instrumented.

Complete generation retains 1,953,334 instructions and 1,922 pages, with no
missing/pending targets. Only `page_101d5.cpp` and `page_101d9.cpp` change among
the 1,923 generated translation units. The manifest grows from 663 to **668**
entries, preserving its exact earlier prefix. Its SHA-256 is
`1f8f45083830a2840b62cbdcfdfa217a77a43a51905cea324893c5a269827bf2`.

## Observed use

Separate 600-tick intervals, both against Manchester United with seed 12345,
record these productive entry counts. They establish execution frequency, not
per-function time or whole-port completion.

| Entry | Arsenal | Cruzeiro |
|---|---:|---:|
| Secondary byte `101d5670` | 161,183 | 158,746 |
| Primary byte `101d56b0` | 160,122 | 157,886 |
| Queue `101d9080` | 597 | 594 |
| Comparator `101d91b0` | 78,676 | 77,647 |
| Fitness `101d93a0` | 161,449 | 159,165 |

## Paired performance

The accepted `2af504a` implementation is the baseline. Twenty measured pairs
compare 600 shared-engine ticks, with one warm-up per matchup and alternating
baseline/candidate order. Snapshot restoration and cloning are outside the timed
interval. No profiler, memory observer, build or concurrent test runs during the
measurement. The deterministic State-only host bridge and instruction/import
accounting are identical.

| Home team | Baseline median | C++23 median | Median paired time reduction |
|---|---:|---:|---:|
| Arsenal | 0.404877 s | 0.394479 s | 2.57% |
| Cruzeiro | 0.378173 s | 0.369488 s | 2.12% |

All twenty measured candidate intervals are faster. Each matchup's complete
126,967,858-byte final snapshot agrees across executables, as do repeat snapshots,
instruction budgets, sampler counts and import counts. Both benchmark processes
exit successfully with empty stderr. This result measures these two simulation
intervals on this machine; it is not a whole-game or interface-speed claim. The
earlier dispatch-environment optimization's approximately 31.6% reduction is a
separate result and is not attributed to these five replacements.

## Installed products

Both Release ARM64 apps are signed and installed by verified atomic exchange,
with the preceding signed apps retained privately. All four original resource
files match their protected inputs. The installed debugger passes its worker
protocol, independent-state, interventions and timeline checks. Its actual
Cruzeiro/Manchester United match completes after 789 steps, 1–2 at 92:38; completed
step rejection, undo/redo and normal backing cleanup pass.

The installed game reaches its visible original menu and exits through the
original confirmation screen. The mapped process executable matches the installed
candidate; its log confirms a normal original-engine exit. This UI check is not
per-entry match coverage; the natural shared-engine captures above provide that.
Save/Load is **not retested**.

| Installed app | Executable SHA-256 |
|---|---|
| CM 01-02 Native Prototype.app | `9ba9abc99fe8bbc4fe4c3121fdc74d6197904ecca3f81a1bc15907db5c436be8` |
| CM Match Debugger.app | `6bdfdb6d2aa0b1cf366e5bf18f58cad04336e535a2f2065c9674ec9a6aa87498` |

The cycle cleanup removes 21 owned reproducible outputs/backing paths, totaling
1,861,042,860 logical bytes. All 845 protected files, the preceding signed apps,
the benchmark baseline, current app bundles, reusable libraries/objects, private
inputs, snapshots, sources and logs remain intact. Installed/candidate identity
is verified again after cleanup. Private evidence is retained under
`.local/actor-order-reconstruction/`; no original payload or runtime snapshot is
published with this report.
