# Ball-contact dependency reconstruction

Status: **five complete entries with component acceptance; production integration pending the complete contact block**.

The installed production manifest remains at 678 semantic entries, including the
accepted ball-action coordinator. This work reconstructs six direct contact
dependencies, totaling 2,043 original instructions. Five entries are implemented
and exercised in isolation; the 1,404-instruction description entry is next.
These candidate components are linked only into their conformance executables.

## Complete original boundaries

| Operation | Original range (exclusive end) | Instructions | State |
| --- | --- | ---: | --- |
| Contact screening facade | `0x101f5050..0x101f50b8` | 26 | Component accepted |
| Contact attempt facade | `0x101f50c0..0x101f5110` | 20 | Component accepted |
| Contact application facade | `0x101f5110..0x101f5180` | 28 | Component accepted |
| Contact resolution facade | `0x101f5180..0x101f51e0` | 24 | Component accepted |
| Action-contact description | `0x101f5350..0x101f6940` | 1,404 | Pinned original; implementation pending |
| Failed-contact reaction | `0x10270320..0x10270b94` | 541 | Component accepted |
| **Complete block** | **Six entries** | **2,043** | **639 instructions implemented** |

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

## Component evidence

The focused normal and UndefinedBehaviorSanitizer matrices cover:

- 2,645 facade comparisons: 1,468 completions, 1,169 interruptions and eight
  discriminating own-result/dependency negatives;
- 18,818 failed-contact comparisons: 2,007 completions, 16,809 interruptions,
  two discriminating negatives and 5,682 paired structural cases;
- all 639 implemented original instruction sites, complete architectural state,
  mapped memory, ordered accesses, dependency ABI, original-site order and exact
  instruction accounting;
- signed/unsigned narrowing, all raw trait-byte values, aliases, zero/unaligned
  pointers, caller-result preservation, random renewals, numeric extrema,
  infinities and NaNs;
- every instruction-budget boundary, each observed memory byte removed in turn,
  and interruptions at every reached dependency call.

The paired wide/zero second-interval injections are structural evidence and are
not claimed as naturally reached gameplay. Five Python audit tests cover the
complete scope, altered/missing/reordered source rejection, reference labels,
three live-table contracts and unchanged production selection.

Build `cm-ball-contact-facade-checks` and `cm-ball-contact-failure-checks` with the
private `CM_ORACLE_DIR`. Run `ball_contact_facade_equivalence` and
`ball_contact_failure_equivalence`. Public audit entry point:
`python3 -m unittest discover -s tools -p test_ball_contact_audit.py -v`.

## Remaining acceptance

Reconstruct the complete action-contact description at `0x101f5350`, retaining
its three mutable tables, messages, two-player iteration and floating save/restore
contracts. After all six bodies have component evidence, integrate their actual
entry labels, original/profile controls, nonlocal transfers and shared-engine
acceptance. Verify the installed app identities and a complete match again.

The five component results do not increment the production entry count and do
not establish new runtime behavior for the installed apps. Save/Load is **not
retested**. No performance improvement is claimed. The complete-port goal remains
active beyond this contact block.

Cycle cleanup removed 4 reproducible conformance executables and their debug
symbols (10 files, 577,715,454 logical bytes). Available space
increased by 576,786,432 bytes during cleanup. All 845 protected files, current
installed/candidate apps, prior app references, reusable production objects and
library, original inputs and independent references were verified intact.
