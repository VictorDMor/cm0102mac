# Semantic Mac background selection core

## Scope and contracts

This delivery reconstructs four related background selectors in semantic C++23.
It preserves the original choices, fallbacks and Mac RNG consumption. It does
not change bitmap resolution, native presentation, menu commands or gameplay,
and makes no visual-quality or performance claim.

The starting checkout and a fresh `origin/main` query both identify
`7b812d530ad8331102af1277a8e20aab8775ac05`. The eleven untracked feature handoffs
are retained and excluded from the product commit. The original read-only Mac
instruction export supplies the reference bodies; no new oracle or Ghidra
export is produced.

| Entry | Exclusive end | Instructions | Contract |
| --- | --- | ---: | --- |
| `0x107122e0` | `0x107124e0` | 128 | Select a catalog node, then its primary or alternate image list |
| `0x10712a60` | `0x10712c4c` | 123 | Select a list node and return its image payload |
| `0x10713960` | `0x107139b4` | 21 | Select the default background for a category |
| `0x107139c0` | `0x10713a74` | 45 | Select by category/object descriptor, or use the category default |

The catalog selector accepts a collection in r3 and a selector in r4. Its count
at collection `+12` is truncated and sign-extended from 16 bits. A null collection
or effective zero count returns zero without consuming a pool sample, although
its original prologue still reads the TOC slots and saves registers. Generated
accessor `0x1074df60` receives the chosen index. A missing node or null node
payload at `+4` returns zero after that sample has already been consumed. A
nonzero low byte of the selector chooses payload `+12`; otherwise it uses the
primary list. It delegates the second choice to `0x10712a60`.

The list selector uses the signed16 reduction of its count at `+8`. Null or
zero-effective-count lists do not consume a sample. Generated accessor
`0x1074dd40` receives the chosen index, and the returned node's payload at `+0`
is the result. An accessor miss returns zero without undoing RNG consumption.

Both selectors use the existing unsigned16 big-endian pool. Relative to live r2,
slots `-0x7b90`, `-0x7b8c`, `-0x7b88` and `-0x7b84` hold the salt pointer, base,
inclusive-end pointer and cursor pointer. The cursor is advanced by two before
sampling. The inclusive final address is valid; only an unsigned address above
the limit wraps to the base and renews salt through semantic Mac rand
`0x1077b770`. The cursor is then reloaded, the effective count compared, the salt
read and finally the unsigned16 sample read, preserving observable alias order.
Sample plus salt wraps as a 32-bit word and is interpreted as
signed32 for the remainder by the effective signed16 count. Positive counts use
the remainder; negative counts use its negation. This preserves malformed-count
behavior and is not an absolute-value normalization.

The original floating branch tests for a count outside `[-65535, 65535]` after
signed16 extension, so it is unreachable. The semantic selectors omit that
branch without replacing or expanding the separate generated sampler
`0x107124e0`.

The default category selector uses the byte enable flag at `r2-0x35d4` and table
pointer at `r2-0x5c3c`. Its offset is `(category << 2) & 0x3fc` with original
32-bit rotation/masking semantics. The object selector instead sign-extends the
category's low 16 bits and uses its wrapped four-byte offset, retaining negative
indices. These calculations remain distinct. Disabled backgrounds or an absent
category return zero. A null object ignores the descriptor and delegates to the
catalog selector; a present object searches through generated `0x1074df90`, then
selects the primary or alternate list from the matched node's `+4` payload.
Descriptor misses do not consume a random sample.

The adapters preserve architectural residues and ordered effects as well as the
result: condition flags, r0 and volatile-register contents, context TOC, PC/LR/CTR,
original backchains, stack spills and nonvolatile reloads. Catalog/list/default/
object frame sizes remain 112/96/64/64 bytes. Pointer loads and register saves
remain interleaved in their original order, and pointer values are reloaded at
the original dependency boundaries. Slot, cursor, salt and stack aliases are not
cached away. Unmapped accesses retain their original fault PC and partial effects.
Generated dependencies execute through `run_until` with their original LR and
context TOC, without callback-style restoration of their register clobbers.

