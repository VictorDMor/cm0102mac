# Semantic Mac formatted input

## Scope and evidence boundary

Seven Mac formatted-input entries now have semantic C++23 bodies in the native
ARM64 engine: format analysis, the descriptor-based scanner, string and stream
callbacks, a guest-argument string facade, and both variadic facades. Production
selects all seven. The output formatter, floating parser, character helper,
locale initialization, stream orientation/read/unread/refill services, files,
gameplay and serialization retain their existing implementations.

The starting tracked checkout and a fresh `origin/main` query both identified
`c4a806699e2153002b041825bccb3f41afcbbdca` on `main`, remote
`git@github.com:VictorDMor/cm0102mac.git`. No applicable `AGENTS.md` was found.
Sixteen private untracked prompts and the five tracked handoffs are preserved.
The existing instruction index was opened read-only; no Ghidra export or
proprietary listing is published. Prior Date/RNG/background/collection/string/
memory/integer reports describe historical acceptance, not this scanner's checks.

Save/Load, save/exit/reopen/load, career/day/week/season campaigns, matches,
serialization audits, benchmarks and the full suite are expressly out of scope.
Persistence is **not retested**, neither passed nor a required pending gate.
The earlier ascending-year Date leap asymmetry is preserved.

## Pins and integration

Fingerprints cover address-ordered `address/assembly/ops/flows/fallthrough`
records, compact sorted-key UTF-8 JSON. Indexed gaps are intentional.

| Entry | Exclusive end | Instructions | SHA-256 |
| --- | --- | ---: | --- |
| `0x1077b7a0` | `0x1077bf20` | 478 | `1f92666f58a9485bc5f015c13c3c3942fc756a123d6270a52717c6866e025c9a` |
| `0x1077bf20` | `0x1077d8cc` | 1,631 | `a8d9ca1f82d1dae1139544ce0bf135f361b2f6eae0cbef423c3d6e7fc5cb5108` |
| `0x1077d960` | `0x1077d9f0` | 36 | `9d8bfe52bc79d144184931008bacb7767f6d95c36ebe2a3b92a70c23139e7967` |
| `0x1077da70` | `0x1077dac8` | 22 | `d247cbccf4af1cd8df61149e1c50d024a4186777f0504193a0d9e7ee07980e12` |
| `0x1077dad0` | `0x1077db14` | 17 | `7db0c98c7415dc19f6e9a6df73e14b24517a032d2c9125b1aa833485934ea585` |
| `0x1077d8d0` | `0x1077d954` | 33 | `862b4f73bb03b4db4aae69453b16d81232966f15819433dc28b397eb5c5d5d19` |
| `0x1077d9f0` | `0x1077da6c` | 31 | `b840d438996ce92f30b4dda4c874385828932d89549225139ed261692923c91b` |

The total is 2,248 indexed instructions. The scanner has three independently
pinned complete fragments: page `0x1077b` has 55 instructions and hash
`b4e3299368839bba2f7db7d215ad64a4606a977bfc372aedffa64604e835b824`;
`0x1077c` has 1,020 and
`30f13561b7b1953663678bea999776d063c76b56117e61ec08fcbac44d49a28e`;
`0x1077d` has 556 and
`cecc5cb7384f46ac586e121cb142fb8af11b7e2a7c199ec01774227bbe7dce48`.
The compiler checks complete bodies before emitting the manifest and checks each
page fragment against its own pin. A first-page fragment cannot pass as the body.
The fresh index audit rejects static external edges into all seven interiors;
none were present. Unknown dynamic transfers are not classified by that audit.
Fresh direct-call counts are 1/2/0/1/13/0/3 in table order; callback descriptor
references are indirect. These counts establish static relevance, not runtime coverage.

Hooks precede the first original operation at destination labels on pages
`0x1077b` and `0x1077d`, covering local goto, page dispatch and indirect transfer.
The manifest has **78 semantic replacements**, preserving the exact prior 71
in order. Their compact sorted-key manifest SHA-256 remains
`1862099e2688776c5b9676d9668069638c739d79ca393382497c86fe45c3f979`;
the integer slice remains `[66:71]`. Existing fixups and dependency hooks remain.

Private global/per-entry original selection, counters, boundary observers and
conversion-dispatch escapes are independent of previous modules and process-local,
never serialized. The probe-only `CM_AOT_FORMATTED_INPUT_ORIGINAL` switch supplies
reference execution. Production defaults select semantic bodies; no normal path
calls its own old body or uses a host scanner, Unicode classification or locale.

