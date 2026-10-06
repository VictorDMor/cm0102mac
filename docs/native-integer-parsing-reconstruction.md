# Semantic Mac integer parsing

## Scope and evidence boundary

Five Mac integer-conversion entries run readable C++23 in the ARM64 engine:
callback-based word32/word64 magnitude parsers, signed32 string conversion and
two decimal ABI wrappers. They form one bounded subsystem. The scanner, string
callback and facade, descriptor stub, unsigned division, floating parsing,
formatter, locale/table initialization, streams, allocation, gameplay and
serialization retain their existing implementations. No visual or performance
improvement is claimed.

The starting checkout and freshly queried `origin/main` were
`2a7eed69ac6f2d3cba704768661dabf788bc6f3a`, with no tracked changes. The remote is
`git@github.com:VictorDMor/cm0102mac.git`; no applicable `AGENTS.md` was found in
the repository or inspected ancestors. Fifteen private untracked handoffs and
five tracked prompts are preserved. The existing instruction index is read-only;
no new Ghidra export or proprietary listing is published.

Earlier Date/RNG/background/collection/string/memory reports are historical
acceptance evidence. The prior memory build was later observed running its
opening and the Reading squad screen, and the user confirmed it worked. That
observation followed the technical report's installation without UI testing.
It does not validate this new integer block.

The user explicitly waived Save/Load, save/exit/reopen/load, career comparisons
and serialization auditing. Persistence is **not retested**, neither passed nor
a pending gate. New offline equivalence and the short installed-app smoke test
are recorded separately below.

## Pinned boundaries and production integration

Fingerprints cover address-ordered original `address/assembly/ops/flows/fallthrough`
records, compact sorted-key UTF-8 JSON. Indexed gaps are intentional.

| Entry | Exclusive end | Instructions | SHA-256 |
| --- | --- | ---: | --- |
| `0x1077ee20` | `0x1077f180` | 215 | `0e6f94e1876cc0214da11fe6867d125dc06d073581ecc399217f3bd399394098` |
| `0x1077f180` | `0x1077f564` | 249 | `b99ed550fab592d2395d29a82f8631d3e08c0c3fa71574c256661d41ba1e8e63` |
| `0x1077f570` | `0x1077f664` | 61 | `0221610ecf6b5d56509dc7fae6948e5b117b2bd895c9bd3d8e413459597f32a7` |
| `0x1077f670` | `0x1077f698` | 10 | `2e45e76b5215652dce79a0ecc6023e610fef5053cb06f9800fd9424152783eb1` |
| `0x1077f6a0` | `0x1077f6c8` | 10 | `6a14b3d0e2dd4d19b0c5a4af9fd370a2e6b9436c4534fe0bff445a3168e609e9` |

The 545-instruction block has zero indexed external static edges into its
interiors. The index audit scans flows and fallthroughs and rejects a new
interior edge, including into the second page of parser32. It does not classify
unknown dynamic destinations. Parser32 crosses pages `0x1077e` and `0x1077f`:
the compiler verifies the whole body before emitting completion and separately
pins each complete page fragment (119 and 96 instructions). A complete fragment
is accepted as a page, but never accepted as the whole routine.

Destination hooks precede the original first operation on both pages. Local
calls/gotos, cross-page dispatch and generated indirect transfers reach them.
The manifest appends these five to the exact ordered prior 66, for **71 semantic
replacements**. The previous manifest prefix SHA-256 is
`186bf921ba5058f50ce04bd2fb5222f57d2c2045e72373b4dc46db08293705ab`.
Date's ascending-year leap asymmetry and club/competition guards remain intact.

Private global/per-entry original selection is independent of the six earlier
modules. `CM_AOT_INTEGER_PARSING_ORIGINAL` selects the probe reference only.
Counters, phase-escape diagnostics and boundary observers are process-local,
not serialized, and add no guest reads or replace existing observers. Production
defaults select all five semantic bodies.

## Original contracts and semantic phases

The magnitude ABI receives base/field limit/descriptor/context/count output/
sign output/overflow output in r3 through r9. Parser32 returns magnitude in r3;
parser64 returns high:low in r3:r4. Both save their own nonvolatile register
ranges, LR and backchain in distinct guest frames. Ordered word spills/restores
also retain the generated register-space cursor. Zero/negative limits and
invalid bases initialize flags and still execute the final unread callback.
There is no invented pointer guard or host conversion routine.

Named phases handle initial whitespace, sign, leading zero, hexadecimal prefix,
digits and termination. Whitespace uses the guest classification mask and does
not consume field width. Signs and prefixes do consume it. Lookahead is fetched
before checking the field boundary, then the last character is returned through
the callback. EOF and no-conversion behavior remain distinct: for example, `0x`
advances the string callback but reports count zero, while base-zero `09`
converts the first byte and leaves the cursor at `9`. A core's sign flag remains
separate from its unsigned magnitude; only its wrapper applies negation.

