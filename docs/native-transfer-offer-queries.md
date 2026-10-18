# Transfer-offer queries and rejection policy

The complete **19-operation, 1,111-instruction** block is prepared and compared
with independent original references. It is **not integrated**. The accepted
installed game remains at 796 entries, revision `1e1f641`. The exact accepted
manifest prefix is pinned; the nineteen candidates have no existing owner.

## Complete original boundaries

| Entry | Exclusive end | Instructions | Named operation |
| --- | --- | ---: | --- |
| `0x103e6290` | `0x103e6358` | 50 | `append_staff_entry` |
| `0x103e6360` | `0x103e6464` | 64 | `count_club_staff_entries` |
| `0x103e6470` | `0x103e64dc` | 27 | `contains_staff_entry` |
| `0x103ea250` | `0x103ea26c` | 7 | `set_staff_contract_flag` |
| `0x103ea270` | `0x103ea28c` | 7 | `clear_staff_contract_flag` |
| `0x103ea290` | `0x103ea348` | 46 | `query_staff_contract_flag` |
| `0x103ea350` | `0x103ea43c` | 59 | `rejection_reason_from_squad_status` |
| `0x103ea440` | `0x103ea5f8` | 110 | `rejection_reason_allowed` |
| `0x103ea600` | `0x103ea75c` | 87 | `describe_rejection_reason` |
| `0x103ea760` | `0x103ea83c` | 55 | `squad_status_for_rejection_reason` |
| `0x103eaea0` | `0x103eb014` | 93 | `collect_staff_offer_indices` |
| `0x103eb020` | `0x103eb1e4` | 113 | `collect_club_staff_offer_indices` |
| `0x103eb1f0` | `0x103eb358` | 89 | `collect_secondary_staff_offer_indices` |
| `0x103eb360` | `0x103eb434` | 53 | `collect_club_offer_indices` |
| `0x103eb440` | `0x103eb464` | 9 | `primary_offer_slot` |
| `0x103eb470` | `0x103eb494` | 9 | `secondary_offer_slot` |
| `0x103f4240` | `0x103f43a4` | 88 | `best_qualifying_offer` |
| `0x103f43b0` | `0x103f4594` | 119 | `query_special_staff_offer` |
| `0x103fb390` | `0x103fb3f8` | 26 | `offer_is_filtered` |

## Scope and composition

The block combines staff membership/marker operations, rejection-reason
selection and eligibility, descriptions and reason-specific squad codes,
primary/secondary offer collection, pointer-slot queries, best qualifying
offer selection and a special-state query. The shared filter and nested
collection calls are complete named operations. All 248 incoming indexed root
edges are recorded privately; no external edge enters a body interior.

The transfer scheduler at `0x103ea840`, offer creation/acceptance, broader
valuation, negotiation and management lifecycle remain outside this block.
This is a complete query/policy boundary, not completion of all transfers.

## Observed behavior

- Rejection reasons distinguish indispensable, important first-team and prospect
  status, unacceptable offers, same division and rivals. Squad codes 2 and 3
  share the important-player reason. Unknown reason codes preserve the
  original formatter/output behavior; they are not silently mapped to a default.
- Prospect eligibility compares the signed low byte of the cached halfword at
  staff+0x5c with 24. The same-division reason compares division pointers; rival
  eligibility retains both shared rule gates, the relationship query and six
  directional club links. Shared rule meanings are not inferred beyond these
  observed predicates.
- List queries can advance iterators, append indices and update a global contract
  scratch pointer. Marker queries can clear stale bits. These are stateful
  operations, including when their primary result is a count or boolean.
- The best-offer routine stops at the first matching family. Within that family
  it keeps the first strictly greatest qualifying signed value, starting at -1.
  A later family with a larger value is not considered.
- Slot queries use the original signed upper-bound comparison and pointer
  arithmetic, including negative indices. Source null/fault behavior is retained.
  One collector dereferences its staff argument before its null branch; another
  repeats a contract query and can dereference zero if its second result changes.

The formatted reason strings and diagnostics retain their live TOC/table
pointers. A 6,630-byte original table is pinned by pointer and SHA-256; its bytes
stay in private generated fixtures. Three complete 18-instruction diagnostics
are normalized only by scratch displacement and diagnostic line.

## Component acceptance

The focused Release CTest passes **10,429 comparisons**: 5,532 complete agreements,
4,864 matching interruptions or memory faults, and 33 rejected negatives.
Every one of the 1,111 original source sites is required. All nineteen roots
have own negatives; fourteen baseline roots with children also have child
negatives. Full architectural/temporary state, mappings and allocation state,
ordered effects, dependency input/output registers, instruction order and
instruction accounting agree. Only ordinary completion, exact guest-budget
exhaustion, explicit dependency interruption and unmapped memory faults are
accepted; fixture assertions cannot count as agreement.

Representative complete paths use measured budget cuts, sampled missing-byte
boundaries and each dependency interruption. Cases cover all flag/status/filter
bytes, reason branches, signed ages/indices/values, strict ties and first-family
selection, empty/sparse/linked lists, optional output, both collection paths,
callback changes, stale-marker rereads, aliasing and relocated TOC/text tables.

Thirteen comparisons from two structural inputs are counted separately:
a mapped low club-array base and a mapped-zero second contract lookup. The
latter completes one source site otherwise followed immediately by a null fault;
the same changed lookup without that mapping separately proves the original
fault prefix. These cases are not natural career execution.

Ten Python guards pin ownership, bodies/fragments, source fields and order,
external edges, table bytes/pointer, complete diagnostic normalization and
independent reference labels/steps with every semantic hook disabled. Named
operations never replay the original bodies. Compilation uses warnings as
errors and `-ffp-contract=off`.

The expanded matrix found a reversed equality comparison whose final result
was unchanged but whose intermediate condition flags differed at the next
dependency call. Restoring operand order fixed that ABI difference. A fixture
expectation for a different target club was also corrected before acceptance.
Earlier diagnostic attempts do not count as accepted complete coverage.

## Delivery boundary

The component target links the four new semantic units only into its checks.
Production ownership, generation, actual-service comparisons, focused sanitizer
acceptance and installation are pending the next consolidated milestone. The
796-entry generated sources, installed executable and protected files remain
unchanged. Private original inputs, references and receipts stay under
.local/transfer-offer-reconstruction and are excluded from publication.

Save/Load is **not retested**; performance is **not benchmarked**. The cumulative
Portuguese PDF explains this block with practical examples and identifies the
prepared/installed boundary. The full reconstruction goal remains active.
