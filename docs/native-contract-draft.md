# Contract draft initialization and preparation

Status: **integrated and accepted at the 871-owner milestone**. Three complete
native operations match independent original references and execute on the
productive path. Actual original-service comparisons, completed recruitment and
proposal-synthesis children, focused UBSAN and protected installation pass.
See [integration evidence](native-contract-draft-integration.md). The complete-game
goal remains active.

## Scope and provenance

| Entry | Exclusive end | Indexed instructions | Native operation |
| --- | --- | ---: | --- |
| `0x103721f0` | `0x10372254` | 25 | `initialize_draft` |
| `0x10372260` | `0x103723b0` | 84 | `prepare_new_contract` |
| `0x103723b0` | `0x1037258c` | 119 | `prepare_from_primary_contract` |

The original index has 46 incoming root edges and no external interior entry.
All 228 indexed instructions are retained. Four nonindexed zero alignment words
are pinned separately. The 39-byte diagnostic bank and its TOC pointer are pinned;
its original bytes are not embedded in the native operations.

This is the complete three-root creation/preparation boundary before the adjacent
presentation functions. The default constructor is called by the reconstructed
recruitment and proposal-synthesis paths. Preparation shares the existing terms,
review, query and Date services. Their already-owned entries are retained rather
than counted again. The separate full contract initializer `0x103727c0`, already
owned by `contract_storage_adapter`, is outside this block.

## Observed behavior

The default constructor receives its destination in r3 and returns that pointer.
It calls the existing default-date service twice, for records at `+0x28` and
`+0x30`. It then writes two `-1` sentinels and selected zero fields, including word
`+0x08`, status words and review flags. It does not initialize the
entire 100-byte object: the first ID, value fields, clause storage and padding
can retain prior bytes. A blanket clear would change the original behavior.

New preparation receives destination, staff and club in r3/r4/r5 and a duration
in r6. It retains only the duration's low unsigned byte before passing it to the
existing weeks service. It stores staff/club IDs, two value fields of 200, a
five-byte copy of today's date and the returned five-byte end date. It inserts
kind five into bits `+0x38[7:2]`, preserving the low two bits. Review preparation,
five option clears, three bonus clears and a zero release amount follow. The
stores around the first option call retain their original order.

Primary-contract preparation receives destination, staff, club and end-date
pointer in r3/r4/r5/r6, a percentage byte in r7, and two option bytes in r8/r9.
It queries the primary contract first. On success, it multiplies contract word
`+0x0c` by the **signed** low byte of r7, retaining only the wrapped 32-bit product
before dividing by 100 toward zero. It stores that result in both value fields,
copies today's and the supplied end date, inserts kind six while retaining the
low two packed bits, prepares review state, clears the clause arrays, and then
writes the low bytes of r8/r9 to the first two option fields.

For example, a supplied raw percentage byte of 200 is read as `-56`; with a value
of 1,000 the prepared result is `-560`. Extreme inputs also distinguish the
wrapped product from a wider multiply followed by division. These are controlled
debug inputs, not a claim about normal career percentages or a currency.

When primary lookup returns zero, the function calls four diagnostic services,
clears the global diagnostic byte and returns without modifying the destination.
If those services return, r3 contains the diagnostic flag address, rather than a
normalized zero or destination pointer. Staff and club inputs on the successful
paths are mandatory: positive null-access faults remain contractual.

Date copies preserve their five-byte read/write order and leave adjacent padding
alone. Output aliases with staff, club, the current date, the supplied end date
or the returned primary contract are therefore observable. The final dependency
return residue is retained; successful preparation does not add an extra
destination assignment at return.

## Component evidence

The component performs **2,491 comparisons**: 2,431 complete returns, 54 matching
stops and six rejected deliberate corruptions.

| Root | Comparisons | Complete returns | Matching stops | Negatives | Complete-return sites |
| --- | ---: | ---: | ---: | ---: | ---: |
| Default draft | 22 | 8 | 12 | 2 | 25 |
| New preparation | 363 | 346 | 15 | 2 | 84 |
| Primary-contract preparation | 2,106 | 2,077 | 27 | 2 | 119 |

All 228 original sites occur in complete returns, without structural exceptions.
Each comparison checks all architectural registers, temporary residues, mapped
memory bytes, ordered reads/writes including instruction PCs, source order,
dependency entry/return ABI and instruction accounting. The reference disables
all discovered productive hooks and retains each original instruction. Native
operations use named C++ stages and a shared clause-reset tail; they do not replay
original bodies, interpret instructions or dispatch by PC.

Calendar, lookup, review and diagnostic dependencies have explicit external
models. The accepted native `clear_option`, `clear_bonus` and
`set_release_amount` leaf bodies run on both sides of their dependency boundary;
their original proofs are not redefined by this component. This is isolated
component evidence, not actual original-service runtime proof.

Inputs cover all 256 packed input bytes for both preparations, unsigned duration
bytes and high-bit residues, signed day/unsigned year/flag forwarding, all 256
percentage bytes against seven value frontiers, final option bytes, missing
primary contracts, aliased inputs, final service-result forwarding, measured
budget cuts, removed accessed bytes and interrupted callees. Independent checks
cover default sentinels/unwritten bytes, packed-bit preservation, five-byte date
forwarding, wrapped signed arithmetic and the missing-contract return residue.

Each root rejects an own-result corruption and an extra memory effect restored
before return. Equal final images are required for the effect negative, so an
image-only comparator cannot pass it. Ten source/data checks pass, covering productive hook placement, the
exact unchanged 868-owner prefix, complete bodies, owned dependency boundaries,
every pinned byte, external interior-entry rejection and independent references.
The focused CTest `contract_draft_component_equivalence` passes on ARM64 with
`-ffp-contract=off`, `-Wall -Wextra -Werror -Wpedantic`.

The cumulative Portuguese guide, edition 33, records this block and its
productive integration. Actual-service coverage reaches 206 selected sites;
the isolated component retains all 228 in complete returns. The missing-primary
diagnostic path retains explicit-model component evidence. Save/Load is
**not retested**; performance is **not benchmarked**. The complete-game goal
remains active.
