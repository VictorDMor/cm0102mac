# Contract review, status and presentation

The complete **13-operation, 1,447-instruction** block below is implemented and
directly compared with the original. Prepared in `a3db7f3`, it is now
[integrated and installed](native-contract-review-integration.md) after the exact
783-entry prefix, reaching 796 entries. This report preserves the original
component evidence; the integration report records actual services, focused
sanitizer and installed-application acceptance. Offer generation, valuation and
negotiation remain outside this review block.

## Original boundaries

| Entry | Exclusive end | Instructions | Operation |
| --- | --- | ---: | --- |
| `0x1037f050` | `0x1037f110` | 48 | Reset review state |
| `0x1037f110` | `0x1037f588` | 286 | Query a review trigger |
| `0x1037f590` | `0x1037f7c4` | 141 | Refresh review conditions |
| `0x1037f7d0` | `0x1037f830` | 24 | Clear one review condition |
| `0x1037f830` | `0x1037f898` | 26 | Clear review conditions |
| `0x1037f8a0` | `0x1037fa48` | 106 | Add status bits |
| `0x1037fa50` | `0x1037fa60` | 4 | Clear status bits |
| `0x1037fa60` | `0x1037fa6c` | 3 | Reset status bits |
| `0x1037fa70` | `0x1037fa78` | 2 | Read status bits |
| `0x1037fa80` | `0x10380264` | 505 | Apply review conditions and consequences |
| `0x10380c10` | `0x10380d4c` | 79 | Describe squad status |
| `0x10380d50` | `0x10380f1c` | 115 | Describe release summary |
| `0x10380f20` | `0x103810d0` | 108 | Query the start-date window |

The indexed source has 379 incoming root edges, no existing owner for these
roots and no indexed external entrance into an interior. All complete bodies
and their fragments on three pages are pinned. The date-window end is the exact
next accepted engine-initialization boundary, without overlap.

## Review state and consequences

Status word `0x48` and review-condition word `0x50` have distinct operations.
Resetting the review clears condition and status state through the actual child
operations, sets the original byte markers and optionally calculates a packed
preference nibble. That calculation clamps its signed-byte result only above
ten, preserves the low nibble and retains null-context behavior.

The trigger query resolves staff and club from the record IDs. It retains both
club-pointer exemptions, zero conditions, the signed staff-ID remainder,
profile and signed-byte trait contributions, score bands, grouped condition
counts, status callbacks and ordered thresholds. It reloads live fields after
the original child calls. Individual condition meanings are not inferred from
bit positions alone.

Refresh retains the primary-contract query and its live global scratch pointer,
contract-kind and club checks, profile and byte-marker gates, trigger and
management queries. A successful refresh invokes its original service, updates
the original bytes and removes seven condition masks through complete nested
clear operations. Each clear re-enters refresh and then updates the staff side;
the changed marker prevents the same deep clear sequence from recurring.
Callback mutation can introduce the masks that the outer sequence subsequently
removes. Clearing all conditions retains its signed low-byte refresh mode.

Adding status bits preserves its signed diagnostic boundary, original role and
profile gates and follow-up service. The 505-instruction coordinator preserves
role filtering, condition writes, notification, both original club-rule queries,
primary staff trait adjustment, primary/secondary relationship draws and
optional notification. Names do not assign unverified trait, relationship or
club-rule meanings to those services.

Three complete 82-instruction inline samplers are shared only after original
normalization proves equality across local relocation, scratch layout and
result register. They retain zero/signed/wide bounds, pool cursor, wrap/reseed,
floating conversion, rounding, correction dependency `0x10374570` and clipping.
The source's third sample overwrites the earlier table register only after its
last table read. All original source sites remain in the independent reference.

## Presentation and start-date window

