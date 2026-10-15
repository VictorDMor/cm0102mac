# Ball-action coordinator reconstruction

Status: **partial C++23 implementation with component acceptance; production replacement pending**.
Accuracy and complete behavioral conversion take priority over optimization.
The original coordinator still runs in production, and the accepted production
manifest remains at 677 semantic entries. These components are linked only into
their conformance executable, not into `cm_aot` or either app.

## Complete original contract

The complete entry is `0x1023cd20..0x10244c58` (exclusive), containing 8,142
contiguous indexed instructions across nine generated pages. The function
inventory's 30,564-byte `body_bytes` field truncates a continuing body; it is not
the replacement boundary. The common return is at `0x10244c38..0x10244c58`.

The canonical original-body SHA-256 is
`5e7dc184cdba66892eebd01d5cf77e027b4195623d09b73b30074125b4fd3646`.
`tools/ball_action_audit.py` pins every fragment and the five original relocated
decision tables. Their word counts are 47, 57, 47, 10 and 47, with 12, 12, 12, 3
and 7 distinct destinations respectively. Table pins characterize the original
input; the eventual coordinator must still read mutable guest tables at runtime.
External static interior entrances are rejected.

Each component reference is emitted only after the complete body and tables
pass their pins. Every original label and checkpoint is retained, with semantic
hooks and guest-context transfer adapters disabled. Private original inputs,
generated reference bodies and runtime artifacts remain outside version control.

## Validated components

| Component | Original range (exclusive end) | Instructions |
| --- | --- | ---: |
| Frame, context and fitness sampling | `0x1023cd20..0x1023d184` | 281 |
| Orientation, height and surface adjustments | `0x1023d184..0x1023d344` | 112 |
| Facing choice | `0x1023d344..0x1023d5e4` | 168 |
| Area reaction | `0x1023d5e4..0x1023d8f8` | 197 |
| Elevated ball-state selection | `0x1023d8f8..0x1023de18` | 328 |
| Low ball-state selection | `0x1023de18..0x1023e404` | 379 |
| Selected-action adjustments and fitness dependency | `0x1023e404..0x1023ea44` | 400 |
| Contact screening and immediate-action exit | `0x1023ea44..0x1023ecf4` | 172 |
| Contact classification and area reaction | `0x1023ecf4..0x1023f1e8` | 317 |
| Contact outcome, recipient and messages | `0x1023f1e8..0x1023fa5c` | 541 |
| Execution recipient, screening and warning | `0x1023fa5c..0x1023ff48` | 315 |
| First live execution-message table | `0x1023ff48..0x10240178` | 140 |
| Receiver-contact classification and area reaction | `0x10240178..0x1024060c` | 293 |
| Receiver outcome, restart selection and messages | `0x1024060c..0x10240ff8` | 635 |
| Execution-result contact attempt, warning and cancellation | `0x10240ff8..0x10241454` | 279 |
| Second live execution-message table | `0x10241454..0x10241680` | 139 |
| Execution-result classification and initiating-player reaction | `0x10241680..0x10241b24` | 297 |
| **Validated contiguous prefix** | **`0x1023cd20..0x10241b24`** | **4,993** |

The named components preserve live reloads, signed-byte/halfword narrowing,
wrapped word arithmetic, reciprocal division, floating-point status, scratch
spills, random cursor advancement, dependency ABI and original instruction
accounting. Identical word-interval sampling schedules and area predicates share
semantic helpers; the distinct halfword adjustment schedule remains separate.
`SemanticGuestCall::sign16` now supports the original record form while keeping
the existing non-record behavior as its default.

Contact screening compares the live single-precision expression with its
mutable zero baseline before integer truncation, reloads it around random
renewal and preserves both the immediate-action and contact-resolution exits.
Contact resolution preserves raw versus signed flag tests, area status changes,
wrapped actor halfword updates, ordered reports and conditional actor reactions.
The outcome component updates ball coordinates and ownership, reloads opposing
team links, selects a recipient, preserves repeated eligibility calls and emits
the original message sequence. Its exit is the shared return at `0x10244c38`;
the return itself is not yet implemented by this candidate.

Execution preparation preserves the ordered temporary actor link, recipient
selection and notification, clearing through the live owner, null-recipient
exit, conditional contact attempt, receiver warning and immediate-action exit.
The first message table reads its current TOC pointer and indexed word. All 47
entries and all 12 message destinations are exercised, including substituted
destinations, relocated/unaligned pointers, aliases, read-time pointer mutation,
default-path table bypass, and direct message entrances. Unrecognized targets
remain explicit guest continuations for the future complete coordinator; these
component checks do not claim execution beyond those continuation boundaries.
The fixture table is generated privately from the pinned original input.

Receiver resolution distinguishes contact five from six, uses the initiating
actor's inverted side for the area test, retains that actor in the match link,
and applies the wrapped decrement and reaction to the selected receiver. These
differences from the earlier initiating-actor contact are preserved separately.

