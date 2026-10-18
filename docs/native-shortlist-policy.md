# Recruitment offer, country and row policies

Status: **prepared and accepted as an isolated component**. Three complete native
operations match independent original references. Their productive integration,
actual-service comparison and installation remain pending. The installed
application remains at revision `bd27aad728074cee6932340eaac4785a036f4a86`, with
861 unique semantic owners. The prospective count of 864 is not an installed
result.

## Scope and provenance

| Entry | Exclusive end | Indexed instructions | Native operation |
| --- | --- | ---: | --- |
| `0x1036f170` | `0x1036f588` | 262 | `classify_contract_offer` |
| `0x1036f590` | `0x1036f664` | 53 | `check_candidate_country_policy` |
| `0x1036fbf0` | `0x1036ff00` | 196 | `evaluate_row_contract_gate` |

The original index has sixteen incoming root edges and no external interior
entry. All 511 instructions are retained. Five nonindexed zero alignment words
are pinned separately. A 728-byte floating bank and its original TOC pointer are
pinned; no original table bytes are embedded in the native operations. Adjacent
role-classification, presentation and list helpers remain outside this block.

This is a coherent policy boundary used by the already reconstructed recruitment
controller: registration calls the country policy before considering rows, the
row gate when evaluating a contract, and offer classification before completing
a registration. The transfer synthesis block also calls the row gate. The last
integration's original offer-policy rejection motivated this selection; it is
not a claim that these are the game's hottest routines.

## Observed contracts

Offer classification receives the current club in r4, staff in r5, a signed
64-bit pair in r6/r7, the considered club in r8 and another signed pair in r9/r10.
A further mode byte is read at entry stack `+0x3b`, after frame creation at
`+0xbb`. The r3 input is overwritten. It consults the secondary contract, club
modes, profile fields, country services and reference amounts. The result is an
integer code in r3. The recruitment caller explicitly rejects code 2; no general
interface labels are assigned to codes 0 and 1.

The second pair's `-1` sentinel selects a service-derived reference. Another
value can select its supplied low word instead. Comparisons of the first pair
must retain signed high-word ordering, low-word carry and exact conversion to
double; reducing the operation to a 32-bit comparison changes this contract.
The implementation uses native named C++ stages and local carry/record helpers.
It has no original-body replay, instruction interpreter or PC dispatcher.

The country policy uses staff in r4 and the considered club in r6. It checks the
staff's first country reference and the considered club's country. Original
country byte `+0x118 == 14` enables additional checks, including the second staff
country reference, a staff exception service and transfer-related lookup. The
final delegated result is forwarded without normalizing it to a C++ Boolean.
No real-world nationality-rule name is inferred from the raw country code.

The row policy consumes a row in r4, an optional contract in r5, the considered
club in r6 and an output-word pointer in r7. It clears that output **before**
null/status guards. It then consults availability, a contract-associated staff
reference, computed amounts and a category service. Some paths compare signed
word amounts; others apply different floating factors. Output aliasing can
therefore affect later reads and is part of the original behavior.

## Component evidence

The final component performs **808 comparisons**: 757 complete returns, 45
matching stops and six rejected deliberate corruptions.

| Root | Comparisons | Complete returns | Matching stops | Negatives | Complete-return sites |
| --- | ---: | ---: | ---: | ---: | ---: |
| Offer | 330 | 305 | 23 | 2 | 262 |
| Country | 141 | 129 | 10 | 2 | 53 |
| Row | 337 | 323 | 12 | 2 | 196 |

All 511 original sites occur in complete returns; this component needs no
unreachable-site exceptions. Each comparison checks all architectural registers,
temporary residues, every mapped memory byte, ordered memory effects with their
instruction PCs, source order, dependency entry/return ABI and instruction
accounting. The original reference disables all discovered productive hooks and
emits every original instruction, independently of the native stages.

The synthetic model explicitly declares external service results/effects. The
already accepted raw signed-pair-to-double arithmetic is reused at that boundary.
Inputs cover optional contracts, raw modes and signed bytes, both country
references, amount thresholds, signed extrema and carries, the `-1` sentinel,
output aliases, measured budget cuts, removed accessed bytes and interrupted
callees. These prepared inputs are not observed career events.

Each root rejects an own-result corruption and an extra external memory effect.
The effect is restored before return, and the negative requires equal final
images; the ordered-effect comparator must still reject it. The country effect
negative is required to reach an actual declared dependency, rather than
pretending an early-return path exercised one.

Ten source/data checks pass, including exact full boundaries, changed/missing/
reordered source, page fragments, padding, every pinned bank byte, independent
references and the exact unchanged 861-owner manifest. The focused CTest
`shortlist_policy_component_equivalence` passes on ARM64 with
`-ffp-contract=off`, `-Wall -Wextra -Werror -Wpedantic`.

The cumulative Portuguese guide records this prepared block and practical
examples. Productive hooks, actual original-service/caller proof, focused UBSAN
and installation belong to the next consolidated milestone. Save/Load is **not
retested**; performance is **not benchmarked**. The complete-game goal remains
active.