Squad status reads the low nibble of byte `0x60` and uses the original computed
table. Values zero through seven map to Undefined, Key, First Team, Rotation,
Backup, Prospect, Youngster and Not Needed. Larger nibbles also select Not Needed.
Partial-case and unknown computed continuations preserve original PC, CTR, LR
and return handling.

The release summary gives minimum-fee precedence. A nonzero signed byte at
`0x1d` is normalized to one before formatting, then reread after the callback.
For example, a stored value of two becomes one when described. Remaining paths
retain the exact nonpromotion/relegation combinations, value-two precedence,
manager-job field, empty text and their original return values. This operation
is stateful. Both description functions require a signed capacity of at least
20; smaller values execute the complete diagnostic, clear the original active
flag and return zero. Three eighteen-instruction diagnostic sequences share a
helper after original register/line normalization.

The start-date query constructs its scratch date and clears the optional
boundary output before checking the feature flag. Its active path compares a
live reference with the start date, calls the real age-at-date dependency and
chooses a threshold of three below age 28 or two otherwise, using the signed low
byte. It retains both interval calls, optional threshold output, signed-day and
unsigned-year comparisons and exact-boundary reporting. The user-facing policy
associated with this window has not been established by this component alone.

Three isolated guest-call helpers implement recorded register intersection,
register addition with carry and extended subtraction. Existing helpers and
defaults are unchanged. Complete comparisons cover the resulting flags,
temporary values and output behavior, including every age byte and upper bits.

## Direct evidence and remaining acceptance

The focused Release CTest passes **10,507 comparisons**: **9,445 complete
agreements**, **1,039 matching interruptions or unmapped-memory faults** and
**23 rejected own/child negatives**. The gate requires every one of the **1,447
source sites**. The **354 comparisons with explicit structural controls** are
separate from ordinary inputs, including retained wide/zero sampler paths that
signed-byte callers cannot ordinarily select.

Comparisons preserve complete architectural, temporary, mapped-memory and
allocator state, ordered effects, dependency input/output ABI, source order and
instruction accounting. Only completion, guest-budget exhaustion, explicit
dependency interruption and unmapped guest-memory faults are accepted outcomes;
fixture assertions cannot count as agreement. Representative paths use four
budget cuts, six sampled missing-byte boundaries and every dependency
interruption. Own negatives cover all roots; child negatives cover every root
with calls in its baseline path.

Cases include correlated traits/flags, profile and club exemptions, all squad
and release bytes, diagnostic sizes, callback mutation, complete nested clearing,
every reseed site, aliased output, relocated TOC/dispatch tables, partial and
unknown continuations, signed dates, optional outputs and exact window equality.
Independent expectations check packed preference clamping, composed clearing,
status operations, callback rereads and age/window boundaries. External services
remain stateful component fixtures, including the previously accepted sampler;
this is not actual-service or career-UI evidence.

Eleven Python guards pass. They pin all bodies/fragments, three tables totaling
1,716 bytes with their live pointers, independent reference labels/steps with
hooks disabled, diagnostic and sampler normalization, complete ownership and
external-edge boundaries. Changed, missing, duplicated, reordered or partial
original bodies and mutated table/pointer bytes are rejected. Named C++
operations do not replay generated bodies. Warnings are errors and compilation
uses `-ffp-contract=off`.

The first matrix detected an extra secondary-staff pointer read; removing that
read restored original source/effect order. The expanded matrix then agreed
but lacked fourteen correlated source sites. Additional actual input
combinations cover those sites without changing the semantic bodies. Earlier
diagnostic runs do not count as accepted complete coverage.

Original data, references and receipts remain private under
`.local/contract-review-reconstruction`. The subsequent
[integration milestone](native-contract-review-integration.md) adds productive
adapters, manifest selection, generation, actual-service comparisons, focused
sanitizer acceptance and installation. Its receipts remain separately identified.
Save/Load is **not retested**. Performance is **not benchmarked**. The cumulative
Portuguese guide explains this cycle. The full reconstruction goal remains active.
