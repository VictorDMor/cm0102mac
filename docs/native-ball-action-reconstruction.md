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
| **Validated contiguous prefix** | **`0x1023cd20..0x1023fa5c`** | **2,895** |

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

The conformance harness checks each component individually and every successive
prefix from the original root. Dependencies are opaque deterministic fixtures
that record full register state before and after calls, clobber volatile
registers, produce observable writes, and support interruptions and mutations.
This checks caller contracts; it does not establish new dependency or production
runtime acceptance.

## Component evidence

The current check contains 127,001 comparisons:

- 18,199 successful component or prefix completions;
- 108,782 instruction-budget, memory-hole or dependency interruptions;
- 20 rejected own-result/dependency negative controls;
- all 2,895 original instruction sites, including dormant branches;
- complete architectural state, mapped memory, ordered accesses, dependency ABI,
  original-site order and instruction accounting comparisons.

The same checks pass in the normal C++23 build and a focused UndefinedBehaviorSanitizer
build with floating-point contraction disabled. Eight Python audit tests check
whole-body/fragment integrity, truncated and reordered input rejection, all 208
table words and five pointers, isolated component labels/checkpoints, external
interior edges, and the unchanged production manifest. Record-form narrowing
also checks all 256 initial condition bytes, both overflow bits and three signed
halfword classes at the first interruption boundary.

19,835 of the comparisons use paired checkpoint injections to reach dormant
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

Continue at `0x1023fa5c` with action execution. The remaining
5,247 original instructions, all five computed decision tables, tail/error paths
and common return still require reconstruction and equivalence evidence.

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

At this checkpoint, cycle cleanup removed 43 obsolete standalone checks and
the three current conformance executables after validation, plus associated
debug symbols: 103 files and 6,446,007,277 logical bytes. Available space
increased by 6,447,804,416 bytes during cleanup; concurrent filesystem activity
can affect that observation. All 845 protected artifacts, both installed signed
ARM64 apps, original inputs, source references and the reusable production
library were verified unchanged. Private cleanup plans and receipts are retained.
