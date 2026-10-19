# Match team selection and preset-target reconstruction

Four complete team-query operations now execute semantic C++23 by default in the
installed ARM64 game. The manifest contains **923 unique owners**, preserving the
exact **919-owner prefix**. The block contains **815 original instructions**.
The complete match-engine and whole-game semantic reconstruction remain unfinished.

| Entry | End, exclusive | Original instructions | Operation |
| --- | --- | ---: | --- |
| `0x101e1bd0` | `0x101e20f4` | 329 | Attribute-weighted actor selection |
| `0x101e2100` | `0x101e22d8` | 118 | Role-weighted actor selection |
| `0x101e22e0` | `0x101e2598` | 174 | Profile-weighted actor selection |
| `0x101e25a0` | `0x101e28a8` | 194 | Preset tactical target cell |

Original addresses, assembly, operations, flows and fallthrough are pinned across
five page fragments. The full index has no external interior entrances, including
fallthrough. Source fingerprints:

- `0x101e1bd0`: `d6754012226c703c7ac84c15470418bd8f36bc73aa9f429eab33c8ebc00817ec`.
- `0x101e2100`: `3bdf67dbb5a7f3c3b64ebfd44fefbd2bc0b6cbc4690e467ce1a4dfcd6118f71e`.
- `0x101e22e0`: `7b5ee98fc508cebf4764f16a761e677bc102070780a3cad9a9ad6124f760b316`.
- `0x101e25a0`: `48cd5e6c11be438b3746e0e463fb4b4cd7deb73b46276124c4bf6eeaa096c50e`.

## Selection and behavior

A probe linked to the preceding 919-owner engine runs 600 actual Arsenal/Manchester
United advances with seed 12345 and 151,918,413 executed instructions. Entry-read
counts in table order are **0, 0, 0, 519**. This establishes natural invocation of
the preset target lookup in that window. The selector family shares live role,
position and tactical data queries; its three policies are reconstructed together.
Entry counts do not establish CPU impact or a speedup. The nearby actor-address
helper `0x101e3180` and other team/football policies retain separate scope.

Each selector scans twenty actor slots, rejects a negative signed position or
odd role, and retains the first actor in a score tie. An empty eligible set returns
minus one. Original floating constants, role-bit precedence and qualifier tests,
retained previous floating weight for unmatched roles, live optional profile reads,
short/byte signedness, single/double rounding and FPSCR residue remain observable.
The attribute policy retains its signed low-byte mode, live actor attributes,
random score alternatives and final random rejection. Profile modifiers preserve
thresholds and repeated loads. Draw order, arguments and consumed cursor state
remain unchanged. Nonfinite values follow the original comparison flags.

Preset targets retain the owner marker and five separate table offsets: `0x9f`,
`0x30`, `0xa4`, `0x3b` and `0x35`. The current position, actor side and table pointer
are read again for the high nibble after the low-nibble output store. Row/column
outputs may alias each other, live actor/owner/table fields or caller stack.
No new bounds, null handling or gameplay correction is introduced.

Named role-weight, profile-modifier, score and winner stages use structured
forward control flow inside the twenty-slot scans. Complete registers/temporaries,
LR/TOC/PC, saved floating registers, frames, effects, dependency ABI, source order,
faults and instruction charges retain the original contract. All three direct
children already have semantic owners: position/role queries and the match sampler.
The shared helper, native platform, sampler and preceding target bodies remain
unchanged. Production does not replay original bodies or use a failure fallback;
private original and profiling controls are separate.

## Independent component evidence

The four-operation component passes **2,369 comparisons**: **772 complete returns**,
**1,590 matching interrupted outcomes** and **seven discriminating own/RNG-child
negative controls**. All **815 source sites** occur in complete nonnegative
executions, without injected source mutations. Complete starting-root returns are
438, 66, 210 and 58.

Cases cover role-bit precedence/qualifiers, all twenty slots, signed side/position
and profile fields, threshold endpoints, optional profiles, both attribute modes,
random sequences and extremes, equal scores, signed zero, infinities, NaNs,
relocated TOC and output/source/stack aliases. Independent first-winner,
no-eligible-actor and packed-output expectations pass. Representative productive
paths include every instruction cut, every accessed-byte hole and interruption
after each dependency call. Position, role and random services are explicit
component models, with volatile-register poisoning and a modeled draw cursor.
The component is distinct from real-service composition.

