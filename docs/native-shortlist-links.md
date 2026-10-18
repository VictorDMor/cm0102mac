# Shortlist link indices, row transitions and contract markers

Ten complete operations were prepared in `ecc9be0`, covering **1,122 indexed
instructions**. That preparation milestone retained 844 installed owners.
The subsequent [productive integration](native-shortlist-links-integration.md)
adds all ten unique owners after the exact accepted prefix, reaching **854**.
The component evidence below retains its own scope and does not replace the
separate actual-service, generated dispatch and installed-application evidence.

## Complete boundaries

| Entry | Exclusive end | Instructions | Operation |
| --- | --- | ---: | --- |
| `0x10362d70` | `0x10362df4` | 33 | `contains_staff_club_link` |
| `0x10362e00` | `0x10362eec` | 58 | `collect_club_links` |
| `0x10362ef0` | `0x10363128` | 141 | `collect_staff_links` |
| `0x10363130` | `0x103632e4` | 109 | `mark_primary_contract` |
| `0x103632f0` | `0x103636ac` | 239 | `add_staff_club_link` |
| `0x10363b80` | `0x10363c98` | 69 | `reset_linked_club_rows` |
| `0x10365590` | `0x10365c28` | 422 | `transition_staff_links` |
| `0x10365c30` | `0x10365c78` | 18 | `clear_primary_contract_flag_one` |
| `0x10365c80` | `0x10365cbc` | 15 | `set_primary_contract_flag_two` |
| `0x10365cc0` | `0x10365d08` | 18 | `clear_primary_contract_flag_two` |

Seventy-six indexed external edges target these roots, with no indexed external
interior entry. Three nonindexed padding words are pinned separately from the
counted no-op at `0x10362e90`. Complete body/page fingerprints, original
diagnostic strings and both live comparator transition vectors are pinned.
The comparators remain owned by `shortlist_category_adapter`; storage and reset
retain their existing owners. Adjacent loan and scouting controllers, including
`0x103636b0`, `0x10363ca0`, `0x10363f60` and `0x10366470`, remain outside this
link-index and marker block. Their instructions are not partially claimed.

## Two views of the same link

The shortlist object holds eight-byte staff/club ID pairs in two arrays: +0 is
ordered by staff then club, and +4 by club then staff. Capacities are at +8/+12;
used counts are at +0x10/+0x14. Original transition vectors select the already
owned comparators `0x10366410` and `0x103663b0` respectively.

Membership builds a stack key from the staff ID and optional club ID. A missing
club becomes -1; the comparator's secondary sentinel permits a staff-only match.
A null first index returns zero before dereferencing the staff. A present index
uses the original search service and normalizes a nonnull result to one.

Club collection searches the reverse index, rewinds to the first adjacent match
and appends every matching pair, preserving its last-entry bound and callback
ABI. Staff collection can first inspect the final sixteen staff records, applying
their active marker, stored-club predicate and staff service. Signed mode 3 skips
that scan; signed mode 2 can reject a reserved-club result. A separate signed
flag can skip cached links. A supplied output list is cleared first; a missing
output is not turned into a universal safe no-op when an append is reached.

For cached staff links, the operation examines the club's forty rows, uses the
matching row's +8 value, and compares it with staff+8. If all forty primary IDs
miss, it uses the **first row's +8 value**. It also applies the club-entry +9
threshold unless the force flag is set and excludes the staff's stored club.
These original fallbacks, access order and volatile return residues are retained.

## Allocation, insertion and linked-row reset

Each index starts with 100 pairs, or 800 bytes, and grows by another 100 when
capacity equals its used count. Both missing arrays are allocated before the
duplicate lookup. The duplicate check uses the first index; it does not silently
repair an inconsistent second index. A new pair is appended to both arrays and
each is sorted through its original comparator descriptor.

A zero signed-byte club-predicate result increments the upper four-bit counter
at the staff-status record's +12 byte. The other nibble is retained; incrementing
15 wraps to zero. Querying with a missing club can match a wildcard, but insertion
of a missing pair still dereferences the club record. A resulting positive null
fault can occur after allocations or the first word write. Allocation, resize,
partial effects, four diagnostics and original exit requests are preserved.