Classification/mapping pointers are captured from `r2-0x5a48`/`r2-0x62bc`.
Their contents remain live guest bytes, including two distinct mapping reads
for an accepted letter. Digits use signed comparisons against base, and invalid
characters are returned with their original callback protocol. Classification
mutations can create unusual or negative mapped digits; the arithmetic retains
the original signed-extension and unsigned overflow comparisons.

Phase dispatch reloads the table pointer at `r2-0x3b24` or `r2-0x3b28`, loads
its selected destination and writes CTR. The observed tables at `0x1091ce48` /
`0x1091ce8c` target whitespace/sign, leading-zero, prefix, digit or loop-control
phases. The live destination selects the semantic phase, so discriminating
redirections and relocated TOCs retain their effects. An uncharacterized target
uses the existing generated dynamic-transfer path and increments a separate
phase-escape counter; an uncompiled destination produces the original runtime
error. This compatibility escape is outside semantic phase coverage. Every
normal equivalence witness requires zero escapes; no original replaced body
executes as normal fallback.

All callbacks retain the generated `0x10781320` stub. Each call reloads the
mutable descriptor's entry/TOC, spills the current TOC in its frame, transfers
through CTR and restores TOC after return. Callback clobbers, read/unread events,
aliased context and descriptor faults remain observable. Parser64 retains the
existing guest `0x10773870` unsigned division path to calculate its overflow
threshold; division is not reconstructed here.

Overflow sets the output flag but accumulation continues modulo 2^32/2^64.
The before-multiplication threshold check and after-multiplication headroom check
remain separate, with their ordered stores, live digits, CR and XER carry.
Termination records count, returns the lookahead, then restores its own ABI.

Signed32 conversion prepares the original guest string context and output flags
in a 96-byte frame. It loads the descriptor from `r2-0x5a44` and calls parser32
with field limit `0x7fffffff`. It writes an optional end pointer before reading
overflow/sign flags and deciding saturation. No conversion writes the original
input pointer. It allows magnitude `0x80000000` only for a negative sign,
saturates range errors to INT32_MIN/MAX, writes `0x22` to the live pointer at
`r2-0x5a58`, and preserves an existing error value on success. End pointer,
flags and spills can alias, so later loads use actual guest memory. Both decimal
entries select base ten/null end pointer and preserve distinct 64-byte frames,
LR values and own return boundaries.

## Focused equivalence

`cm-integer-parsing-checks` / `modern_integer_parsing_equivalence` clones the
same original input state while all prior modules remain semantic. It compares
all architectural register and temporary bytes, PC/LR/CTR/TOC/CR/XER, every mapped
memory byte, allocator/import state, ordered read/write/map/clear effects and
callback arguments/events, including prefixes before faults. Only instruction
accounting and private diagnostics are excluded; differences are not normalized.

Independent expectations cover magnitude/wrap, consumed count, sign/overflow,
end pointer, prior error/range error, callback mode and cursor. The small matrix
includes bases 0/2/8/10/16/36 and invalid bases, empty/whitespace/sign/prefix/high-
byte/EOF inputs, case mapping, field boundaries and extreme finite EOF limits,
signed32/unsigned32/unsigned64 limits and overflow. Mutations exercise live
phase/classification/mapping tables, mapping rereads, relocated TOC, descriptor
changes, alternate generated callback and observable clobbers. Minimal accessed
mappings and missing accessed bytes cover frame, TOC, table, descriptor, input,
context and output fault prefixes. Outputs/spills/end-pointer/flags aliases
retain ordered effects.

Five own return negatives distinguish nested children and parents, with an
additional parser-child negative stopping before its decimal parent. Mixed modes
cover original wrapper/semantic parser and the inverse. Five generated indirect
entry transitions require semantic counters and zero original selection.
Two generated string-facade/scanner compositions verify a signed32 destination
and an unsigned64 destination, assignment count, `%n` consumption and callback
cursor. These are offline integration checks, not UI or persistence evidence.

## Executed validation and delivery

One regeneration retained 1,953,157 instructions across 1,922 pages, with zero
missing/pending targets and 71 replacements. A consistent Release ARM64 build
rebuilt the dependent objects after extending the State diagnostics. Compilation
and linking completed without warnings or errors. A small adapter edit made
during that build was included in the final link; no second regeneration was
needed. Final hook-validation changes emit byte-identical parser pages.

| Executed check | Result |
| --- | --- |
| `modern_integer_parsing_equivalence` | PASS: 435 comparisons, including 222 unmapped-access faults |
| Own/nested return controls | PASS: five own negatives and one parser-child negative |
| Normal dispatch | PASS: five nonempty entries, zero original-selected calls and zero phase escapes |
| Mixed dependency modes | PASS: two wrapper/parser selections |
| Generated indirect transitions | PASS: five own entries |
| Generated scanner compositions | PASS: `%d%n` and `%llu%n`, destination/count/cursor checked |
| `modern_string_core_equivalence` | PASS |
| `modern_memory_primitives_equivalence` | PASS |
| `original_unsigned_division` | PASS |
| `python3 tools/test_integer_parsing_audit.py` | PASS: five body/page/hook/edge/manifest/dependency tests, no skips |
| Existing memory/string/collection/background audits | PASS: 5/5/5/4 tests, no private-index skips |
| Focused UBSAN | PASS: all 435 comparisons and controls, empty stderr |

