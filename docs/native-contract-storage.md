# Contract storage creation

All three complete operations in this storage creation block have accepted,
directly compared C++23 implementations. Their **2,016 indexed instructions**
cover row reservation, primary staff binding and full initialization. They are
integrated with the three queries at the **762-entry installed milestone**, retaining
the exact prior 756-entry prefix. The later [terms integration](native-contract-terms-integration.md)
extends the installed game to 783 entries. The [combined integration report](native-contract-integration.md)
records real-service runtime, sanitizer, installation and preservation evidence.

## Original scope

| Entry | Exclusive end | Instructions | Operation |
| --- | --- | ---: | --- |
| `0x10078e70` | `0x10078ef8` | 34 | Bind a primary staff contract |
| `0x10078f00` | `0x10079048` | 82 | Reserve or reuse a contract row |
| `0x103727c0` | `0x10374570` | 1,900 | Initialize a staff contract |

All three complete bodies have pinned fingerprints and page fragments, no
accepted semantic owner and no indexed external entrance into an interior.
The index records nine incoming root edges. The row reservation diagnostic
identifies `CONTRACT_MANAGER.CPP` and
`CONTRACT_MANAGER::get_free_staff_contract_space()`.

## Reservation and binding

A zero signed mode byte scans existing 100-byte rows for a staff ID of -1 and
returns the first such index without incrementing the used count. A nonzero
byte appends directly. Values such as 256 retain the zero-byte path. The scan
rereads the live used count on each iteration.

Appending grows capacity by 200 only when used count equals capacity. It
stores the new capacity before requesting `capacity * 100` bytes, using the
original 32-bit wraparound. A populated controller reallocates; an empty one
allocates. A null result follows the original diagnostic sequence and returns
-1. The changed capacity and stored null pointer remain visible. Success
increments the used count and rereads it before subtracting one for the result.
No new capacity checks or allocation recovery rules are introduced.

Primary binding calls reservation with mode 1. A nonnegative signed result is
written into the staff statistics' primary index. The binder rereads the staff
ID, writes it into the selected contract row, reloads the controller's storage
pointer and invokes the full initializer with the original arguments. Negative
reservation results skip these mutations.

## Full initialization

The initializer prepares identity and flags, resolves the live club, selects
player or nonplayer ability, derives its rating and chooses existing or
calculated dates and numeric values. It retains both date paths, signed age
narrowing, current-date and threshold comparisons, optional profiles and the
original early exits. It is not a record copy operation.

Named phases evaluate the age/role option, three release options and three
profile bonus fields. Competition type and virtual callbacks determine release
eligibility where the original requires them. Callbacks retain their CFM ABI
and saved-TOC reload. Mode two sets the original contract flag; mode three
clears five options, three bonuses and the release amount before final storage.
The implementation does not infer unverified UI names for numeric fields.

Country rules read live selector variables and nation IDs. They retain squared
rating floors, reputation bands, mode-specific caps, scale curves and the
default country's competition, nationality, age and ability gates. Earlier
gates are correlated under ordinary inputs; explicit structural controls cover
the retained country phase and the apparently contradictory comparison at
`0x10374344`. These are not claims of natural career traversal.

Five complete 72-instruction range samplers share a helper only after original
assembly normalization proves equality across bound/result registers, stack
scratch and local branch relocation. The sixth preserves its distinct scratch
register order. Small and wide signed ranges, clipping, zero-range callers,
cursor advancement, wrap/reseed, shared salt and dependency `0x10374570` retain
their source order. That sampling dependency remains outside this block.

The initializer preserves its unusual below-stack FPR save, 19-GPR save area,
256-byte frame, restoration order, 45 direct field-write sites and original
single/double rounding. Compilation uses `-ffp-contract=off`. Optional record
flags on the shared shift and add-carry helpers implement exactly the three
original `srawi.`/`addze.` sites. Their previous default behavior is unchanged.

## Direct evidence and remaining integration

The initializer matrix passes **2,866 comparisons**: 1,192 complete agreements,
1,670 matching interruptions or faults and four rejected own/child negatives.
It requires all **1,900 initializer sites** and additionally reaches 72 sites
in the complete linked reservation/binding bodies. Its **1,576 comparisons
with explicit structural controls** are reported separately from ordinary
inputs. The binder-to-reservation-to-initializer path executes all three real
original/C++ operations, including capacity growth.

The focused isolated reservation/binding matrix passes **509 comparisons**:
237 complete agreements, 268 matching interruptions or faults and four
discriminating negatives. It covers all **116 helper sites** and retains a
stateful initializer fixture so allocation failures, wraparound and aliasing
remain isolated. The two matrices together cover all **2,016 source sites**.

Comparisons include complete architectural/temporary/mapped-memory and
allocator state, ordered effects, dependency input/output ABI, source order
and accounting. Only normal completion, guest budget exhaustion, explicit
dependency interruption and unmapped guest-memory faults are valid outcomes;
fixture assertions cannot count as matching faults. Representative paths
exercise four budget cuts, six sampled missing-byte boundaries and each
dependency interruption. Independent expectations check staff/club binding,
indices/counts, retained numeric value/end date and mode-specific clearing.

Cases include signed and floating boundaries, NaN/infinity, both date paths,
profile/competition guards, live callback mutation, every inline reseed site,
both sampler scratch variants, record-flag preservation, null faults and
complete nested calls. External date, value, rule, RNG and allocator services
are stateful component fixtures; actual-world coverage is recorded separately in the integration report.

Twelve Python guards pin all original instructions/page fragments, three tables
totaling 780 bytes, their pointers and two padding words. They reject altered,
missing, duplicate or reordered bodies, partial pages, ownership overlap and
external interior edges, including reversed contract order. They also verify
sampler normalization, the three record-flag sites and productive hook placement. Independent references
retain every original label and step with semantic hooks disabled. Named C++
functions do not replay generated instruction bodies.

Original data, references, mappings, fixture history and execution receipts
remain private under `.local/contract-storage-reconstruction`. Save/Load:
**not retested**. Performance: **not benchmarked**. The full reconstruction
goal remains active.