## Format contracts

Format analysis receives format / guest argument-cursor pointer / output record
in r3/r4/r5, returns the next format address in r3, and uses a 176-byte guest frame.
Ordered saves/restores of r21–r31 retain the register-space spill cursor. Template
loads from the pointer at `r2-0x3b40` initialize a live 60-byte frame record. Output
copies retain field-by-field load/store order and register residues, including
distinct `%%` and zero-width paths; overlapping record/frame/cursor memory is live.

| Record offset | Meaning |
| --- | --- |
| 0 / 1 / 2 / 3 | suppression / explicit-width flag / destination modifier / conversion byte |
| 4 | field width, initially `0x7fffffff` in the supplied template |
| 8 / 12 | repeated-conversion flag / packed lane limit |
| 16 / 20 / 24 | current delimiter pointer / retained delimiter pointer / delimiter length |
| 28–59 | 256-bit character membership map |

Named parsing operations preserve `%%`, suppression, wrapped decimal width,
`h`, `hh`, `l`, `ll`, `L`, `v` before/after a modifier, invalid/truncated formats,
scansets, inversion, ranges and an initial `]`. Width zero marks the conversion
invalid and still returns its own partially initialized record. Classification
reads the live guest table at `r2-0x5a48`.

`v` requests repeated conversions packed into a 16-byte accumulator; it is not
host varargs syntax. Limits and unusual modifier behavior are preserved, including
an `hh` packed conversion that stores no lane data. `@` consumes one guest
argument as a delimiter string, calls the existing semantic length entry
`0x1077dbd0`, trims literal ASCII spaces from both ends and restarts conversion
parsing once. Another unrecognized prefix supplies a one-byte delimiter. These
prefixes preserve suppression, argument-cursor advance and frame effects.

The conversion table pointer at `r2-0x3b44` and its destination are read live;
CTR retains the loaded target. Semantic operations select that destination:
integer validation, floating modifier validation, pointer conversion to `x`,
character/string modifier validation, scanset parsing, delimiter prefix or final
record copy. A redirected `d` table entry selecting pointer validation independently
produces `x` and modifier 3. Unknown targets transfer through the existing generated
runtime until the caller, with a separate private escape counter. The explicit
uncompiled-target control preserves its complete error/effect prefix; escapes
are excluded from normal semantic coverage.

## Scanner and facades

The scanner receives descriptor/context/format/guest argument vector in r3–r6,
saves r13–r31 and LR, and uses a 240-byte frame. Named operations handle format
whitespace, literals, conversion parsing, numeric input, characters, strings/
scansets, percent, count storage, packed assignments and termination. Guest
accesses, big-endian stores, CR/XER/CTR/LR/TOC and observable registers remain live.
Assignment count, the original consumption counter and the frame's completed-conversion count are
separate; EOF with zero completed conversions returns -1 even if assignment count
is zero for another reason. `%n` stores consumption without adding an assignment.

Magnitude32/64 calls remain `0x1077ee20`/`0x1077f180`, with their independent
semantic selection. The scanner applies sign and narrowing itself, preserving
modulo overflow rather than applying the signed textual wrapper's saturation.
Signed/unsigned destinations retain word32, byte, halfword and high:low word64
stores, including low-before-high ordering. `%n` supports the corresponding sizes.
Repeated numeric/float/character conversions retain packed writes, lookahead,
delimiter matching, suppression and the original mode-1 whitespace loop. Explicit
nonspace delimiters keep those adversarial fixtures bounded. Suppressed character
conversions retain the original omission of their bytes from the consumption counter.

Every callback uses generated descriptor stub `0x10781320`. Entry and TOC are
reloaded for each call, the caller's TOC is spilled/restored, and callback arguments,
clobbers, modes 0/1/2 and faults stay observable. The scanner is not specialized
to a host string. A noncontiguous generated callback fixture independently returns
1, 2 and a terminator while preserving the scanner's count and destinations.

The string callback stores cursor and EOF in guest context words at offsets 0/4.
Read NUL sets EOF and returns -1 without advancing. Other bytes advance modulo
2^32, including high bytes. Unread clears EOF or decrements the cursor, returns
r4, and has no extra host guard. Query reads the EOF word; other modes return zero.
The vector facade initializes its context in a 64-byte frame and rejects null/
initial-NUL input before scanning. The variadic facade preserves ordered r4–r10
spills and calls that vector facade using guest stack slots.

