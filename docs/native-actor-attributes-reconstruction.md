# Actor action attributes and condition recovery

The latest [injury-selection delivery](native-injury-selection.md) extends this
component to twenty-two entries and the accepted manifest to 704. The subsequent
[injury-registration block](native-injury-record-registration.md) has a separate
owner and extends the manifest to 707 while retaining these twenty-two entries.

This report records the preceding 685-entry delivery. The subsequent
[periodic condition and consumption block](native-actor-condition-reconstruction.md)
extends the same owner to five entries and 687 total replacements; its separate
report contains current installation and combined acceptance evidence.

The complete actor action-attribute refresh (`10235ea0..102373a0`), signed
maximum (`102373a0..102373b4`) and condition recovery (`10235db0..10235e9c`)
now have named C++23 implementations. They cover **1,408 indexed original
instructions**. All three roots are selected in the shared game engine; the
**685-entry manifest** preserves its preceding 682 entries and the two earlier
attribute candidate entries in order and ownership.

The resumed whole-port goal follows the committed conversion-first discipline:
accurate reconstruction and technical acceptance determine integration;
optimization is deferred. The earlier two-root candidate was withheld because
of a small timing regression. Those measurements remain historical evidence
below; this delivery makes no speedup claim.

## Scope and selection

Fresh CPU samples of the accepted spatial-role build (`b7f91c6`) highlighted
the still-generated pages containing this operation in both Arsenal and
Cruzeiro scenarios. Each profiling run executes 70 restored 600-tick intervals.
Page samples are selection evidence, not a measured cost attributable entirely
to this root. Paired timing is reported separately below.

The original index contains two external calls to the refresh root, from
`1022e380` and `102393ec`, and four calls from refresh to the maximum helper.
There are no indexed external entrances into either body interior, including
fall-through edges. Condition recovery is included in this resumed delivery; its
two indexed callers are `10277a54` and `10277ce4`, with no external interior
entrances. The separate sampler at `10226520` and random refresh at `1077b770`
remain explicit dependencies.

## Original behavior retained

Refresh writes ten transient action bytes from the actor's baseline bytes:

| Baseline | Destination | Sampling |
|---|---|---|
| `+111` | `+107` | Maximum of two condition-adjusted draws |
| `+113` | `+10d` | Maximum of two draws |
| `+114` | `+10a` | Single draw |
| `+115` | `+10b` | Single draw |
| `+116` | `+10c` | Single draw |
| `+117` | `+10e` | Maximum of two draws |
| `+112` | `+109` | Maximum of two draws |
| `+118` | `+108` | Single draw |
| `+119` | `+110` | Single draw |
| `+11a` | `+10f` | Single draw |

Offsets are hexadecimal. The first pair derives its signed bound from the live
condition float at `+50`, the live single-precision divisor and the original
integer conversion/narrowing. Its second draw reloads both the condition and
baseline. Other paired traits also reload their baseline between draws.

Zero bounds consume no pool word. Nonzero bounds advance the shared cursor by
two, compare against the captured limit slot and wrap/reseed when required.
Negative small bounds retain the original signed remainder/sign adjustment.
The original wide-bound paths retain live double constants, stack conversion
scratch, the separate correction sampler and upper clamp. Although ordinary
byte-derived bounds cannot reach those paths, they remain present and are
tested with paired source-boundary mutations rather than counted as natural
match coverage.

Named condition adjustment, paired/single trait updates, pool advancement and
wide sampling retain all source checkpoints. The 128-byte frame, saved
registers, volatile residues, LR/TOC, conditions, floating-point flags, ordered
reads/writes, faults and instruction charges agree with isolated original
references. The maximum helper preserves its signed comparison and register
effects, including equal operands.

## Condition recovery contract

Recovery writes action marker 103 at actor `+19c`, sets the byte at `+2c` to one
only if it was zero, and advances the shared random pool once. The live limit,
wrap/reseed result and salt retain original ordering. A signed remainder modulo
five plus five is converted through the original double scratch, narrowed during
the single-precision subtraction, then added to live condition at `+50` with a
separate single-precision addition and store. Recovery itself does not clamp
condition; the later periodic routine has separate bounds in internal units.
The reseed callback can change condition before that later load.

The complete 80-byte frame, three individual integer saves, volatile register
residues, floating flags, memory effects and fault prefixes are retained. The
new body extends the existing adapter and private diagnostics. Source fingerprints,
page fragments and isolated references cover all 59 added original instructions;
the incoming-edge audit uses the extrema of every contract, independent of
manifest ordering.

## Component evidence

Release and UBSAN each pass **35,404 comparisons**: 3,167 normal completions,
32,232 interruptions and five discriminating own/helper negatives. Of these,
10,995 are paired structural cases using an explicitly injected wide bound.
All 1,408 original source sites are reached. Every nonnegative comparison checks
the full architectural and temporary state, mapped memory, allocator/import
state, ordered memory effects, dependency ABI, source order and accounting.

The refresh matrix includes all 256 encodings of each baseline byte; condition and
constant IEEE edge values; enabled floating exceptions; positive/negative wide
bounds and correction extremes; every wrap position among the fourteen draws;
dependency mutations and interruptions; aliased slots, actor/stack overlaps,
relocated TOC; every instruction budget and every accessed byte hole along
representative paths. Independent expectations check ordinary output bytes,
pool advancement, zero consumption and signed maximum results.

