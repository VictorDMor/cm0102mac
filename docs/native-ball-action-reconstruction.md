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
| Execution-result outcome, restart selection and messages | `0x10241b24..0x102424ac` | 610 |
| Unopposed action marker, receiver counter and status effects | `0x102424ac..0x102425f4` | 82 |
| Final receiver/score gate and independently converted aggregates | `0x102425f4..0x102426c8` | 53 |
| Special state-four profiles, counters, receiver reaction and balance | `0x102426c8..0x10242cc0` | 382 |
| Regular-action messages and balance | `0x10242cc0..0x10242f3c` | 159 |
| Completed-action trigger, area gate and player reaction | `0x10242f3c..0x102433d4` | 294 |
| Failed-action receiver/area gates and signed reaction sample | `0x102433d4..0x1024366c` | 166 |
| Third live action-message table | `0x1024366c..0x1024389c` | 140 |
| Live ten-word failed-comment table and default | `0x1024389c..0x102439d0` | 77 |
| Live failed-fallback table, callback and floating comparison | `0x102439d0..0x10243ba4` | 117 |
| **Validated contiguous prefix** | **`0x1023cd20..0x10243ba4`** | **7,073** |

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

Execution-result outcome publishes the inverted initiating side, reports the
initiating player's contact, then overwrites the selected message register with
the restart-player pointer. Followup eligibility and area queries use that
selected player. Blocked endings retain the receiver's identity in their
message and mark contact six as well as four/five. Final movement/reset and
the wrapped twenty-five decrement apply to the initiating player. These
distinctions from receiver outcome remain explicit.

The unopposed path preserves the conditional stack marker, signed action-byte
storage, wrapped receiver count, and its special state-four bypass. Its two
status queries retain their ordered ABI and separate live results; the second
query is conditional and chooses between the original 59/70 decrements. The
receiver is still accessed before the later null gate, preserving the original
fault behavior. Both new phases have actual original-root fixture prefixes.

Final preparation preserves the nullable receiver/score gate, initiating-player
callback, two independent floating conversions and aggregate updates, and the
receiver's wrapped byte increment. Special state four retains repeated profile
and classification queries, message selection, separate counter/rating writes,
receiver reaction and the signed balance clamp. Regular actions retain their
flag/owner/state/coordinate gates and distinct report paths. The three balance
copies preserve their original control flow: the final lower clamp falls through
where the earlier copies branch, which matters for accounting and budget stops.

Completed-action reaction keeps its signed balance sample and random threshold,
conditional match flags/aggregates, trigger call, inverted-area gate and signed
initiating-player reaction sample. Word sampling still uses the shared audited
schedule. Dormant wide intervals and the structurally zero special interval are
covered by paired injections; they are not claimed as natural game paths.
Four additional original-root prefixes reach these phases and compare all
state, effects, source sites and instruction accounting.

Failed-action preparation retains its null-receiver diagnostic continuation,
dependency/blocked/signed-sample gates, receiver-area query and even random
selection. Its three live tables preserve their separate 47/10/47-word extents,
receiver-first and inverted-side message ABI, direct message entrances, altered
targets and explicit continuations. The fallback callback receives the signed
rating-minus-score difference and may return early; its alternative floating
comparison preserves both single-precision additions/subtraction and their
original order around the random sixth. All five coordinator tables now have
component evidence. Whole-root continuation/production acceptance is still open.

The conformance harness checks each component individually and every successive
prefix from the original root. Dependencies are opaque deterministic fixtures
that record full register state before and after calls, clobber volatile
registers, produce observable writes, and support interruptions and mutations.
This checks caller contracts; it does not establish new dependency or production
runtime acceptance.

## Component evidence

The current check contains 382,287 comparisons:

- 61,668 successful component or prefix completions;
- 320,565 instruction-budget, memory-hole or dependency interruptions;
- 54 rejected own-result/dependency negative controls;
- all 7,073 original instruction sites, including dormant branches;
- complete architectural state, mapped memory, ordered accesses, dependency ABI,
  original-site order and instruction accounting comparisons.

The same checks pass in the normal C++23 build and a focused UndefinedBehaviorSanitizer
build with floating-point contraction disabled. Eight Python audit tests check
whole-body/fragment integrity, truncated and reordered input rejection, all 208
table words and five pointers, isolated component labels/checkpoints, external
interior edges, and the unchanged production manifest. Record-form narrowing
also checks all 256 initial condition bytes, both overflow bits and three signed
halfword classes at the first interruption boundary.

37,354 of the comparisons configure paired checkpoint injections for dormant
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

Continue at `0x10243ba4` with receiver selection, effects and remaining reports.
The remaining 1,069 original instructions, diagnostic tail and common return
still require reconstruction and equivalence evidence. No computed table remains
unimplemented at the component level.

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

The result-outcome checkpoint removed its two conformance executables and debug
symbols after matching Release/UBSAN acceptance: five files and 309,440,204
logical bytes. Available space increased by 310,484,992 bytes during cleanup;
concurrent filesystem activity can affect that observation. All 845 protected
files, both signed ARM64 apps and the production library hash remained unchanged.

The success-effects checkpoint removed the two conformance executables, its
private diagnostic executable/object and debug symbols after matching
Release/UBSAN acceptance: seven files and 457,440,944 logical bytes. Available
space increased by 458,780,672 bytes during cleanup. All 845 protected files,
both signed ARM64 apps and the production library hash remained unchanged;
diagnostic sources and evidence are retained.

The failed-messages checkpoint removed both conformance executables and debug
symbols after matching Release/UBSAN acceptance: five files and 315,254,804
logical bytes. Available space increased by 315,035,648 bytes during cleanup.
All 845 protected files, both signed ARM64 apps and the production library hash
remained unchanged; source references and evidence are retained.