Receiver outcome keeps the initiating actor, contacted receiver and selected
restart player distinct. It preserves raw-side ball ownership, blocked-action
messages, signed message gates, receiver marking, opposing-team reloads,
repeated initiating-actor eligibility calls, restart-player message identity,
random-dependent report order and the receiver's wrapped halfword decrement.
The common return remains an explicit continuation boundary.

Execution-result preparation clears an existing contact without consuming a
random sample. A new contact uses the signed score/rating comparison and a
conditional random fifth; its warning queries the initiating player's area,
then samples the receiver's signed halfword. The cancellation tenth reports
the current contact and clears it before resolution. The second table reads
its own live 57-word mapping from the saved action register, with the initiating
player before the receiver in its message ABI. All twelve message destinations,
mutable targets, direct entrances and explicit continuation boundaries are checked.

Result classification uses the initiating side without inversion. Major contact
retains the receiver in the match link, subtracts 500 from the initiating
player's wrapped halfword, and samples that player's reaction. This differs
from receiver resolution in both ownership and side selection. Root-prefix
fixtures independently configure the two contact attempts and require actual
entry into the second resolver, rather than accepting an earlier exit as its
composition evidence.

The conformance harness checks each component individually and every successive
prefix from the original root. Dependencies are opaque deterministic fixtures
that record full register state before and after calls, clobber volatile
registers, produce observable writes, and support interruptions and mutations.
This checks caller contracts; it does not establish new dependency or production
runtime acceptance.

## Component evidence

The current check contains 259,341 comparisons:

- 34,042 successful component or prefix completions;
- 225,265 instruction-budget, memory-hole or dependency interruptions;
- 34 rejected own-result/dependency negative controls;
- all 4,993 original instruction sites, including dormant branches;
- complete architectural state, mapped memory, ordered accesses, dependency ABI,
  original-site order and instruction accounting comparisons.

The same checks pass in the normal C++23 build and a focused UndefinedBehaviorSanitizer
build with floating-point contraction disabled. Eight Python audit tests check
whole-body/fragment integrity, truncated and reordered input rejection, all 208
table words and five pointers, isolated component labels/checkpoints, external
interior edges, and the unchanged production manifest. Record-form narrowing
also checks all 256 initial condition bytes, both overflow bits and three signed
halfword classes at the first interruption boundary.

28,367 of the comparisons use paired checkpoint injections to reach dormant
wide-interval or zero-threshold branches. These are structural equivalence
evidence, not naturally reachable gameplay coverage. Other cases include live
dependency mutations, aliases, null and unaligned pointers, random renewal,
signed extrema, infinities, NaNs, and attribute fields that alias earlier sample
intervals. No timing improvement is claimed.

Build the focused target in a build configured with the private `CM_ORACLE_DIR`,
then run `ball_action_component_equivalence`. The public audit entry point is
`python3 -m unittest discover -s tools -p test_ball_action_audit.py -v`; private
contract tests explicitly skip when original inputs are unavailable.

## Remaining acceptance

Continue at `0x10241b24` with the execution-result outcome and unopposed action.
The remaining 3,149 original instructions, three further computed
decision tables, tail/error paths and common return still require reconstruction
and equivalence evidence.

Before installing the replacement, validate the complete root, nonlocal transfers
and faults, hook its generated destination label, prove dedicated production
entry counters and shared-engine behavior, and verify the installed ARM64 app
identity. Partial component results do not increment the production entry count.
Save/Load is **not retested** in this component checkpoint.

The installed apps remain the accepted team-line build, and all 845 protected
game, runtime and handoff artifacts are unchanged. After complete acceptance,
the planned follow-up is an updated Match Debugger containing the conversion
and clearer controls and match information. That UI/UX work is not implemented
or delivered by this checkpoint.

At the preceding contact checkpoint, cycle cleanup removed 43 obsolete standalone
checks and the three current conformance executables after validation, plus associated
debug symbols: 103 files and 6,446,007,277 logical bytes. Available space
increased by 6,447,804,416 bytes during cleanup; concurrent filesystem activity
can affect that observation. All 845 protected artifacts, both installed signed
ARM64 apps, original inputs, source references and the reusable production
library were verified unchanged. Private cleanup plans and receipts are retained.

The execution/receiver checkpoint used only the focused CMake and sanitizer
executables, then removed both and their debug symbols: five files and
302,183,780 logical bytes. Available space increased by 302,161,920 bytes during
that cleanup. The same 845 protected artifacts, app identities and production
library hash remained intact.

The receiver-outcome checkpoint likewise removed both check executables and
their debug symbols after validation: five files and 304,070,860 logical bytes.
Available space increased by 304,078,848 bytes during cleanup. All 845 protected
artifacts, both installed signed ARM64 apps and the production library hash
were verified unchanged; sources, original references and evidence are retained.

The execution-result checkpoint removed its two conformance executables and
debug symbols after matching Release/UBSAN acceptance: five files and
307,009,956 logical bytes. Available space increased by 307,019,776 bytes during
cleanup. All 845 protected files, both signed ARM64 apps and the production
library hash remained unchanged.