The initial CTest group passed the three existing regressions. Two expectations
in the new fixture were corrected: no-conversion count is not the callback's
advanced cursor, and the existing memory observer runs before the load value is
materialized. The controlled alternate callback now supplies its intended
sequence at that point. The letter fixture changes the second mapping load,
separately discriminating the reread. Only the new harness was rebuilt/rerun;
no production equivalence correction or repeated green regression was needed.
Final diff review corrected a harness witness loop left with six iterations for
five entries. The new CTest and focused UBSAN were rerun after that correction;
the productive adapter and installed executable were unchanged.

UBSAN instruments only `integer_parsing_adapter.cpp` and
`integer_parsing_checks.cpp`, using C++23/ARM64, `-O1`, `-ffp-contract=off`,
`-fsanitize=undefined` and `-fno-sanitize-recover=all`, linked to consistent
engine libraries. It does not instrument the entire engine. No full suite,
Date/RNG/allocator, season/day/week replay, benchmarks or career campaign ran.

### Protected installation

The prior installed app was copied with resource forks/extended attributes to
`.local/integer-parsing-reconstruction/reference/installed.app` and fully
compared. Its ARM64 executable hash is
`6c00be6327cbbee65a57078320bfd2b6c530e2d1a7d9d34ba8a001e59b20e6b5`.
Twenty-four historical reference/candidate/linker bundles are protected, including
the four earlier subsystem references. The baseline covers 809 Game files, five
Runtime files, 47 Runs files, the LFS snapshot, four original input files and
20 retained prompts (fifteen private, five tracked). Presentation.plist is absent.

The compiler/linker bundle is retained separately as `source-linker.app`. The
source was supplied its original payloads, ad-hoc signed and retained as
`candidate.app`. `tools/install_native.py` ran **once**, October 6, 2026,
21:20:36.099117–21:20:36.890268 UTC (18:20:36 in São Paulo), with no app session
running. Existing-game and English club-index checks passed.

| Artifact | Architecture | Executable SHA-256 |
| --- | --- | --- |
| Retained compiler/linker output | ARM64 | `bdb431cc58581e00999cca8ee9d39a4f79a050282451ecf861f62c93c42d29d1` |
| Signed source / candidate / installed app | ARM64 | `191e7c599671efb7f0f1bed0273020693c86eddc59a5d40349f0a79e28d845b4` |
| New prior-installed reference | ARM64 | `6c00be6327cbbee65a57078320bfd2b6c530e2d1a7d9d34ba8a001e59b20e6b5` |

Source/candidate/installed/reference pass strict deep signature verification.
All four bundled original files match the existing inputs. Production plists
are identical without diagnostic/validation keys or LSEnvironment; no fixtures
or private diagnostic configuration files are bundled. Identity comes from the executable hash,
not the app folder timestamp. The immediate postinstall check confirms identical
personal contents and resource forks/Finder information/extended attributes/
modes/mtimes/flags, unchanged inputs, saves, prompts and protected bundles.
This preservation check precedes the intentionally separate smoke run.

### Installed-app smoke test

The signed installed app was launched through Computer Use at its exact
Applications path. Its initial window and rendered Prepare Game menu were
observed. Command-O reached the original Load saved game chooser; Command-[
returned to Prepare Game without selecting/loading a career. Command-Q showed
the original exit confirmation. A coordinate Yes attempt did not close it;
Return confirmed normal exit. The log records a normal original application
exit; the process is gone, and no fatal/unimplemented/uncompiled/unmapped/budget
failure was found. No concurrent user interaction was observed.

The smoke receipt is timestamped October 6, 2026, 21:22:29 UTC (18:22:29 in
São Paulo). The tested installed ARM64 hash is
`191e7c599671efb7f0f1bed0273020693c86eddc59a5d40349f0a79e28d845b4`.
The run selected all five integer entries as modern, with zero original calls
and phase escapes, but did not invoke them. Their individual nonempty coverage
comes from the harness, not this startup. No days, match or career were opened.
Persistence remains **not retested**. The chooser's catalog/header inspection
is not a save-load acceptance test.

The smoke run intentionally created a run log and updated preferences. The
post-smoke check retains identical Game files, the LFS snapshot, prompts and
protected references/bundles; it does not claim byte-identical Runtime/Runs.

### Publication boundary

Only the semantic adapter, focused checks, audits, integration and technical
reports are committed to the existing private `origin/main`. Private handoffs,
indices, proprietary payloads, saved games, generated sources, fixtures, logs,
images, receipts and bundles are excluded. Ignore and LFS rules are unchanged.
Publication uses a normal commit/push followed by a fresh remote-SHA check.
