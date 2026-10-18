# Candidate contribution and club-interest assessment

Two complete original operations have been reconstructed and compared as an
isolated component. They cover **5,797 indexed instructions**. Production remains
at the accepted **857 installed owners**: these two entries have no production
adapter or replacement hook. A later consolidated integration would reach 859
only after preserving that exact prefix and establishing actual-service proof.

## Complete boundaries

| Entry | Exclusive end | Instructions | Operation |
| --- | --- | ---: | --- |
| `0x10361490` | `0x10362d6c` | 1,591 | `assess_candidate_contribution` |
| `0x10369080` | `0x1036d238` | 4,206 | `evaluate_club_interest` |

There are 41 indexed incoming root edges, nine and thirty-two respectively, and
no external interior entrance. Full bodies, seven page fragments, three zero
alignment words and seven data regions are pinned. Data includes the 1,120-byte
floating bank and the complete **44-byte single-precision prefix**, through
its original +0x28 read. The smaller twelve-byte prefix used by an earlier
recruitment component does not cover this operation's accesses.

The adjacent loan/date/offer controller `0x103636b0` and the independently called
floating evaluator `0x10366470` remain separate candidates. This block does not
claim the whole recruitment or transfer subsystem.

## Contribution assessment

The first operation receives the manager object, club, staff, shortlist row,
auxiliary output pointer, reference integer and signed cap flag in r3-r9. Its
integer result is returned in r3. It resolves an associated reference, selects
qualification/classification masks, adjusts thresholds against existing interest,
selects coefficients and a baseline, applies field-indexed curves and country
scales, and performs the original floating-to-integer conversion.

The auxiliary result is written before some rejection paths. Missing associated
references can be selected lazily and cached using the shared random cursor.
An independent assertion confirms that a negative supplied estimate returns
zero while retaining the written auxiliary value, cached reference and cursor
advance. This is not a transaction with universal rollback.

Thresholds and curves retain signed low-byte/halfword interpretation, fixed
classification tables, ordered service calls and single/double rounding. Raw
staff+0x5c, profile+10/+12 and club+0x1ec are not assigned unverified UI meanings,
ages, reputation labels or currencies.

## Club-interest assessment

The second operation receives manager, staff, club, signed mode, request option,
category and optional reason-mask pointer in r3-r9. It returns a floating score
in f1. It can clear and update the reason mask, publish the primary-contract
scratch pointer and write date/scratch objects. An early score rejection does
not undo preceding effects.

The operation resolves primary and secondary contracts and the current club,
then applies relationship services, national contexts, affiliated-club links,
contract preferences and the category tables. It combines reference values from
the candidate and current clubs, including optional associated records and
original weighted arithmetic. Contract flags, elapsed dates and request options
can change both the score and its recorded reasons.

A later financial refinement requires a nonzero signed mode byte, a zero request
option and an initial score of at least **1.75**. Separate services supply the
quote and financial reference. The original 0.65 affordability guard, later 0.7
ratio threshold, bounds, rounding and rejection paths remain distinct. A debug
viewer can eventually expose these intermediates and reason bits once a
production integration validates the real services; no new gameplay rule or UI
control is introduced here.

## Independent component evidence

The isolated reference retains every original instruction and disables every
productive hook. Both complete operations run from their real entry points on
cloned states. The harness compares architecture and temporaries, every mapped
byte, ordered memory effects, complete dependency entry/exit registers and
continuations, original instruction order and exact accounting.

| Entry | Comparisons | Complete returns | Matching stops | Rejected negatives |
| --- | ---: | ---: | ---: | ---: |
| Contribution | 9,936 | 8,673 | 1,261 | 2 |
| Interest | 12,218 | 11,226 | 990 | 2 |
| Total | **22,154** | **19,899** | **2,251** | **4** |

All **1,591 contribution sites** and **4,189 interest sites** execute in complete
returns: **5,780 sites** in total. The remaining **17 original instructions** are
retained and covered by separately checked source/data invariants:

- Ten reason-mask instructions follow impossible bounds of `1 - signed_byte/400`.
  That factor lies between 0.6825 and 1.32, inside the original 0.5 and 1.5 guards.
- Three null-current-club arms lie inside the nonzero branch of the preceding
  stable-register guard.
- One lower-reference arm requires a factor above one after its original ceiling
  of one, including the original single-precision rounding and unordered exit.
- Three financial-refinement instructions contradict the preceding 1.75 entry
  threshold while f31 remains unchanged by the body and ABI-preserving children.

The generated reference validates these premises against pinned original bodies
and data before compilation. Dynamic and structural sets must be disjoint and
partition all 5,797 indexed sites. Structural proof is not runtime coverage.

Cases include bounded deterministic object/result combinations, meaningful
threshold perturbations derived from complete entry inputs, relocated TOCs and
constant tables, shared-output aliases, signed limits, NaN payloads, infinities,
signed zero, floating status/exception behavior, budget boundaries, pointer holes
and interrupted dependency calls. A mapped-zero object is an explicit separate
fixture; ordinary unmapped null accesses retain their original faults. Six
specified null-record fault sites exercise declared partial-record combinations;
unexpected fixture faults are rejected. Negative controls independently alter
one root result and one child memory effect for each entry.

Dependency services are explicit fixtures. Register save/restore and the compiler
conversion reuse existing semantic modules. The fixture date arithmetic uses a
declared 365-day model, not the real calendar implementation. Other callbacks
supply controlled results, clobber volatile registers, preserve nonvolatile ABI,
and retain declared TOC and memory effects. They do not establish actual-service
or installed-game behavior.

## Validation and delivery boundary

The isolated target `cm-shortlist-assessment-checks` built with warnings as
errors and floating contraction disabled. Its direct component run passed, and
CTest registers `shortlist_assessment_component_equivalence`. Eleven focused
source/data checks passed. Shared guest support, floating support, earlier
recruitment bodies and the productive owner manifest remain unchanged.

Productive generation, actual-service integration, focused UBSAN and app
installation are deferred to the consolidated milestone. Save/Load: **not
retested**. Performance: **not benchmarked**. The whole-game goal remains active.