Linked-row reset constructs a temporary list and calls staff-link collection.
For each result it writes 100 to the linked club entry's +9 byte. With a nonzero
signed-byte flag it scans forty rows and changes a matching nonnegative +0x18
reference to -1, retaining the primary +0x10 reference. It then disposes of the
temporary list. Counts, row pointers and iterator results are reread in the
original order; the operation does not normalize its final return residue.

## Transitions and contract markers

The transition operation takes object, staff, optional club, signed-byte mode
and signed-byte row filter in r3 through r7. Mode 4 with a missing or stored club
can collect other links and call the transition recursively with mode 1/filter 2.
A missing club then disposes of the temporary list and returns one.

Mode 5, or a non-mode-4 transition to a different club, removes one matching pair
from each index. It shifts successors, clears the old tail and decreases each
used count. Missing either index returns zero after list disposal. If both
searches found a pair and the club predicate's signed low byte is zero, the
metadata nibble decreases; zero wraps to fifteen. A missing search result keeps
the counter unchanged even if the other index was modified.

The operation then queries the club contract and scans forty rows. Filter one
skips type eleven, filter zero accepts only eleven, and other signed values use
the remaining original path. Primary-ID matches are considered; mode four can
also match the intervening +0x14 reference.

| Mode | Observed row behavior |
| ---: | --- |
| 1 | Invalidates primary/secondary references, conditionally clears the intervening reference, clears bit seven at +0x28 and calls reassignment. |
| 2 | Uses the original date-add service with 120 days, stores the staff ID in +0x14, invalidates +0x18, clears +8/+12 and updates the contract nibble when present. |
| 3 | Invalidates +0x10/+0x18, uses the date-subtract service with 120 days and conditionally clears +0x14. |
| 4 | Clears a matching +0x14; a primary match also invalidates both references and uses the 120-day subtraction. |
| 5 | Uses the mode-two field sequence with a fourteen-day date addition. |

These are internal modes and observed date arguments, not inferred UI actions
or a promise that every link has a fixed expiry. The remaining mode paths,
contract/status changes, row-pointer reloads and normalized transition return
are preserved too. Reassignment, scouting and transfer service bodies are
external dependencies of this component.

Primary-contract marking first applies the original staff and contract guards,
sets flag one at contract+0x5c, and writes 7 into the low nibble of +0x60. Pending
transfer, club and random-difference services govern its subsequent calls. The
two percentage draws retain their prefetch order, refill boundaries, signed
threshold and arithmetic residues. No universal probability interpretation or
UI label is assigned. Two clear operations normalize presence to zero/one; the
flag-two setter retains the contract-pointer return convention. The unrelated
contract bits and upper status nibble are preserved.

## Comparison evidence and next milestone

**3,117 comparisons pass**: 2,076 complete agreements, 1,021 matching budget,
pointer, dependency-interruption or exit stops, and twenty rejected negatives
(one own-result and one child-effect control for every root). All **1,122 sites**
occur outside budget-only observations. Complete returns cover **1,034 sites**;
the remaining **88 sites** belong to the four nonreturning allocation-error
diagnostics. No error path is reported as a natural complete return.

Two source-site controls are explicitly structural: replacing the receiver after
index allocation, and introducing a late null-club key after the normal early
return before restoring the club for row processing. They reach otherwise
unavailable joins and are not natural career observations.

Comparison checks every architectural byte, generated temporary, mapped byte,
ordered read/write effect, dependency entry/return register image, source-site
order and instruction count. Inputs include forward/backward pair scans, absent
keys, signed flags, reserved staff, score/status boundaries, both allocations and
resizes failing, all transition modes, contract presence, date callback pointer
replacement, nibble wrap, RNG refill boundaries, all 256 marker bytes, relocated
TOC/tables, pointer holes and budget cuts. Independent assertions establish the
wildcard key, complete neighboring collection, both index counts and contents,
retained fields, marker return conventions and row/contract effects. Composed
cases also execute the selected original/C++ child bodies, including recursive
transitions, rather than replacing those children with fixture results.

The independent references keep all original instructions with every productive
hook disabled. External search, sorting, allocation, list, date, club, contract
and notification services are declared dependency fixtures; their internal
algorithms and actual-service composition are not proved by this component.
Ten source/data guards passed at preparation. The integration subsequently adds
the four semantic units and their adapter to production; its report records the
extended guards, actual services, focused sanitizers and installed artifact.
Save/Load: **not retested**. Performance: **not benchmarked**.