The stream callback has its own 64-byte frame and delegates read/unread to
`0x10775c10`/`0x10775d80`. Query loads byte 13 first, and reads byte 12 only if
needed. The stream variadic facade preserves r31/LR and r4–r10 spills even for a
null stream. `0x1077fa50` receives orientation -1; only its negative return enters
the scanner through the live descriptor at `r2-0x5a40`. Its frame and return
boundary remain independent. Stream fixtures use artificial guest objects with
controlled orientation, buffered cursor/count at offsets 40/44 and generated
unread state. No files or network are opened; no OS refill is induced.

Character paths retain `0x10778170`, including the original differing pointer
increments for `lc` versus `ls`/scansets. Floating conversions retain parser
`0x1077de10`, float rounding/status and float/double stores. Existing decimal input
reaches the unimplemented `CarbonLib::dec2num` bridge; NaN reaches the unimplemented
`CarbonLib::nan` bridge. Both failures are compared exactly, without replacing
those dependencies. Positive/negative infinity exercise successful float/double/
long-modifier, suppression and packed stores. The infinity parser consumes a
lookahead without returning it; the packed fixture supplies two delimiter bytes
to preserve that legacy behavior. A separate original hexadecimal-float attempt
hit its 100,000-instruction budget and is not counted as successful coverage.
These limitations belong to the retained parser/services, not newly reconstructed
float or stream implementations.

## Focused equivalence

`cm-formatted-input-checks` / `modern_formatted_input_equivalence` clones each
input State, leaving prior modules semantic except explicit mixed-mode controls.
Comparison covers all architectural and temporary bytes, mapped memory, ordered
reads/writes/maps/clears, callback arguments/events, PC/LR/CTR/TOC/CR/XER and import/
allocator state at returns and faults. Only accounting/private diagnostics are
excluded. No ABI differences are normalized.

Independent expectations cover record/cursor fields, argument advancement,
assignments, `%n`, signed/unsigned/wrapped values, packed lanes, guard bytes,
string cursor, stream buffer/count/unread state and flags. The focused matrix
includes literals/space/percent, mismatch, EOF, null/empty input, suppression,
widths, bases, overflow, supported sizes, characters/strings/scansets and extensions.
It adds live template/ctype/destination/descriptor mutations, relocated TOCs,
callback clobbers, generic callbacks, wrap and representative aliases. Minimal
access mappings and omitted accessed bytes test exact fault prefixes. Seven own
return negatives and a format-child negative reject before the parent. Generated
indirect transitions cover all seven entries. Twelve mixed modes discriminate
facade/scanner, scanner/format, scanner/parser and parser/callback, including stream.

## Final validation, installation and publication

One regeneration retained 1,953,157 instructions across 1,922 pages with zero
missing/pending static targets and 78 replacements. The Release ARM64 build
rebuilt all State-dependent units. An incremental final relink included adapter
repairs made during the build. Both builds completed without warnings/errors.
Final audit tightening emits byte-identical generated pages; no second regeneration
was needed. Private development linkage accelerated differential diagnosis during
the full rebuild; only the final consistent libraries support acceptance below.

| Executed check | Result |
| --- | --- |
| `modern_formatted_input_equivalence` | PASS: 931 comparisons, including 645 expected faults |
| Own/nested return controls | PASS: seven own negatives and one format-child negative |
| Normal semantic coverage | PASS: all seven entries nonempty, zero original-selected calls and escapes |
| Generated indirect transitions | PASS: all seven entries |
| Mixed dependency modes | PASS: 12 selections including stream |
| `modern_integer_parsing_equivalence` | PASS: 435 comparisons, including 222 faults and both `%d%n`/`%llu%n` compositions |
| `modern_string_core_equivalence` | PASS: 314 comparisons, including 62 faults |
| New Python audit | PASS: six body/page/hook/edge/manifest/dependency tests, no skips |
| Existing integer/memory/string/collection/background audits | PASS: 5/5/5/5/4 tests, no skips |
| Focused UBSAN | PASS: 931 comparisons and controls, empty stderr |

Normal successful semantic call witnesses, excluding fault/escape/mixed-original
cases, are 318 / 192 / 1,596 / 139 / 87 / 108 / 13 in entry-table order. Separate
negative/boundary checks are not added to the 931 comparison count. The 645 faults
include 637 omitted-byte mapping cases, three decimal-service failures, one NaN
service failure, one unknown destination and three deliberately corrupted-LR
aliases. These are expected matching failures, not successful conversions.

