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
| **Validated contiguous prefix** | **`0x1023cd20..0x1023ea44`** | **1,865** |

The named components preserve live reloads, signed-byte/halfword narrowing,
wrapped word arithmetic, reciprocal division, floating-point status, scratch
spills, random cursor advancement, dependency ABI and original instruction
accounting. Identical word-interval sampling schedules and area predicates share
semantic helpers; the distinct halfword adjustment schedule remains separate.
`SemanticGuestCall::sign16` now supports the original record form while keeping
the existing non-record behavior as its default.

The conformance harness checks each component individually and every successive
prefix from the original root. Dependencies are opaque deterministic fixtures
that record full register state before and after calls, clobber volatile
registers, produce observable writes, and support interruptions and mutations.
This checks caller contracts; it does not establish new dependency or production
runtime acceptance.

## Component evidence

The current check contains 59,219 comparisons:

- 12,142 successful component or prefix completions;
- 47,063 instruction-budget, memory-hole or dependency interruptions;
- 14 rejected own-result/dependency negative controls;
- all 1,865 original instruction sites, including dormant branches;
- complete architectural state, mapped memory, ordered accesses, dependency ABI,
  original-site order and instruction accounting comparisons.

The same checks pass in the normal C++23 build and a focused UndefinedBehaviorSanitizer
build with floating-point contraction disabled. Eight Python audit tests check
whole-body/fragment integrity, truncated and reordered input rejection, all 208
table words and five pointers, isolated component labels/checkpoints, external
interior edges, and the unchanged production manifest. Record-form narrowing
also checks all 256 initial condition bytes, both overflow bits and three signed
halfword classes at the first interruption boundary.

15,581 of the comparisons use paired checkpoint injections to reach dormant
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

Continue at `0x1023ea44` with contact selection and action execution. The remaining
6,277 original instructions, all five computed decision tables, tail/error paths
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
