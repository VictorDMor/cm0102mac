# Periodic actor condition and action consumption

This is the preceding 687-entry delivery. The subsequent
[restoration and record-query block](native-actor-condition-restoration.md) extends
the component to nine entries and the accepted manifest to 691.

Two complete original operations now have named C++23 bodies in the existing
actor-attribute subsystem: periodic refresh (`102393c0..1023958c`, 115 indexed
instructions) and condition consumption (`10239590..1023978c`, 127). The first
has two indexed callers; the second has four. Their separate roots are required:
`10239590` is independently callable, not an interior continuation of refresh.
Neither complete body has an indexed external interior entrance.

This adds 242 original instructions and two productive entry points. The
**687-entry manifest** preserves the preceding 685 entries in order and owner.
The combined five-entry attribute/condition scope covers **1,650 instructions**.
Earlier refresh, maximum and recovery acceptance remains recorded in the
[attribute report](native-actor-attributes-reconstruction.md). This is a coherent
condition-update block; remaining actor behavior and the full port are unfinished.

## Original contracts

Periodic refresh first calls the accepted ten-attribute operation, then draws
one additional shared-pool word. Its original signed remainder modulo eleven
determines whether condition is updated. Cursor increment, captured limit,
wrap/reseed, salt narrowing and dependency clobbers retain source ordering.

On the update path, an exertion byte at actor `+102` can decrease after a signed
comparison derived from `+103` and the live player-record byte at `+25`. The
operation adds the single-precision drift at `+5c` to condition `+50`. An exact
sentinel resets drift to zero; otherwise a live double factor decays it before
single narrowing. An unsigned actor byte at `+1`, above 50, selects a power-based
additional cost through the existing CFM descriptor. Its saved TOC, result
narrowing and original cost cap remain intact. No stronger interpretation of that
byte is required by this reconstruction.

The periodic operation clamps the internal condition to the live double bounds,
which are **1 and 10,000** in the pinned original program. These internal units
must not be confused with the displayed condition percentage. Recovery has no
corresponding clamp in its own body. The 96-byte frame, integer saves, LR/TOC,
volatile residues, floating status and ordered fault prefixes are preserved.

Consumption sign-extends its action halfword. The `-1` branch applies one of four
movement factors using live internal-condition thresholds 9,500, 8,750 and 7,750.
Other actions use a live 28-word guest destination table for action codes 19–46,
or the original default path outside that range. All three formula destinations
remain semantic, and changing the table's pointer or its selected destination is
observable. The product/subtraction/negation sequence retains separate rounding;
it is not contracted into a fused operation. Costs retain subsequent multiply,
divide, single narrowing, condition subtraction and the zero floor.

Computed transfers preserve the guest destination and existing LR through the
explicit dependency boundary. The component matrix separately checks a changed
external destination; it is not counted as ordinary match coverage. The leaf
has no frame or callback dependency on its normal formula paths.

## Evidence

The combined component matrix passes **49,380 comparisons** in Release and
in affected-unit UBSAN: 3,560 completions, 45,812 interruptions and eight
own/child negatives. The 10,995 explicitly injected wide-bound cases belong to
the preceding attribute sampler. Every one of the combined 1,650 original
source sites is reached. Nonnegative comparisons check full architectural and
temporary state, mapped memory, ordered effects, dependency ABI, source order
and instruction accounting against independent original references.

New cases cover the modulo gate and pool wrap, signed-byte threshold extremes,
periodic lower/upper clamps, exact and decayed drift, power results including IEEE
edges and the cost cap, every action-table row redirected to every formula,
relocated live table and TOC, signed halfword encodings, all movement thresholds,
all formula floors, NaNs/infinities/signed zero and enabled exception flags,
aliases, dependency interruption, every instruction budget and every accessed
byte hole on representative paths. Independent expectations establish drift,
exertion decrement and movement cost/floor results. Source/table pointer and
all 28 original table words are pinned with mutation negatives.

Actual callers in a seeded Arsenal/Manchester United match exercise all five
roots. The original-selected and C++23 modes both reach full time after **798
advances**, preserving intermediate/final complete states, host inputs/import
ABI and every tick's return and instruction accounting:

| Operation | Normal calls and returns |
| --- | ---: |
| Attribute refresh | 17,728 |
| Signed maximum | 70,912 |
| Recovery | 117 |
| Periodic condition | 17,490 |
| Consumption | 13,119 |

Dedicated modern counts equal original-control counts, with no original/profile
selection, faults, budget stops or dispatch escapes in the modern match. The
periodic snapshot used by interruption fixtures is captured from a natural caller
whose update branch actually reaches the final condition store.

Release and UBSAN each pass **82 production comparisons**, including five own
negatives and 32 budget stops, plus four separate null-actor faults. Direct,
generated and CFM descriptor paths, global/per-entry original controls and
private per-entry diagnostics are exercised. Fifteen original-profile controls
pass separately in Release.

Both modes pass **140 nonlocal comparisons**, including 80 restore/handler
faults. The periodic parent's cases include interruption inside its refresh
child, after that child returns and at the final condition store. Parent and
child transfer/return/fault counts remain exact. The consumption leaf is
interrupted at its condition store through both memory observer channels.
Abandoned host continuations do not execute later stores or epilogues. Existing
refresh/recovery/context controls remain included.

Four focused CTests pass in 25.06 seconds. Nine subsystem boundary/reference/table
and hook audits and 48 affected historical manifest checks pass. Eight affected
units are instrumented separately with UBSAN; generated pages and unchanged
linked archives retain the consistent Release build. Sanitizer stderr is empty.
The first production fixture used a budget of 25 for a naturally shorter leaf
path; both versions correctly returned. The fixture now uses an actual
interruption at budget 18. No product correction was needed for that fixture.

Generation retains 1,953,334 indexed instructions across 1,922 pages, with no
missing or pending destinations. Only `page_10239.cpp` changes among 1,923
generated C++ files relative to the preceding 685-entry generation. State layout
is unchanged; the adapter's counters remain private diagnostics.

## Delivery boundary

The protected installer retains the preceding game as a private reference and
installs the signed ARM64 executable with SHA-256
`ea8cb2efdcfdc9f2619434187f742b60d9fece9adec20b7be7db8649d72dbd67`.
Candidate and installed identities, strict deep signatures and all four original
payloads agree. All 845 protected files and the installed Match Debugger are
preserved. The initial installed UI attempt was blocked by the Mac lock. After
manual unlock, a supplemental check reached Prepare Game, displayed the original
exit confirmation and exited normally after 9,059,733,818 statically recompiled
instructions. The installed executable identity matched the 687-entry artifact.
That deferred smoke is now resolved; its private receipt is
`delivery/unlocked-smoke.json`.

Receipts remain private under `.local/actor-condition-reconstruction`.
Save/Load is **not retested**. No new timing is measured and no speedup is claimed:
the complete-port objective prioritizes accurate conversion and defers
optimization. The full conversion goal remains active.

Cleanup removes six reproducible comparison executables (862,330,776 logical
bytes), recovering 856,748,032 observed free bytes. Objects, archives, generated
references, recipes, receipts, application bundles and protected inputs remain.
