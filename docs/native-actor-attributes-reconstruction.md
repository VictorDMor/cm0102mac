# Actor action attributes: validated integration candidate

The complete actor action-attribute refresh (`10235ea0..102373a0`) and its
signed-maximum helper (`102373a0..102373b4`) now have named C++23 component
implementations. They cover **1,349 indexed original instructions** across
three generated pages. Both roots are selected by destination hooks in the candidate
shared game engine on `codex/actor-attributes-integration-validation`. Its **684-entry manifest**
retains the preceding 682 entries in exactly the same order and with the same
owners. This integration is withheld from `main` and the installed game because
paired measurements show a runtime regression. Functional equivalence passes;
performance acceptance does not.

## Scope and selection

Fresh CPU samples of the accepted spatial-role build (`b7f91c6`) highlighted
the still-generated pages containing this operation in both Arsenal and
Cruzeiro scenarios. Each profiling run executes 70 restored 600-tick intervals.
Page samples are selection evidence, not a measured cost attributable entirely
to this root. Paired timing is reported separately below.

The original index contains two external calls to the refresh root, from
`1022e380` and `102393ec`, and four calls from refresh to the maximum helper.
There are no indexed external entrances into either body interior, including
fall-through edges. The adjacent condition-recovery operation is outside this
scope. The separate sampler at `10226520` and random refresh at `1077b770`
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

## Component evidence

Release and UBSAN each pass **32,703 comparisons**: 2,956 normal completions,
29,744 interruptions and three discriminating own/helper negatives. Of these,
10,995 are paired structural cases using an explicitly injected wide bound.
All 1,349 original source sites are reached. Every nonnegative comparison checks
the full architectural and temporary state, mapped memory, allocator/import
state, ordered memory effects, dependency ABI, source order and accounting.

The matrix includes all 256 encodings of each baseline byte; condition and
constant IEEE edge values; enabled floating exceptions; positive/negative wide
bounds and correction extremes; every wrap position among the fourteen draws;
dependency mutations and interruptions; aliased slots, actor/stack overlaps,
relocated TOC; every instruction budget and every accessed byte hole along
representative paths. Independent expectations check ordinary output bytes,
pool advancement, zero consumption and signed maximum results.

Eight Python audits pass for source fingerprints, boundaries, incomplete or
mutated records, isolated references and all page fragments, including destination hooks before original instruction
accounting. The focused component CTest passes. UBSAN instruments the body,
adapter, private diagnostics, component/production/transfer checkers and
independent original references; unchanged linked libraries and generated
production pages are not newly instrumented. Sanitizer
stderr is empty. Private inputs, generated references and receipts stay under
`.local/actor-attributes-reconstruction`.

## Production and complete-match acceptance

Natural engine callers capture both roots without synthetic argument setup.
The fresh-world and caller-capture interval records 44 refresh calls and 176
maximum calls, with equal
return counts and no original fallback, faults or dispatch escapes. A paired
Arsenal/Manchester United match, seed 12345, reaches full time after **798
advances** with 17,728 refresh calls and 70,912 maximum calls. Every tick agrees
in return and instruction accounting; intermediate and final complete states,
host inputs and import ABI also agree.

Release and UBSAN each pass 31 actual-production comparisons plus one explicit
null-actor fault after frame creation. The matrix covers direct, generated and
CFM descriptor entry paths; global/per-entry original selection; two negative
controls; and eleven budget stops, including completed frame creation and the
maximum helper. Six original-profile controls pass
separately. All 48 affected historical manifest checks pass.

Both modes also pass **42 nonlocal comparisons**, including 24 context-restore
or resumed-handler faults. Interruptions occur before the first draw, after a
maximum child returns, and at the final attribute store. Both memory observer
channels are exercised; the primary channel's first point uses a scratch write
because that channel does not report reads. The maximum leaf has no memory or
external call boundary inside its five instructions; its complete instruction
budget matrix is checked separately. Abandoned refresh frames execute no later
stores, epilogues or callback continuations. Completed child counts remain
exact. Original/modern context dependencies and turbo controls are included.

Four focused CTests have passing results. The initial transfer fixture attempted
a read interruption on the write-only primary observer; it was corrected and
the affected Release/UBSAN transfer checks rerun successfully. This fixture
correction did not change the production implementation.

Generation retains all 1,953,334 instructions, 1,922 pages and zero missing or
pending targets. Only `page_10235.cpp` and `page_10237.cpp` differ among the
1,923 generated C++ files. Full-body/page fingerprints and all indexed incoming
edges remain audited. State layout is unchanged; diagnostics are private.

## Performance and installation

The first integrated candidate was slower by median 7.67% and 2.71% in the two
600-tick scenarios. The final candidate exposes constant instruction sites and
register selections across named phases, specializes the absent-observer path,
and routes unobserved, unprofiled maximum calls with sufficient budget directly
through their existing adapter. Original controls, counters and interrupted-call
handling remain intact. Release and UBSAN acceptance was repeated after these
changes. These adjustments do not establish a performance gain.

Final timing compares the actual pre-integration library with the final candidate
in alternating order: ten measured groups of five runs per mode, after warm-up,
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
This closes the integration experiment with functional acceptance and a failed
performance gate.
The integration is withheld from `main`; the installed game remains the accepted
spatial-role build. Neither application was replaced in this cycle. Match
Debugger source is unchanged. Save/Load and installed management UI behavior
are **not retested**. Further optimization requires an explicit goal resume;
no additional subsystem is started as part of this cycle.