The initial fixture exposed missing decimal/NaN services and a bounded original
hexadecimal-parser loop. Float coverage was changed to executable infinity paths
and exact service-failure comparisons. Packed infinity fixtures were adjusted for
the retained parser's consumed lookahead. Corrupting an aliased saved LR is expected
to fail at the same uncompiled target. One production repair corrected the distinct
repeat callback link/fault sites for signed versus unsigned suppression. No ABI
normalization or retained dependency rewrite was used to obtain agreement.

UBSAN instruments only `formatted_input_adapter.cpp` and
`formatted_input_checks.cpp` with C++23/ARM64, `-O1`, `-ffp-contract=off`,
`-fsanitize=undefined` and `-fno-sanitize-recover=all`, linked to final consistent
engine libraries. No full suite or broader historical campaign ran.

### Protected installation

The prior installed app was copied with resource forks/extended attributes into
`.local/formatted-input-reconstruction/reference/installed.app` and fully compared.
Its ARM64 hash remains
`191e7c599671efb7f0f1bed0273020693c86eddc59a5d40349f0a79e28d845b4`.
The protection baseline retains 114 historical reference/candidate/validation
bundles, 809 Game / 5 Runtime / 48 Runs files, the LFS snapshot, original inputs
and all 21 handoffs. No user app session was running at installation.

`tools/install_native.py` ran once on October 6, 2026, from
20:10:52.463412 to 20:10:53.434574 in São Paulo
(23:10:52.463412–23:10:53.434574 UTC). Existing-game and private English club-index
verification passed. Source linker output is retained separately; the source was
supplied its four original payload files, ad-hoc signed and retained as a candidate.

| Artifact | Architecture | Executable SHA-256 |
| --- | --- | --- |
| Retained linker output | ARM64 | `f363b8279932f15c1db9808b75ccd891eada4b1b8b514d3e456974876d82faa4` |
| Signed source / candidate / installed app | ARM64 | `2e88cca135176aed98fe9771c06a269463e7fd16ebc14161fd66bd9c4492437e` |
| New prior-installed reference | ARM64 | `191e7c599671efb7f0f1bed0273020693c86eddc59a5d40349f0a79e28d845b4` |

Source/candidate/installed/reference pass strict deep signature verification.
All four bundled originals match their existing input identities. Production
plists agree without diagnostic/validation keys or LSEnvironment; no fixture or
private validation configuration is bundled. Identity is verified from the inner
executable, not an app-folder timestamp. The immediate postinstall inventory
retains personal content, forks, Finder information, xattrs, modes, mtimes, flags,
the LFS snapshot, handoffs and historical bundles. This check precedes the separate
intentional smoke run; it does not claim unchanged Runtime/Runs afterward.

### Installed-app smoke test

The installed Applications-path app was launched through Computer Use. Its
opening logo and rendered Prepare Game menu were observed. Command-O opened the
original saved-game chooser; Command-[ returned to Prepare Game without selecting
or loading a career. Command-Q opened the original exit confirmation and Return
confirmed it. The process is gone and the log records normal original application
exit, without fatal/unimplemented/uncompiled/unmapped/budget failures. The final
AX query reported no eligible process after exit, consistent with the process/log
checks. No concurrent user interaction was observed.

The receipt was recorded October 6, 2026, 20:12:02.916966 in São Paulo
(23:12:02.916966 UTC). The tested installed ARM64 hash is
`2e88cca135176aed98fe9771c06a269463e7fd16ebc14161fd66bd9c4492437e`.
All seven entries selected modern with zero original calls and escapes, but startup
and this chooser did not invoke them. Their nonempty coverage comes from the
harness. The chooser is UI-response evidence, not persistence acceptance.
Persistence remains **not retested**; no career, days or match were entered.

The smoke intentionally produced a run log and updated preferences. Post-smoke
checks preserve Game content/metadata, the LFS snapshot, handoffs and installed
reference. Counts are 809 Game / 5 Runtime / 49 Runs files. Runtime/Runs are not
claimed byte-identical after this intentional execution.

### Publication

Only the adapter, checks, audits, production integration and technical reports
belong to this delivery. Private handoffs, indices, original payloads, generated
sources, saved games, logs, images, receipts, fixtures and bundles are excluded.
Ignore/LFS rules are unchanged. Publication uses a normal commit/push to the
existing private `origin/main`, followed by a fresh remote-SHA query.
