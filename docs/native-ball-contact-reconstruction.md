# Ball-contact dependency reconstruction

Status: **complete block integrated and installed in both signed ARM64 apps**.

The accepted production manifest has **682 unique entries**, retaining all 678
previous entries and adding four contact roots. Application (`0x101f5110`) and
resolution (`0x101f5180`) already belonged to `match_discipline_adapter`; they
keep those owners and their tested implementations. The contact facade API reuses
them instead of maintaining duplicate bodies or adding duplicate hooks.

The complete six-entry block covers 2,043 original instructions; 1,991 belong
to the four newly integrated entries. Components, real callers, original/profile
controls, nonlocal transfers and a paired full match pass. Generation retains
1,953,334 original instructions across 1,922 units, with no missing or pending
targets. Only the two units containing new entry hooks changed.

## Complete original boundaries

| Operation | Original range (exclusive end) | Instructions | State |
| --- | --- | ---: | --- |
| Contact screening facade | `0x101f5050..0x101f50b8` | 26 | Integrated |
| Contact attempt facade | `0x101f50c0..0x101f5110` | 20 | Integrated |
| Contact application facade | `0x101f5110..0x101f5180` | 28 | Reused discipline owner |
| Contact resolution facade | `0x101f5180..0x101f51e0` | 24 | Reused discipline owner |
| Action-contact description | `0x101f5350..0x101f6940` | 1,404 | Integrated |
| Failed-contact reaction | `0x10270320..0x10270b94` | 541 | Integrated |
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
not claimed as naturally reached gameplay. Six Python audit tests cover the
complete scope, altered/missing/reordered source rejection, reference labels,
three live-table contracts, complete entry hooks and the unchanged 678-entry
prefix. All 49 affected audit tests also pass.

Build `cm-ball-contact-facade-checks`, `cm-ball-contact-failure-checks` and
`cm-ball-contact-description-checks` with the private `CM_ORACLE_DIR`. Run
`ball_contact_facade_equivalence`, `ball_contact_failure_equivalence` and
`ball_contact_description_equivalence`. Public audit entry point:
`python3 -m unittest discover -s tools -p test_ball_contact_audit.py -v`.

## Shared-engine and nonlocal acceptance

The actual Cruzeiro/Manchester United match (seed 12345) completes 789 advances
with all six roots selected as original versus C++23. Every advance preserves
instruction accounting and return status; intermediate and final states and the
import ABI agree. Natural calls/returns are:

| Entry | Calls | Returns |
| --- | ---: | ---: |
| Screening | 100 | 100 |
| Attempt | 474 | 474 |
| Application, reused owner | 1 | 1 |
| Resolution, reused owner | 1 | 1 |
| Description | 13,063 | 13,063 |
| Failure reaction | 1 | 1 |

All six actual entry captures pass 72 direct/generated/CFM comparisons, including
12 budget stops and six discriminating negatives. Eighteen profile controls
select the original bodies. Forty-nine nonlocal comparisons cover each root and
the description after preparation, including 28 restore/handler faults; abandoned
frames do not resume. Dedicated counters record each owner without double counting
shared entries. The complete production comparison and match also pass with
UBSAN instrumentation on the new adapter, contact bodies and reused facades.

## Measured performance

An initial timing run was noisy. Grouping five identical 600-tick intervals per
sample confirmed that the first integrated candidate was slower: median paired
runtime changes were +1.88% and +0.86%. Inspection showed that the compiler kept
small preparation operations as separate calls. `prepare_description` now uses
`gnu::flatten` to expose fixed register selections across both players. Arithmetic,
observation order, budget checkpoints and nonlocal behavior are unchanged; the
177,348-case Release/UBSAN matrix, actual match and nonlocal checks pass again.

The final comparison uses the retained pre-integration 678-entry library as the
baseline, with no original-selection wrapper overhead. Each scenario has ten
measured pairs after warmup; each sample averages five runs, alternating execution
order. Complete 126,967,858-byte snapshots, instructions and host inputs agree.

| Scenario against Manchester United | Median paired runtime reduction | Faster samples |
| --- | ---: | ---: |
| Arsenal | 0.58% | 9/10 |
| Cruzeiro | 0.21% | 6/10 |

These are small interval measurements, close to timing variation. This delivery
establishes accurate reconstruction and removes duplicated ownership; it does
**not establish a large speedup** or a whole-game performance improvement.

## Installed delivery

Both candidate and installed executables are ARM64 with verified signatures and
matching hashes. Original program resources agree with the private source.
The previous applications remain private backups; all 845 protected files were
verified intact before and after atomic installation.

| Installed executable | SHA-256 |
| --- | --- |
| CM 01-02 Native Prototype | `fd070a5bbd78e04de657bbbd8f5441dada508cac631b18c8bbf07f15acf6c43b` |
| CM Match Debugger | `2054db1cc3f620b5cf4923b857a8beb65e16cd42498dbe474eb493185456eb89` |

The installed Debugger completes the same match in 789 steps, 1–2 at 92:38,
with a readable Full time event, completed-step rejection, undo/redo and normal
backing cleanup. The main game shares the validated library; its management UI
was not separately exercised in this cycle. Save/Load is **not retested**.
The full native-port objective remains active beyond this block.
