# Transfer-offer valuation, comparison and exchange editing

Eight complete original operations (1,188 instructions) are reconstructed and
prepared for production integration. Together with the preceding eight record
operations, this forms a 16-operation candidate: 831 unique owners after the
exact accepted 815-entry prefix. Neither prepared component adds production
ownership yet. The installed application remains at `b976b2b`.

## Complete boundaries

| Entry | Exclusive end | Instructions | Operation |
| --- | --- | ---: | --- |
| `0x10415e50` | `0x104163c8` | 350 | `sample_offer_range` |
| `0x10416840` | `0x10416d24` | 312 | `adjusted_offer_value` |
| `0x10416d30` | `0x10416d5c` | 11 | `positive_installment_total` |
| `0x10416d60` | `0x10416e04` | 40 | `staff_offer_status` |
| `0x10416e10` | `0x104170f0` | 183 | `equivalent_offer_terms` |
| `0x104170f0` | `0x10417260` | 92 | `remove_exchange_staff` |
| `0x10417260` | `0x104172e4` | 32 | `find_exchange_staff` |
| `0x104172f0` | `0x10417590` | 168 | `rebalance_offer_terms` |

The index contains 30 incoming root edges and no external body-interior edges.
Valuation, list, normalization, finance, contract and statistics services remain
explicit dependency boundaries. This block does not reconstruct automatic
proposal synthesis, negotiation policy or the underlying finance service.

## Shared range sampling

The offer sampler and the accepted contract sampler have identical 350-step
control flow after relocation. Exactly five instructions differ: one live TOC
table pointer and four divisor offsets. Both now use `range_sampling_detail.hpp`
with explicit entry/table/divisor layouts. Each keeps its own independent
original reference. The former contract implementation is a thin named wrapper;
the shared engine never replays generated bodies. The common semantic-call
helper is unchanged.

Zero does not advance the pool. Narrow signed bounds use the original signed
remainder. Wide bounds retain scaling, inline correction draws, recursion and
clamping, including source-order pointer reuse and rollover/reseed effects.
The offer layout reads the divisor 65535 from table+0xa0; the contract layout
reads its own live table+0. Structural branch controls used to exercise deep
paths are reported separately from natural component inputs.

## Adjusted value and installment sum

The simple installment query adds only strictly positive signed amounts at
record+0x40 and +0x48, with original 32-bit wrapping. The adjusted valuation is
separate from the preceding record's bare value composition. It begins with the
base, can discount installment conditions, considers signed cached age,
availability, reputation and statistics, optionally adds exchanged players,
and applies a final scale. A 200-unit ordinary installment with code 10
contributes 190 before other factors: `200 * (1 - 0.005 * 10)`.

With an eligible primary contract, a positive signed period and the adjustment
flag enabled, the finance mood's signed low byte selects factors. Mood -2 gives
an extras factor of 0.5 and final scale 0.1; -1 gives 0.75 and 0.3. Mood 0 uses
final scales 0.9 for period below 7, 0.7 below 13 and 0.4 otherwise. Mood 1 uses
0.9 only above 12; other values retain the default 1.0. These codes are not
assigned speculative UI labels. Flags are independently truncated to low bytes.

The final integer-to-float conversion/subtraction and multiplication explicitly
round to single precision before truncation to integer. Intermediate products,
floating status and all original writes remain ordered. Unlike bare value
composition's alternate exchange mode, this adjusted operation passes **each
exchanged player's identity** to the valuation service. It also rereads mutable
list pointers and preserves the two statistics calls and shared contract scratch.

## Comparison and status

Comparison normalizes amounts through the original 64-bit service result; it is
not raw record equality. Nonpositive signed installment amounts can compare as
absent despite differing codes. Nonpositive signed condition bytes likewise
have absence semantics. The packed upper nibble is compared independently.
Exchange counts must match, then every item on the right must be found in the
left. Matches are not consumed: left `{1,2}` and right `{1,1}` can compare equal,
while the reverse fails. Shared duplicate lists can repeat until the instruction
budget stops them. The reconstruction retains this asymmetry and cursor effects.

The status query takes an integer staff ID. If it is the main target and the
record kind is 3, the query stores status 4 before returning it; otherwise it
returns the stored main status. For exchange IDs it returns the payload status,
and defaults to 4 when absent. The separate find/remove operations instead take
a staff pointer. Find returns the matching payload pointer or zero.

## Exchange removal and payment rebalance

Removal visits the list, removes matching IDs, and disposes/clears an empty list.
When the list pointer is nonnull, it then obtains a service value, adds it to the
base and decrements an unsigned-byte cached count with wrapping. These latter
effects still happen if no item matched. A missing list takes the original
diagnostic path and clears its flag instead; no rollback is added.

Rebalance requires a profile, potential minus current ability of at least 25,
zero percentage byte and zero ordinary/contingent installments. A finance query
must be no greater than the sign extension of the **wrapped 32-bit doubled base**.
Both skill comparisons use signed integer skill divided by **500**. If club
score is below 15 and skill/500 exceeds that score by more than 2, base becomes
80 percent and the percentage byte becomes 20. Otherwise reputation above 5000,
skill above 4250 and availability byte zero select a 30-percent contingent amount
with code 10 and an 80-percent base. Otherwise skill/500 below club score minus 3
selects a 30-percent ordinary amount with code 20 and an 80-percent base.
The remaining path leaves the amounts unchanged. These are observed numeric
fields; no unproved season/month or player-facing meaning is attached to codes.

## Evidence and delivery boundary

The focused Release CTest passes **7,559 comparisons**: 3,242 complete agreements,
4,302 matching interruptions/memory faults and 15 discriminating negatives.
All **1,188 original source sites** are required against an independently emitted
exact list. Twenty-five comparisons use identified structural branch controls;
this is component evidence, not natural career execution. Eight own-operation
negatives and seven dependency negatives are rejected; the pure installment sum
has no child call. Fixture assertion failures cannot count as agreement.

Checks compare full architecture and temporaries, mappings/bookkeeping, ordered
memory effects, call entry/return ABI, source order and instruction accounting.
Cases include signed low bytes/halfwords, overflow, normalization, duplicate and
shared lists, mutable dependencies, nulls, mood/period thresholds, special float
values, relocated TOC/tables, sampled missing bytes, budget cuts and each child
interruption. Independent expectations cover the installment discount, plain and
service exchange values, final scales, normalized amounts, duplicate asymmetry,
status mutation, cache wrapping and all payment distribution branches.

Twelve source/data guards pin the exact installed prefix, absent ownership,
complete original bodies, edges, three live tables/pointers, shared diagnostic,
sampler layout differences and independent-reference labels/steps/sites.
The accepted 21-operation contract-terms component also passes its complete
**5,242 comparisons and 1,203 sites**, with twelve source guards, after extraction
of the shared sampler. The preceding record component's sources remain unchanged.
Warnings are errors and floating contraction is disabled.

New semantic units are linked only into component checks. Production generation,
actual-service coverage, focused sanitizers and installation remain for a
consolidated milestone. Generated sources, the installed game, protected files
and eighteen unrelated prompts are unchanged. Private references, tables and
receipts stay outside publication under `.local/transfer-valuation-reconstruction`.
Save/Load is **not retested**; performance is **not benchmarked**. The cumulative
Portuguese PDF adds these mechanisms and practical examples. The full goal remains
active.