Recovery additionally covers initially zero/nonzero flags, signed remainder and
word32 carry extremes, single-precision IEEE edge values, enabled exception
flags, pool-limit and wrap positions, callback mutation/interruption, aliased
slots and actor/stack overlaps, relocated TOC, every instruction budget and every
accessed byte hole on representative paths. Independent assertions establish
its output marker, flag preservation, one-word consumption and numeric increment.

Eight Python audits pass for source fingerprints, boundaries, incomplete or
mutated records, isolated references and all page fragments, including destination hooks before original instruction
accounting. The focused component CTest passes. UBSAN instruments the body,
adapter, private diagnostics, component/production/transfer checkers and
independent original references; unchanged linked libraries and generated
production pages are not newly instrumented. Sanitizer
stderr is empty. Private inputs, generated references and receipts stay under
`.local/actor-attributes-reconstruction`.

## Production and complete-match acceptance

Natural engine callers capture all three roots without synthetic argument setup.
A paired Arsenal/Manchester United match, seed 12345, reaches full time after
**798 advances** with 17,728 refresh calls, 70,912 maximum calls and **117 recovery
calls**. Every tick agrees in return and instruction accounting; intermediate
and final complete states, host inputs and import ABI also agree. Dedicated
normal call/return counts agree for every root, with zero original selection,
faults, budget stops or dispatch escapes on that complete match.

Release and UBSAN each pass **48 actual-production comparisons**, plus two
explicit null-actor faults after the respective frame setup. The matrix covers
direct, generated and CFM descriptor entry paths, global/per-entry original
selection, three negative controls and eighteen budget stops. Nine original-profile
controls pass separately in Release. All 48 affected historical manifest checks
pass; the eight subsystem audits retain their body/hook/reference coverage.

Both modes also pass **84 nonlocal comparisons**, including 48 context-restore
or resumed-handler faults. Refresh interruptions occur before the first draw,
after a maximum child returns, and at the final attribute store. Recovery is
interrupted at its initial action store, conversion scratch store and final
condition store. Both memory observer channels are exercised. The primary
channel's first refresh point uses a scratch write because it does not report
reads. The maximum leaf has no memory or external call boundary inside its five
instructions; its complete instruction-budget matrix is checked separately.
Abandoned frames execute no later stores, epilogues or callback continuations.
Completed child counts remain exact. Original/modern context dependencies and
turbo controls are included.

Four focused CTests pass in 20.14 seconds. Seven affected units are separately
instrumented with UBSAN, and all three linked comparison binaries pass with
empty sanitizer stderr. Generated pages and unchanged linked archives retain
the consistent Release build. Historical fixture corrections remain recorded in
the earlier candidate commit; none changed this delivery's production behavior.

Generation retains all 1,953,334 instructions, 1,922 pages and zero missing or
pending targets. Only `page_10235.cpp` and `page_10237.cpp` differ among the
1,923 generated C++ files. Full-body/page fingerprints and all indexed incoming
edges remain audited. State layout is unchanged; diagnostics are private.

## Historical performance experiment

The earlier two-root integrated candidate was slower by median 7.67% and 2.71%
in the two 600-tick scenarios. The final candidate exposes constant instruction sites and
register selections across named phases, specializes the absent-observer path,
and routes unobserved, unprofiled maximum calls with sufficient budget directly
through their existing adapter. Original controls, counters and interrupted-call
handling remain intact. Release and UBSAN acceptance was repeated after these
changes. These adjustments do not establish a performance gain.

The earlier final timing compares the actual pre-integration library with that
candidate in alternating order: ten measured groups of five runs per mode, after warm-up,
for each 600-tick scenario. The median paired runtime increase is **2.11% for
Arsenal and 1.18% for Cruzeiro**. The candidate is faster in only 1/10 and 3/10
groups respectively. Full 126,967,858-byte snapshots, instruction accounting,
clock requests, bridge calls and sampler calls agree in both scenarios.

| Scenario | Baseline median seconds | Candidate median seconds | Median paired increase |
| --- | ---: | ---: | ---: |
| Arsenal / Manchester United | 0.379328 | 0.388167 | 2.11% |
| Cruzeiro / Manchester United | 0.365251 | 0.369059 | 1.18% |

The paired percentage is the median of group ratios, not the ratio of the two
independent medians. Benchmark receipts and original inputs remain private.
That two-root integration experiment passed functional acceptance and failed its
then-applied performance gate. It was preserved in `a37e44d` without installing.
These measurements predate recovery integration and are not a fresh benchmark of
the 685-entry artifact. The resumed complete-port objective prioritizes accurate
conversion and defers optimization, as stated in the committed progress ledger.
No speedup is claimed.

## Installation and remaining scope

The protected update preserves the previous game as a private reference and
installs the signed ARM64 executable with SHA-256
`2d8ac630790d9d46c5bacb61d1201381267ee3f91a60f06d0c4028bff37179bd`.
Candidate, installed and running executable identities agree; strict deep signing,
all four bundled original-program payloads and 845 protected files pass. The
installed app renders Prepare Game and exits through Command-Q and its original
confirmation. An accessibility query reopened the app after the first exit;
that second owned process was also closed normally. Final process inspection
finds no game process. Per-root runtime coverage belongs to the complete paired
match, not the initial-menu smoke. Only the game application is updated; the
installed Match Debugger and its source are preserved.
The complete port remains active. Save/Load is **not retested** for this bounded
actor-attribute and recovery delivery.

Cleanup removes ten reproducible comparison executables (1,436,560,792 logical
bytes), recovering 1,420,857,344 observed free bytes. Objects, linked libraries,
generated references, recipes, receipts, app bundles and protected inputs remain.
