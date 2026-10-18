# Transfer-offer record lifecycle, persistence and value

The complete **eight-operation, 806-instruction** record block, prepared in
`d94ea3c`, is now [integrated and installed](native-transfer-proposal-integration.md)
alongside the eight valuation/exchange operations. Their sixteen unique owners
extend the exact accepted 815-entry prefix to **831 installed entries**. The
component evidence below retains its original scope.

## Complete boundaries

| Entry | Exclusive end | Instructions | Named operation |
| --- | --- | ---: | --- |
| `0x10411e60` | `0x10411ea0` | 16 | `construct_empty_offer` |
| `0x10411ea0` | `0x10411f08` | 26 | `construct_generated_offer` |
| `0x10411f10` | `0x10411f9c` | 35 | `destroy_offer` |
| `0x10411fa0` | `0x104123d4` | 266 | `read_offer` |
| `0x104123e0` | `0x104126cc` | 185 | `write_offer` |
| `0x104163d0` | `0x104164e4` | 69 | `initialize_zero_fee_offer` |
| `0x104164f0` | `0x10416710` | 136 | `initialize_priced_offer` |
| `0x10416710` | `0x10416834` | 73 | `offer_value` |

All 94 indexed incoming root edges are recorded privately; no external edge
enters a body interior. Constructor/destructor, raw record IO, two explicit
initializers and value composition form one record boundary. The delegating
constructor calls automatic terms synthesis at `0x104126d0` through an explicit
dependency boundary; that larger service and negotiation policy remain generated.
The kind byte set to 1 by the zero-fee initializer is not assigned an unverified
player-facing label.

## Behavior

The empty constructor initializes its date subobject and two list pointers,
leaving other fields untouched. The delegating constructor initializes the date,
passes its six saved arguments to the synthesis service with a null output, and
returns the original object regardless of the service result.

Destruction treats the group list as shared: a one-item group is destroyed;
other nonnull groups remove the current index stored at object+0x28. The exchange
list is destroyed separately. The object's storage is released only when the
signed low halfword of the destructor argument is positive. Null objects and
original pointer/partial-effect behavior are retained.

Persistence transfers an **84-byte raw header**, a signed-halfword group count
with four-byte indices, then a signed-halfword exchange count with eight-byte
payloads. Reads can recover a shared group from an earlier indexed record;
otherwise they adopt a newly built list. A temporary list used during relinking
is cleared and disposed separately. A zero group count preserves the header's
raw group pointer; zero exchange count explicitly clears its pointer. Negative
nonzero counts still take the allocation branch but run no element iterations.
Short IO clears the original diagnostic flag and retains the original partial
state; a missing earlier reference is diagnosed without necessarily stopping
all subsequent reads. The operation does not add rollback or input clamping.

Explicit initialization retains read/write order, IDs, amount bits, sentinel
indices, packed-nibble preservation and diagnostics. Priced initialization
queries the contract twice and updates shared scratch storage. Its same-division
path advances the shared RNG cursor, retains rollover/reseed order and the
signed remainder-by-20 and skill-greater-than-7000 predicate. A changed second
lookup can fault at zero; the reconstruction retains that fault prefix.

Value composition starts with the base amount. Its default mode adds only
positive signed installments and a positive signed-byte percentage of the main
player's stored value when enabled. Products wrap at 32 bits before signed
truncation toward zero. Exchange inclusion has a separate flag. The usual path
adds the stored values of the exchanged players. Another mode calls the
valuation service with the **main target for every exchange item**, preserving
that original argument rather than silently substituting each item's player.
List cursors and record pointers are reread in source order.

## Component acceptance

The focused Release CTest passes **6,150 comparisons**: 1,876 complete agreements,
4,258 matching interruptions/memory faults and sixteen rejected negatives.
Eight own-operation negatives and eight dependency negatives are discriminating;
the pure successful zero-fee initializer uses its diagnostic path for the latter.
Every one of the 806 source sites is required against an independent exact site
list. Full architecture/temporaries, mappings and allocation bookkeeping,
ordered memory effects, dependency entry/return registers, source order,
accounting and fixture IO streams agree. Fixture assertion failures cannot count
as accepted agreement.

Cases cover signed counts/destructor arguments/flags/amounts, percentage bytes,
overflow, strict skill/RNG boundaries and rollover, missing/changed contracts,
shared/empty/new lists, allocation failure, each IO failure position and partial
transfer, aliased initialization inputs, mutable lists and IO callbacks, relocated
TOC/text tables, measured budget cuts, sampled missing bytes and each dependency
interruption. A complete in-memory record-stream roundtrip independently checks
raw bytes and both list payload words; reconstructed pointer words may differ.
This is fixture-stream evidence, not a new game Save/Load claim.

Thirteen comparisons from one mapped-zero structural input complete the changed
second-contract-lookup path. The same unmapped lookup separately proves the
original fault. This structural path is not natural career execution.

The initial ten source/data guards pinned the then-complete 815-entry prefix, absent ownership,
boundaries/fragments, fields/order, interior edges, a live 38-byte diagnostic
table/pointer, fourteen normalized complete diagnostics and all original labels,
steps and required sites. Every semantic hook is disabled in the independent
reference. The four named semantic units never replay original bodies and do not
modify the shared semantic-call helper. Warnings are errors and contraction is
disabled with `-ffp-contract=off`.

## Delivery boundary

The consolidated integration links these units through `cm_aot`, audits
productive hooks and includes actual-service, focused sanitizer and installed-app
acceptance. The semantic bodies and common helper are unchanged. The original
component references/table payloads remain private under
`.local/transfer-record-reconstruction`; integration receipts live under
`.local/transfer-proposal-integration`.

Save/Load is **not retested**; performance is **not benchmarked**. The cumulative
Portuguese PDF adds practical explanations of shared ownership, record persistence
and value composition. Whole-game reconstruction remains active.
