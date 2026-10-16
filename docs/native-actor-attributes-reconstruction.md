# Actor action attributes: complete component reconstruction

The complete actor action-attribute refresh (`10235ea0..102373a0`) and its
signed-maximum helper (`102373a0..102373b4`) now have named C++23 component
implementations. They cover **1,349 indexed original instructions** across
three generated pages. They are linked only into the component checker;
production selection remains at **682 entries**. Neither installed application
was changed in this cycle.

## Scope and selection

Fresh CPU samples of the accepted spatial-role build (`b7f91c6`) highlighted
the still-generated pages containing this operation in both Arsenal and
Cruzeiro scenarios. Each profiling run executes 70 restored 600-tick intervals.
Page samples are selection evidence, not a measured cost attributable entirely
to this root. No speedup is claimed for the new components yet.

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

Seven Python audits pass for source fingerprints, boundaries, incomplete or
mutated records, isolated references and all page fragments. The focused CTest
passes. UBSAN instruments the new body, checker and independent original
references; unchanged linked libraries are not newly instrumented. Sanitizer
stderr is empty. Private inputs, generated references and receipts stay under
`.local/actor-attributes-reconstruction`.

## Remaining acceptance

These are validated components, not a production integration. Dedicated
selection/counters, real-caller comparisons, nonlocal transfer checks, paired
complete-match acceptance and a fair timing comparison remain before selecting
the bodies in the game. Any later installation is game-only unless the user
requests Match Debugger work. Save/Load and installed UI behavior were not
retested. The complete C++23 port goal remains active.