## Production integration and remaining generated code

`background_selection_adapter.*` contains the named catalog, list, category and
object selections. Destination hooks in generated pages `0x10712` and `0x10713`
intercept all four entries before their original first operation, including
same-page calls/gotos. The normal dispatch selects semantic implementations.
A read-only indexed flow/fallthrough audit found no external static edge into
any of the four interiors. Entry incoming-flow counts are 3/4/1/6 respectively;
these are static evidence and do not classify unknown dynamic entry points.
A private background-only comparison mode can run the original bodies, retaining
semantic Date and RNG. Optional boundary observers and per-entry counts live in
process-local diagnostics at the end of `State`; they do not claim app coverage.
The existing probe prints a compact count summary on normal application exit.

The generator pins the complete 317-instruction block to these fingerprints:

| Entry | SHA-256 of normalized original records |
| --- | --- |
| `0x107122e0` | `1a92a426b5789bed300658ff087bfcebd05ffd229e192c3c9b97a84da5b0578b` |
| `0x10712a60` | `074912d3cb6b49880517225d7dd83dfd5c0d8a22250def5792f745fb8cf368b8` |
| `0x10713960` | `1efc413b607f6cc6c32cd45329b54d34b846e968d5f495aeca2b8e6f42bd85a7` |
| `0x107139c0` | `c206b64c4f3b65e30292264dd4a3ea84ad30b8a794315e2e659ed496a2ed7435` |

Normalization uses `address/assembly/ops/flows/fallthrough`, matching the existing
audits. Changed instructions or partial bodies reject generation. The manifest
adds four entries to the preserved 34 Date and three RNG entries, totaling 41.

The sampler, wrappers `0x10713a80` through `0x10713c10`, tree/list accessors,
descriptor comparison, catalog loading/release, image application/drawing and
other RNG consumers remain generated. Null-getter guards at `0x10713aec` and
`0x10713b3c` remain intact; independent getter faults still fail. No drawing
observer, presentation interface or command path is replaced. Date retains its
original asymmetric ascending-year leap behavior.

## Focused validation

The consistent Release ARM64 build passed. The generated listing remains 1,922
pages and 1,953,157 indexed instructions with zero missing static targets.
The shared target is `cm-background-selection-checks`, registered as
`modern_background_selection_equivalence`. Original and semantic runs use
identical starting state and the original bodies as oracle. Comparison covers
complete architectural register/temporary files, PC, mapped memory,
allocator/import state and interleaved memory reads/writes, including fault
prefixes. Instruction accounting and process-local diagnostics are separate.

| Executed check | Result |
| --- | --- |
| `modern_background_selection_equivalence` | PASS: 163 comparisons; own catalog/list/default/object cases 48/41/18/38; 18 faults |
| Own background boundary controls | PASS: four entry-specific mutations rejected at their own first return |
| Nonempty production dispatch | PASS: all four entries, four callbacks and four generated indirect transitions; original-mode counts zero in normal-mode witnesses |
| `original_background_selection` | PASS: club/competition/default selection, RNG consumption, null getter guards and independent getter faults |
| `modern_rng_core_equivalence` | PASS: existing 2,022 ABI/memory/effect comparisons and own negatives |
| `python3 tools/test_background_selection_audit.py` | PASS: four audit/hook tests, no skipped private-index cases |
| `python3 tools/test_rng_core_audit.py` | PASS: two tests, including the still-generated sampler on the affected page |
| Focused UBSAN | PASS: the adapter and shared harness instrumented; all 163 comparisons and four negatives; empty stderr |

