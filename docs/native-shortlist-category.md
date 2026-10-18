# Shortlist category eligibility, row review and pair comparison

Five complete original operations were prepared and compared as a component
in `5936f4f`. They cover **467 indexed instructions**. Their productive
[integration](native-shortlist-category-integration.md) now adds five unique
owners after the exact accepted 839-entry prefix, reaching **844 entries**.
The component evidence below retains its separate scope.

## Complete boundaries

| Entry | Exclusive end | Instructions | Operation |
| --- | --- | ---: | --- |
| `0x10365d10` | `0x103660a4` | 229 | `eligible_for_category` |
| `0x103660b0` | `0x10366250` | 103 | `count_and_prune_club_rows` |
| `0x10366250` | `0x103663b0` | 87 | `refresh_unassigned_rows` |
| `0x103663b0` | `0x10366410` | 24 | `compare_pairs_by_second` |
| `0x10366410` | `0x10366470` | 24 | `compare_pairs_by_first` |

Six indexed edges target these roots; no indexed external edge enters a body
interior. Two nonindexed padding words are separately pinned to `60000000`.
The complete sixteen-destination category table, its live pointer and the
original diagnostic strings are pinned. The neighboring owned storage/date
operations are excluded from this block.

## Category eligibility

The low signed byte of r5 selects the category; r4 is a staff pointer. The
object in r3 is unused. Categories 0 through 14 require a profile at staff+0x14
and one or two signed profile bytes of at least 15. Their original layout is:

| Category | Required profile byte offsets |
| ---: | --- |
| 0 | +0x0f |
| 1 | +0x10 and +0x19 |
| 2 | +0x11 and +0x18 |
| 3 | +0x11 and +0x17 |
| 4 | +0x11 and +0x19 |
| 5 | +0x16 and +0x18 |
| 6 | +0x12 and +0x19 |
| 7 | +0x16 and +0x17 |
| 8 | +0x13 and +0x18 |
| 9 | +0x13 and +0x19 |
| 10 | +0x13 and +0x17 |
| 11 | +0x14 and +0x18 |
| 12 | +0x14 and +0x19 |
| 13 | +0x14 and +0x17 |
| 14 | +0x15 |

These are observed field requirements; complete UI category/field labels are
not assigned speculatively. For category zero, 14 fails, 15 passes and raw byte
255 fails because it is read as -1. A qualifying profile can satisfy multiple
categories. With no profile, categories other than 15 return zero before the
range diagnostic, including invalid category values.

Category 15 works without that profile. With a nonnull club pointer and staff
role byte 11, 15 or 13, it consults the external club predicate with arguments
`(club,0,1,0,0)`; a nonzero signed low-byte result rejects eligibility. Otherwise
it requires staff+0x1c nonnull and a signed staff ID below `staff_count - 16`.
It does not add a nonnegative-ID check. Invalid categories with a profile use
the original diagnostic and clear the shared error flag.

The live table destination and CTR effects are retained. Named qualification
logic also supports aligned partial destinations inside those qualification
cases and both terminal joins. A changed destination outside those handled
cases remains an explicit continuation boundary; it does not synthesize a call
or LR write. Fifteen deliberately controlled table-destination cases are
reported separately from ordinary fixture inputs.

## Counting and pruning forty club rows

The count operation receives the object, club and optional category in r3/r4/r5.
It selects the twelve-byte club entry by club ID and scans forty 44-byte rows.
Both reference words +0x10 and +0x18 must be nonnegative. A signed low-byte
category of -1 accepts every row type; other values must match the signed row
type byte. This query does not independently validate the upper staff-ID bound.

With club-entry byte +4 equal to 2 and a nonzero signed-byte result from the
club predicate `(club,1,1,0,0)`, it subtracts twenty days from the current date
and compares that result to the date reached through club+0x90 at +0x4c/+0x4e.
Years compare unsigned; equal years retain the original signed-halfword
XOR/shift/AND/subtraction comparison and carry effects. The comparison is strict.

When that date test passes, it resolves the staff ID and calls the original
transfer query `0x103eaea0`. A zero full-word result clears both reference words
to -1 and excludes the row from the count. Other paths count the matching row.
The word at +0x14 is retained. Repeated row-pointer reads are preserved, including
when callbacks replace the row array. The date field's complete business label
and the external predicate/query internals remain outside this block.

## Reviewing unassigned rows

This operation takes object, club and a pointer to a club-entry row-array pointer
in r3/r4/r5. It scans forty rows, replacing an ID above `staff_count - 1` with -1
and rereading that ID. Negative IDs are skipped. A primary-contract lookup
updates the original shared contract scratch. A present contract whose parsed
signed type is not 5 supplies its indexed club; otherwise that club is null.

If neither that club nor the staff's stored club equals the requested club, and
row+0x18 is -1, the operation calls `0x10365590` with the original object, staff,
club, mode 3, a flag that is zero only for signed category 11, and the signed
category. The subfic carry and intermediate residues are retained. The external
registration operation may update the records; its body is not claimed here.
This review does not normalize a return value or clear unrelated row fields.

## Comparing pair arrays

The comparators order eight-byte pairs, one by the second word then the first,
and the other by the first word then the second. A null first pointer returns
-1, including when both pointers are null. Otherwise a null second returns 1.
With equal primary words, a secondary -1 on either side returns zero. Remaining
comparisons use the original wrapped 32-bit subtraction; no host comparison or
strict-order repair is substituted. All early returns and access order remain.

## Component evidence and remaining work

The independent references retain all original instructions and disable every
productive hook. **2,615 comparisons** pass: 2,197 complete agreements, 410
matching interruptions/faults and eight deliberately altered negatives rejected.
All **467/467 sites occur in complete returns** as well as the aggregate matrix.
Fifteen comparisons explicitly alter the category table destination. They are
structural controls, not natural career observations.

Comparison includes every architectural byte, generated temporary residue,
mapped byte, ordered access/effect, dependency entry/return register image,
source-site order and instruction count. Cases cover all category bytes, signed
qualification/role thresholds, missing profiles, date/year frontiers, category
wildcards, full-word versus low-byte service results, row-array/date-pointer
mutation, contract kinds, null clubs, invalid IDs, sentinel ties, wrapping pair
differences, pointer faults, budgets, interrupts and relocated tables/TOC.
Independent assertions check the inclusive threshold, signed bytes, two-column
pruning, retained fields, invalid-ID cleanup, category-eleven flag and comparator
edge cases. Twelve source/data guards pin the complete bodies, fragments, tables,
padding, entry boundaries, absence of new owners and isolated references.

At the preparation milestone the CMake target linked the two semantic units only
into the component executable. The later integration links those unchanged units
into the runtime and adds the adapter, real-service matrix and installed delivery
described in the integration report. Membership is used by both passes of the
shortlist reset, making it a coherent continuation of that storage block.
No whole-manager or natural-career coverage is inferred.
Save/Load: **not retested**. Performance: **not benchmarked**.
