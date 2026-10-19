# Complete localized phrase formatting reconstruction

The complete original phrase formatter at `0x10727890..0x107283f8` has a semantic
C++23 component implementation. It retains all **730 original instructions**, including
translated argument binding, translated rendering, base-format preparation, both marker
rewrite directions and cleanup. The component was accepted independently and is
now integrated by default in the
[905-owner localization milestone](native-localization-integration.md). Its component
models below remain distinct from the subsequent actual-service and installed
acceptance. The full-game reconstruction goal remains active.

## Whole source and data boundary

The end-exclusive original body has canonical SHA-256
`e5707d712ed3a661c02ecabb7eec4338b594e952cc2aad7583db4053ae656bdc`. Its two page fragments contain 476 and 254 indexed instructions. Two
nonindexed alignment words are pinned separately. The complete indexed graph has
**13,194 external root edges** and zero external interior edges. Five original
pointer/storage regions and two original mode/language defaults are pinned. Live
runtime values remain live: none of these data contracts substitutes constants for
language selection, formatter mode, buffers, tables or arguments.

The independent original reference disables every productive hook and retains all
730 source sites. Original records, reference source, payloads and comparison journals
remain private. The already accepted
[three-root localization lookup component](native-localization-lookup-reconstruction.md)
and shared semantic-call helper remain byte-identical.

## Original operation

The ABI takes the output and source format in r3/r4 and variadic words beginning at
r5. The 0x5e0-byte frame, r17..r31 saves, original variadic spill addresses, LR, CR,
register/temporary residues, access order and instruction accounting remain observable.
Leading spaces are copied first; the returned address points beyond those spaces.
Language bytes zero and -1 select the base path; other language bytes attempt lookup.
Lookup success uses its signed low byte, and a nonempty translated field is required.

`collect_bindings` retains the original pair of prefix/suffix modifier bytes for each
positional argument. It initializes ten pairs, counts original argument delimiters,
consumes modifier words at their original times and either formats the argument list
or fills the original mode-selected placeholder list. `split_arguments` replaces
tildes with NULs and stores guest pointers in the live pointer table. It preserves
original word32 indexing and does not introduce an argument-count correction.

`render_translation` reads the lookup metadata to reorder arguments. The original
backspace-byte placeholders select those mapped fields. A pair of -1 modifier bytes
uses the argument directly; other pairs call the existing substitution child with
live argument, prefix, suffix and byte operands. Prefix text, optional suffix text,
intervening spaces and doubled percent signs retain their original behavior.

The base path preserves two distinct marker passes. `prepare_fallback` consumes
modifier arguments, strips the original empty marker forms and scans angle-delimited
source fragments before the existing variadic formatter runs. For a formatted output
containing a brace or square-bracket marker and fewer than 1,000 bytes, the secondary
pass copies the output to the original frame buffer. Prefix markers collect the next
word and optional suffix; suffix markers scan backward through the already emitted
word. Both call the existing substitution child with the original 100/100 operands.
Length 1,000 and larger bypasses that secondary rewrite and retains the formatted
marker text. The nonzero mode retains its original separate behavior.

The three nullable lookup-owned strings are released in the original sequence.
Malformed marker syntax, missing pointers, live aliases, unchecked guest-buffer
boundaries and instruction-budget exhaustion retain their original observable
outcomes. No new repair or host string parser replaces these guest accesses.

## Component acceptance and limits

The Release ARM64 component passes **523 comparisons**: **420 complete returns**,
**100 matching stops** and **three discriminating negatives**. All **730 own source
sites** occur in complete starting-root comparisons. The new and adjacent lookup
CTests both pass; together their separate component matrices contain **1,137
comparisons** over **1,171 original sites**. Those totals do not establish productive
composition of the two components.

Comparisons include every architectural and temporary byte, PC, guest mappings and
bytes, allocator/import state, exact executed count, dependency entry/return register
bytes, released pointers and ordered read/write/clear/map payloads with instruction
PCs. Independent result corruption, inserted-then-restored writes and a corrupted
lookup child result must all be rejected.

Independent assertions verify plain language/fallback selection, low-byte lookup
success, preserved leading spaces/effective return address, positional reordering,
r5/r6 variadic spill use, translated and base prefix/suffix composition, both live
modifier bytes and the actual 1,000-byte rewrite boundary. The long boundary fixtures
have an explicitly larger instruction budget so they must complete rather than count
a budget stop as a successful length witness.

The matrix includes empty/plain/escaped text, multiple marker forms, angle-following
markers, nonzero mode, empty substitution, nullable record fields, reversed metadata,
lookup-induced mode/language mutations, required null faults, unaligned output,
source/argument/global-buffer aliases, relocated TOCs, missing mapped bytes, finite
budgets and interrupted child calls. Nine source/data/reference guards pass without
skips. Early target-name and fixture integer-width build errors were corrected;
only the final built-and-run artifact contributes acceptance.

Lookup, variadic formatting, string operations, substitution and deletion are explicit
component models. They are not actual native-service evidence, and the substitution
model is not a reconstruction of its much larger original child at `0x10728ac0`.
No generated-body dispatcher or original replay occurs inside the semantic formatter.
The four-root localization block now has productive destination hooks, actual-service
composition, focused UBSAN and one protected installation, recorded separately in
the [joint integration report](native-localization-integration.md).

## Component history and current integration

At standalone component acceptance, all **846 protected files**, eighteen unrelated
private prompts and the accepted dependency sources retained their baseline hashes.
Production then contained 901 owners, with installed ARM64 executable SHA-256
`0bef3b58a6880f6f68f8cf85842b8b8d81f5354d605ac93dcedec861fc8a173c`.

The lookup and whole formatter now share
[905-owner productive integration](native-localization-integration.md), preserving
the exact previous prefix, raw bodies, independent references and shared helper.
Current actual-service evidence, sanitizer scope, installed identity and visual
acceptance are recorded in that joint report. Catalog lifecycle/sorting and the
large substitution child retain their existing separate ownership.

Save/Load is **not retested**; performance is **not benchmarked**. PDF updates remain
paused. Whole-game completion is not established.