Cases include null/zero-effective-count collections, one/multiple items, selected
node/accessor misses, primary/alternate low-byte selectors, enable bytes,
descriptor tree match/miss, signed16 count/category boundaries, signed32
sample-plus-salt wrap and `INT_MIN/-1`, inclusive end/renewal on either composed
selection, relocated TOC, cursor/salt/limit/spill aliases, exact original-access
mappings and unmapped entry/dependency faults. The reference bodies establish
ordered accesses; additional fixture expectations independently verify selected
payloads and sample consumption. Mixed per-entry modes test original/semantic
composition while Date/RNG remain semantic.

The first focused run passed RNG and the existing background regression. Only
the new harness and its UBSAN run were repeated while correcting alias and
null-head fixture expectations against the original: partial aliases can change
the inclusive limit as well as the cursor, and a positive list traversal through
a null node faults rather than returning a miss. That concrete fault is retained
as its own comparison. No production adapter correction was needed. The final
background check passed in 2.35 seconds; successful unrelated groups were not
repeated. No whole-suite result is claimed.

Executed build/test commands from the project root:

```sh
python3 tools/compile_recomp.py .local/recomp-input .local/recompiled --root 0x10773710 --all-analyzed
cmake -S native -B .local/aot-build -DCMAKE_BUILD_TYPE=Release \
  -DCM_GENERATED_DIR="$PWD/.local/recompiled" -DCM_ORACLE_DIR="$PWD/.local/recomp-input"
cmake --build .local/aot-build -j 4
ctest --test-dir .local/aot-build --output-on-failure -V \
  -R '^(original_background_selection|modern_background_selection_equivalence|modern_rng_core_equivalence)$'
python3 tools/test_background_selection_audit.py
python3 tools/test_rng_core_audit.py
# Harness-only fixture corrections; no regeneration or clean build.
cmake --build .local/aot-build --target cm-background-selection-checks -j 4
ctest --test-dir .local/aot-build --output-on-failure -V -R '^modern_background_selection_equivalence$'
```

UBSAN instruments only `background_selection_adapter.cpp` and
`background_selection_checks.cpp`, linked to the consistently rebuilt existing
engine libraries. It does not claim whole-engine instrumentation or ASan. Private
receipts and logs are retained with the delivery diagnostics.

Save/Load and persistence were not retested at the user's direction on October 5,
2026. They are neither a PASS nor a required pending gate for this delivery.
No Date/RNG campaign, day/week replay or whole-suite acceptance is inherited
from older handoffs. Harness dispatch evidence is distinct from live app evidence.
No live application session was opened for this delivery: club/competition/default selection is
established with the existing and new fixtures, not claimed as fresh UI coverage.
The installed app's production defaults use the four semantic implementations.

## Protection and delivery

Before rebuilding, the installed ARM64 executable was preserved in a new private
reference with SHA-256
`e6907400e9aa29896c061af4144efa99d5141fe39f119a025830f5b72bb24c41`.
The personal protection baseline covers 809 Game files, five Runtime files and
44 Runs files, including content, resource forks, Finder metadata, extended
attributes, modes, modification times and flags. It also records the presentation
preference and hashes of retained feature handoffs. Private diagnostics stay under
`.local/background-selection-reconstruction`; proprietary inputs, payloads,
fixtures, screenshots, logs and saves remain excluded from Git.

The existing protected `tools/install_native.py` ran once on October 5, 2026,
from 18:13:23.044 to 18:13:23.707 UTC (15:13 local time). The source and installed
executables are ARM64 with identical SHA-256 `c5850fc5cb5ba228fbd30d2251bab27f386e5644f9452473539097d09b42a11a`.
All four original bundled payloads match `.local/recomp-input`; the installed
plist matches production and contains no diagnostic environment or validation keys.

The postinstall receipt confirms identical personal Game/Runtime/Runs content and
metadata, unchanged presentation preference, unchanged retained handoff hashes
and an unchanged private installed reference. Personal careers were not opened
for installation validation. Original inputs, fixtures and historical candidates
were not rewritten. Publication includes only the product code, focused checks
and technical reports; the eleven pending handoffs and private evidence remain
excluded.
