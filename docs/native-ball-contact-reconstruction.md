# Ball-contact dependency reconstruction

Status: **all six complete entries have component acceptance; production integration and runtime acceptance pending**.

The installed production manifest remains at 678 semantic entries, including the
accepted ball-action coordinator. This work reconstructs six direct contact
dependencies, totaling 2,043 original instructions. All six entries are implemented
and exercised against independent original references, including the complete
1,404-instruction description from entry through return.
These candidate components are linked only into their conformance executables.

## Complete original boundaries

| Operation | Original range (exclusive end) | Instructions | State |
| --- | --- | ---: | --- |
| Contact screening facade | `0x101f5050..0x101f50b8` | 26 | Component accepted |
| Contact attempt facade | `0x101f50c0..0x101f5110` | 20 | Component accepted |
| Contact application facade | `0x101f5110..0x101f5180` | 28 | Component accepted |
| Contact resolution facade | `0x101f5180..0x101f51e0` | 24 | Component accepted |
| Action-contact description | `0x101f5350..0x101f6940` | 1,404 | Component accepted |
| Failed-contact reaction | `0x10270320..0x10270b94` | 541 | Component accepted |
| **Complete block** | **Six entries** | **2,043** | **2,043 instructions implemented** |

`tools/ball_contact_audit.py` pins every field of each complete original body and
rejects external static interior entrances. The description contains three live
computed tables: 48, 10 and 9 words, with 13, 10 and 9 distinct destinations.
All 67 original words and three relocated pointers are pinned before emitting
any reference. Table pins describe the original input; they do not authorize
replacing mutable guest tables with host mappings.

References preserve every original label and instruction checkpoint, with all
semantic hooks and guest-context transfer adapters disabled. Original inputs,
reference bodies and runtime evidence remain private.

## Implemented behavior

The four facades preserve the complete original spill/reload order, 64-byte
frames, match-context offset and dependency destinations. The attempt facade
narrows its third argument to a signed halfword. The other facades retain their
unsigned byte arguments, full-word arguments and original unused registers. The
application facade reads the ninth incoming byte and writes the original outgoing
stack word before calling its dependency. Every facade retains the complete
returned word and restores the caller's stack and link.

The failed-contact reaction preserves the signed score gate, single-precision
profile/attribute sum, integer conversion, zero and wide interval paths, random
twentieth gate, signed trait selection, separate report IDs, parity/third samples
and three forms of the original match request. Its softer path performs the
ordered signed halfword decrement, a second interval sample and conditional
reaction/reset. Random renewals retain the original registers and captured
trait order. The 112-byte frame and all eight nonvolatile integer registers are
restored on normal return.

The action-contact description preserves both players' random samples, record
queries, fitness/profile/context weights, selected-player early exit and paired
iteration. All three tables are read from live guest memory. The kind-five
assessment retains separate single-precision rounding and the original random
threshold. Selected and fallback reactions retain their complete argument and
scratch-register effects, including the kind-two fallback that falls through to
an additional kind-five callback. The root owns its 192-byte frame across all
phases and restores eleven nonvolatile integer and five floating registers.

The receiver-weight gate intentionally reads the initiating player's profile:
a null initiating player can fault even with a nonnull receiver. Arithmetic
matches the pinned indexed reference, including separate multiply/add rounding;
this is not an independent claim about physical PowerPC floating-point behavior.

## Component evidence

The focused normal and UndefinedBehaviorSanitizer matrices cover:

- 2,645 facade comparisons: 1,468 completions, 1,169 interruptions and eight
  discriminating own-result/dependency negatives;
- 18,818 failed-contact comparisons: 2,007 completions, 16,809 interruptions,
  two discriminating negatives and 5,682 paired structural cases;
- 177,348 description comparisons in each of Release and UBSAN: 31,010
  completions, 146,323 interruptions and 15 discriminating negatives. These
  include separate phases and complete entry-to-return executions;
- all 2,043 original instruction sites across the six entries, complete architectural state,
  mapped memory, ordered accesses, dependency ABI, original-site order and exact
  instruction accounting;
- signed/unsigned narrowing, all raw trait-byte values, aliases, zero/unaligned
  pointers, caller-result preservation, random renewals, numeric extrema,
  infinities and NaNs;
- every instruction-budget boundary, each observed memory byte removed in turn,
  and interruptions at every reached dependency call;
- every raw description style byte, both player positions, every canonical live
  table destination and remapped table rows, externally redirected destinations,
  mutable table pointers, null players, floating extrema and enabled exceptions.

The combined component matrices contain 198,811 comparisons per build mode.
UBSAN instruments the component implementations, harness and generated references;
the existing shared runtime library is reused. These are component observations,
not captures from an installed game.

The paired wide/zero second-interval injections are structural evidence and are
not claimed as naturally reached gameplay. Five Python audit tests cover the
complete scope, altered/missing/reordered source rejection, reference labels,
three live-table contracts and unchanged production selection.

Build `cm-ball-contact-facade-checks`, `cm-ball-contact-failure-checks` and
`cm-ball-contact-description-checks` with the private `CM_ORACLE_DIR`. Run
`ball_contact_facade_equivalence`, `ball_contact_failure_equivalence` and
`ball_contact_description_equivalence`. Public audit entry point:
`python3 -m unittest discover -s tools -p test_ball_contact_audit.py -v`.

## Remaining acceptance

Integrate all six actual entry labels with dedicated counters, original/profile
controls, nonlocal transfers and shared-engine acceptance. Compare a complete
real match against the original routines, measure paired simulation timings,
and verify the installed app identities and completion. Component equivalence
alone does not satisfy these gates.

The six component results do not increment the production entry count and do
not establish new runtime behavior for the installed apps. Save/Load is **not
retested**. No performance improvement is claimed. The complete-port goal remains
active beyond this contact block.

The earlier five-entry cycle cleanup removed 4 reproducible conformance executables and their debug
symbols (10 files, 577,715,454 logical bytes). Available space
increased by 576,786,432 bytes during cleanup. All 845 protected files, current
installed/candidate apps, prior app references, reusable production objects and
library, original inputs and independent references were verified intact.
