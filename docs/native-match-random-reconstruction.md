# Original match range sampler reconstructed

The later [context/mode and score block](native-match-selection-support-reconstruction.md)
installs 926 owners and adds two complete selection policies that consume this
unchanged sampler. Its 2,056-case component passes alongside both selection blocks.

This report retains the original sampler milestone. The later
[team-selection block](native-match-selection-reconstruction.md) installs 923 owners
and adds three complete policies that consume this unchanged sampler. Its
2,056-case component passes alongside the selection and target components.

The subsequent [shared sampler reconstruction](native-range-sampling-reconstruction.md)
adds the complete `0x10226520` and `0x107124e0` bodies and installs 911 owners.
This report retains the historical 656-owner milestone and its measurements.

The complete `101d3c90..101d3e48` range sampler now executes structured C++23
in the game and standalone match debugger. Its 110 indexed instructions have
415 indexed direct call sites, including its recursive call. The ordered
production manifest grows from 655 to **656** entries without changing its
previous prefix. This is one complete, heavily reused operation; the other
match sampler (`10226520`), general sampler (`107124e0`) and the full engine
remain unfinished.

## Why this block came first

A fresh Arsenal/Manchester United match, seed 12345, executed 200 actual engine
ticks. A private read observer counted 33,962 calls to this sampler, 241 to
`10226520` and zero to `107124e0` during those ticks. These are exact observed
entry-read counts for the three candidates, not CPU percentages or proof of
coverage elsewhere. The 110-instruction body offered a small migration with
many productive callers. Original listings and the profiling input remain
private.

## Preserved contract

The signed range arrives in r3. Zero returns zero without advancing the random
pool, while retaining the original frame, saves and live TOC-slot loads.
Nonzero ranges advance the big-endian 16-bit pool cursor by two. An unsigned
comparison against the live inclusive limit decides whether to wrap to the
live base and refresh the salt through the existing Mac CRT generator.
Cursor stores, late pointer reads and salt refresh preserve alias behavior.

Bounds from -65,535 through 65,535 use the original signed remainder, with the
negative-bound sign adjustment. Larger bounds use the live double constants,
original stack conversion scratch, signed integer conversion and a recursive
sample for rounding correction. Multiplication before recursion wraps at
32 bits. The original upper clamp and negative-range behavior are retained;
this is not a replacement with a host random distribution.

Named pool-advance, small-range and wide-range operations retain the 112-byte
frame, nonvolatile saves, volatile and temporary residues, LR/TOC, conditions,
FPSCR, PC, instruction charges, ordered guest accesses and fault prefixes.
Normal execution does not replay the sampler's original body. Diagnostic
original selection, per-entry selection, profiling, call/outcome counters and
nonlocal scope routing remain separate. Private runtime diagnostics grow
without changing the State layout used by generated translation units.

The source fingerprint is
`17e9330216425b9b39ef2830f09567441660ba1c2fc6282a6009aad902b2eaa9`.
The complete manifest fingerprint is
`6619436c27e93ba3be0a0931c24b9c3c664948c22f6f132938570aa1b04c58ab`.
Generation audits every indexed instruction and retains 1,953,334 instructions,
1,922 pages and zero missing/pending targets. Only `page_101d3.cpp` changes among
1,923 generated C++ files; its accepted predecessor and manifest are retained.

## Validation

- **2,056 independent-original component comparisons:** 701 normal returns,
  1,353 interrupted outcomes and two discriminating own/recursive negatives.
  All 110 original source sites are reached. Comparisons include full registers,
  temporary storage, mapped memory, allocator/import state, ordered effects,
  dependency ABI, source order and accounting. The matrix covers signed bounds,
  overflow, salt/sample extremes, wrap, recursion, relocated TOC, slot/stack
  aliases, live nonfinite constants, dependency faults, every instruction budget
  and every accessed byte hole along representative paths. Independent ordinary
  result and cursor expectations also pass.
- **15 actual-production comparisons:** natural caller capture, direct/generated/
  CFM entry paths, global and individual original controls, a negative, two
  budget stops and three explicit wide-bound recursive cases. One natural match
  tick records 177 dedicated C++23 calls and 177 returns without original
  selection, faults or dispatch escapes. Three original-profile controls pass
  separately.
- **14 nonlocal comparisons:** both outer and recursive sampler frames, original
  and semantic context dependencies, turbo controls and eight restore/handler
  faults. Abandoned frames do not finish their epilogues or callbacks.
- **Five focused CTests pass**, including actual match completion: 100 Arsenal
  ticks and a Cruzeiro match reaching full time after 789 ticks, with original
  completion agreement and debugger undo/redo. Seven source/edge/hook audits and
  40 changed manifest assertions pass. UBSAN instruments the new body, adapter,
  private diagnostics, original reference and three sampler harnesses; all three
  runs pass with empty sanitizer stderr. Unchanged archives are not newly
  instrumented.

## Paired performance measurement

The benchmark restores identical real match states and runs 600 ticks per
interval. Two club pairings each receive one warmup pair and nine measured
pairs, alternating execution order. The baseline bypasses the diagnostic
original wrapper and runs the generated body directly; an extra diagnostic
`run_until` is therefore not credited as a semantic speedup. There is still a
common destination-hook check. Timing excludes creation, restoration and
comparison. Turbo and instruction profiling are disabled; explicit identical
clock inputs are supplied when requested.

| Match | C++23 calls per interval | Median paired time reduction | Faster pairs |
|---|---:|---:|---:|
| Arsenal / Manchester United | 101,062 | 0.15% | 5 / 9 |
| Cruzeiro / Manchester United | 94,167 | -0.90% | 4 / 9 |

All forty executions, including warmups, produce identical paired complete
states, instruction counts and clock-input counts. The mixed results show
**no consistent runtime speedup** in these short intervals. This delivery's gain
is semantic ownership of a widely used operation. It is not an 80/20 CPU claim,
a whole-game benchmark or evidence of a user-visible speed increase. Raw paired
measurements and receipts remain in `.local/random-sampling-reconstruction`.

## Installed acceptance

Both ad-hoc signed ARM64 applications are installed with protected prior
references, metadata-preserving APFS copies and atomic exchanges. Candidate and
installed executable hashes agree:

- Game: `a6b42be75302bf96a1e4eb4cd8698bb1945abac025f1191167b75cdbcbd40953`.
- Debugger: `c64d2d70b632e6e234d6e49915562afe698d0f9a998ff5c1db15d0b338f2b1c7`.

All four packaged original-program files in each app and 845 protected
original/game/runtime/save/handoff files are verified. The installed debugger's
two-worker checks pass real club/roster creation, progression, reset, goals,
withdrawal, timeline operations and substitutions, with normal quit/EOF,
empty stderr and removal of their backing. The installed game renders its
original menu and exits through the original confirmation; its mapped running
executable is verified against the signed candidate. Menu startup is not
claimed as sampler runtime coverage.

Save/Load is **not retested**. The sampling algorithm, random state layout and
persistence format are unchanged. The complete C++23 port goal remains active.

After acceptance, 19 validated owned disposable outputs are removed, recovering
1.45 GB of measured available space. Current objects/libraries, both installed
and candidate apps, prior signed references, original inputs and private
validation evidence remain. Protected files and application identities are
rechecked after cleanup.
