# Injury fitness and supporting randomness

## Status

Seven complete original operations have named C++23 component implementations
and passing direct comparisons. They are **prepared, not integrated into the
production game**. The accepted semantic manifest and installed application still
contain the previous 734 entries. This work does not establish gameplay or
installed-application acceptance for the new operations.

The validation cadence now accumulates related components before expensive batch
integration. Each changed component retains direct comparison with the original;
production generation, runtime integration, sanitizers, application installation
and visual verification occur at the larger batch milestone. This avoids repeating
those steps for every small group of functions.

## Complete operations

| Original entry | Exclusive end | Instructions | C++ operation |
| --- | --- | ---: | --- |
| `0x10187450` | `0x10187644` | 125 | Update staff condition |
| `0x10187650` | `0x10187680` | 12 | Read signed staff condition |
| `0x10187680` | `0x101877dc` | 87 | Classify injury recovery phase |
| `0x101877e0` | `0x101878d0` | 60 | Read raw or percentage fitness |
| `0x101878d0` | `0x10187a1c` | 83 | Read effective fitness with recovery limits |
| `0x10187a40` | `0x10188488` | 658 | Refresh cached physical state and training recovery |
| `0x101864a0` | `0x10186a18` | 350 | Draw a signed bounded random value |

The source index contains all 1,375 instructions and no incoming external edges
into the interiors of these seven bodies. The adjacent accepted flag setter at
`0x10187a20` keeps its existing owner.

## Preserved behavior

The implementation retains original signed guards, byte and halfword narrowing,
unchecked negative staff identifiers, the injury-index comparison with `count`
rather than `count - 1`, lazy shared date initialization before early returns,
cached club and schedule values, original dependency order, and repeated pointer
loads. It preserves stack residues, volatile registers, arithmetic flags, floating
status, and instruction accounting at the observable guest boundary.

Condition changes use the original random pool cursor, wrap and reseed behavior.
Fitness calculations retain the original integer reciprocal divisions, single
precision rounding points, double precision calculations and unfused negative
multiply/subtract operation. Compilation uses `-ffp-contract=off`.

The supporting random operation preserves signed bounds, wrapped 32-bit
multiplication, the four originally inlined draw levels and the final recursive
call. The further wide branches are retained even when the preceding arithmetic
constrains ordinary inputs to the narrow path.

Original instructions, flow metadata, table pointers and constant bytes are
fingerprinted. The standalone original references are generated with semantic
hooks disabled. Production component bodies do not replay the indexed
instructions or call the reference implementation.

## Direct evidence

The isolated Release component check, linked against the accepted 734-entry
dependency libraries, passed **590 comparisons**:

- 253 completed comparisons, 324 matching interrupted or faulted comparisons,
  and 13 rejected negative controls.
- Full register, temporary, mapped-memory, allocator/import, ordered-effect,
  dependency ABI, original-site order and instruction-count comparisons.
- Representative budget cuts, sampled missing bytes, and interruption after each
  dependency in the selected boundary fixtures.
- Coverage of all 1,375 original sites. **47 comparisons use explicit structural
  register or condition controls**, including nested wide random branches. These
  controls are component evidence, not natural gameplay observations.
- One controlled fitness-to-random comparison executes both new C++ components
  and both complete original references through the actual dependency boundary.
  Other dependencies use deterministic fixtures.

Four source-guard tests also pass, including altered instructions, omitted body
instructions, external interior entries, changed constants and changed pointers.
The private originals are required for these checks; source-guard tests skip when
those inputs are absent.

```sh
python3 -m unittest discover -s tools -p test_injury_fitness_batch.py -v
```

Private component build recipes, original references and detailed receipts remain
under `.local/injury-fitness` and `.local/injury-random`. No private original code,
game assets, snapshots or saves are published.

## Remaining batch work

Complete the related daily injury processing and physical-state maintenance
frontier before integrating the prepared components together. At that milestone,
add production adapters and generated hooks, retain the exact accepted manifest
prefix, build the changed batch consistently, exercise actual runtime boundaries,
run focused sanitizers, and install and inspect the resulting application once.

Production integration, sanitizer acceptance and gameplay acceptance are pending
for these seven operations. Save/Load: **not retested**. Performance: **not
benchmarked**. The complete-game reconstruction goal remains active.