Output aliases can overwrite saved caller state. The independent reference stops
at the observed own return instruction and restored caller stack, retaining the
resulting PC. Following a damaged caller is outside this component boundary.
No production behavior is changed to make such a fixture return safely.

## Actual-service evidence

The production harness runs 600 natural native match advances. Dedicated counters
observe zero calls for the three selectors and **519 complete preset-target calls**,
matching the preceding selection probe. The three missing roots use explicit
indexed original BLs `0x101dc8f8`, `0x101dbe90` and `0x101dbb50`, supplied with the
actual match, home side and attribute mode zero. Each runs its full selected query
with actual services and returns to that BL's own continuation. Each BL also passes
a paired original/candidate comparison. This is prepared call-site coverage;
the full event parent and natural invocation of those selectors are not claimed.

The matrix passes **59 paired comparisons**: **43 completions including four
negative controls**, hence **39 valid completions**, **twelve matching budget stops**
and **four required-pointer faults**. Fifteen complete productive modern comparisons
include three original BLs plus direct/generated/CFM paths for all four roots.
Global and per-entry original controls, three budget cuts per root and one
required-pointer fault per root pass. A separate profiling run passes **twelve
original controls** across the three dispatch paths.

Complete productive runs observe **603 own source sites**: **274, 107, 159 and 63**
in table order. They observe **1,018 completed selected-helper returns**, excluding
natural preparation and original controls: **244 sampler**, **642 position** and
**132 role-query** returns, including nested invocations. Individual root counts,
raw PC sets, captured snapshots, commands and logs are retained privately.
Full state, ordered effects, exact accounting and import ABI match. No component
service models are used; only host clock import values are paired explicitly.
Runtime and profile stderr are empty. This matrix does not cover every selector
mode, tactical variant, match event or complete original event parent.

## Integration, sanitizer and installation

Ten source/hook guards and **87 focused historical manifest methods** pass without
skips. Historical changes only update totals and bound the preceding target block's
ownership slice. Full generation retains **1,953,334 indexed instructions**, **1,922
pages**, 1,923 generated C++ files and zero missing/pending targets. Only
`page_101e1.cpp` and `page_101e2.cpp` change. The three independent selection,
target and sampler reference outputs remain byte-identical after the build.

Three focused CTests pass **8,567 comparisons**: the new 2,369-case block, unchanged
4,142-case target component and unchanged 2,056-case sampler component, covering
**1,710 original sites** together. Composed component/runtime UBSAN passes with
**13 instrumented translation units** and **17 pinned headers**: four semantic
bodies, adapter, runtime diagnostics, both harnesses, independent reference, two
changed root pages and two unchanged original-caller pages. Both executables match
Release stdout with empty stderr. Actual productive PC sets are byte-identical.
Unchanged archive helpers and whole-game execution are outside this sanitizer claim.

The installed game, candidate and mapped live executable have ARM64 SHA-256
`32d24b72e12394df9c5aaead383b4f1f3e581faa27bb50bac29fa03975cabf5d`. Strict deep signatures pass. Four original resource files and
bundle metadata are unchanged. Atomic installation preserves the previous
919-owner application with executable SHA-256 `36b1ab0167a0a6cd4ecaa6f35883f7197a44eaa41fc6ef13c7a66e4375e9ee18`.
The installed Match Debugger retains executable SHA-256
`2054db1cc3f620b5cf4923b857a8beb65e16cd42498dbe474eb493185456eb89`
and its strict signature.

CUA screenshots verify Prepare Game, the original Portuguese options, cancellation
back to Prepare Game and the original quit confirmation. The observed process exits
normally; independent inspection confirms absence. This UI smoke does not attribute
match-entry invocation. All **846 protected files** and **eighteen unrelated private
prompts** retain their hashes. Original sources, private payloads, snapshots,
evidence and the previous application are preserved.

Save/Load is **not retested**; performance is **not benchmarked**. PDF updates remain
paused at the user's request, with edition 39 the last accepted guide. The complete
reconstruction objective remains active.
