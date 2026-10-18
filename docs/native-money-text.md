# Money text formatting, currency labels and settings

Status: **accepted and installed as part of the consolidated money-presentation
milestone**. The [integration report](native-money-presentation-integration.md)
records the exact 871-owner prefix, thirteen added operations, 884 installed
owners, actual original-service/caller comparisons, focused UBSAN and protected
application delivery. The isolated contract and evidence below describe this
component's own scope; their explicit external models remain distinct from the
integrated runtime corpus. The whole-game goal remains active.

## Complete original scope

| Entry | Exclusive end | Indexed instructions | Native operation |
| --- | --- | ---: | --- |
| `0x10009ef0` | `0x10009f18` | 10 | `selected_symbol_length` |
| `0x1000b150` | `0x1000b158` | 2 | `display_setting` |
| `0x1000b160` | `0x1000b410` | 172 | `currency_label` |
| `0x1000b410` | `0x1000c764` | 1,236 | `format_amount` |

All 1,420 indexed instructions are pinned, including both formatter pages. The
index contains 348 incoming root edges and no external interior entry. Eight
nonindexed zero words are pinned: seven alignment words and the skipped word
at `0x1000b524` within the digit loop. Native stages do not execute that word.

Pinned source data include sixteen 64-bit powers of ten (128 bytes), the
25-destination label table (100 bytes), two sixteen-destination layout tables
(64 bytes each), and 856 bytes of labels and format strings. Six 52-byte BSS
buffers and their TOC pointers are pinned at their initial zero image. They are
mutable runtime inputs, not immutable suffix constants. Four contiguous settings
bytes initially contain `ff 00 00 00`. The fixtures relocate the pinned banks;
native code reads guest data rather than embedding their bytes. The adjacent
currency configuration and other preference initialization bodies remain outside
this complete formatting boundary.

## Symbol, setting and label contracts

`selected_symbol_length` loads the selected string through `TOC-0x59d0`, calls
strlen and returns its signed low byte. Length 128 therefore yields -128, and
256 yields zero. A null selected-string pointer retains the required read fault.
`display_setting` returns the unsigned byte at `TOC-0x3b02`, preserving 0..255.

`currency_label` takes destination, capacity and index in r3/r4/r5. It requires
a nonnull destination and a **signed low-halfword capacity of at least 50**.
The signed low index byte must compare unsigned at most 24. Valid selection
reads the original jump bank and copies the chosen guest string, then returns
one. High index bits are discarded: 256 selects zero, while 255 is rejected.
The 25 original labels include Peso, Aus$, Real, Euro, Zloty, Escudo and Ruble;
this routine does not assign conversion rates to them.

Invalid capacity/destination and invalid index use distinct original diagnostic
paths, call four external services, clear the shared diagnostic flag and return
zero. Guest frame offsets, spills, call arguments and access order are retained.

## Digit construction and layouts

`format_amount` receives destination, capacity, high word, low word and flags in
r3..r7. It requires a nonnull destination and a **signed low-halfword capacity
of at least 25**, otherwise reports the diagnostic and returns zero. Successful
formatting clears the first output byte and builds a separate stack digit string.

Each iteration computes a signed remainder modulo the next power of ten and a
signed quotient by the current power. It prepends the signed low quotient byte
plus ASCII zero, reconstructs the low 64-bit sum using explicit products and
carries, and continues while that sum compares unsigned below the input and the
signed digit count is less than fifteen. Zero takes the zero-digit layout;
wide or negative inputs retain the original arithmetic and fifteen-character
limit. This is not replaced with unrestricted host integer-to-string conversion.

Flag bit `0x02` chooses grouped layouts. They use the separator byte at
`TOC-0x3aff`; four-digit values additionally inspect `TOC-0x3b01`, where nonzero
suppresses that grouping. The other mode chooses compact layouts and suffixes
by digit count. Optional decimal positions are trimmed when the inspected digit
is ASCII zero, and the decimal separator comes from `TOC-0x3b00`. For lengths
10..12, an empty suffix at `TOC-0x59ec` chooses a grouped fallback using the next
suffix. All formats preserve original signed-character varargs and spill order.

The final loop reads until the first NUL, substitutes ASCII space with byte
`0x7c` (`|`) and increments a signed-halfword cursor. A successful call returns
one. These are internal string bytes; final on-screen rendering is a separate
layer. With controlled symbol `"$ "`, grouping comma, decimal point and suffix M:

| Input | Mode | Internal output |
| ---: | --- | --- |
| 1,234,567 | Grouped | `$\|1,234,567` |
| 1,234,567 | Compact | `$\|1.23M` |
| 1,200,000 | Compact | `$\|1.2M` |
| 1,000,000 | Compact | `$\|1M` |

These are asserted component examples, not a named currency, natural-career
observation or performance result.

## Structure and individual evidence

The native implementation has named digit-building, grouped layout, compact
layout and space-substitution stages. Its bounded switches select one original
table destination and call direct layout code; there is no instruction/PC dispatch
loop or original-body replay. A private authoring tool helped transcribe the
repetitive static argument preparation; the emitted native code has ordinary
structured conditions and direct calls, without a runtime opcode/action table.

The component passes **2,948 comparisons**: 2,747 complete returns, 193 matching
stops and eight detected deliberate corruptions.

| Root | Comparisons | Complete returns | Matching stops | Negatives | Own complete-return sites |
| --- | ---: | ---: | ---: | ---: | ---: |
| Symbol length | 276 | 262 | 12 | 2 | 10 |
| Display setting | 262 | 257 | 3 | 2 | 2 |
| Currency label | 340 | 291 | 47 | 2 | 172 |
| Amount text | 2,070 | 1,937 | 131 | 2 | 1,236 |

Every root covers all its own instructions in complete starting comparisons,
with no structural exceptions. The independent references retain every original
instruction and disable all discovered productive hooks. Comparisons check all
architectural registers, unique residues, mapped bytes, ordered memory effects
with instruction PCs, source sequence, dependency entry/return ABI and selected
instruction accounting. Each root rejects a wrong result and a temporary extra
memory effect restored before return, with equal final images for the latter.

The existing signed remainder (`0x10773b70`) and quotient (`0x10773960`) bodies
run on both sides of the dependency boundary. Their previous proofs and productive
adapter accounting are not redefined. Strlen, string copy, byte printf and the
four diagnostic services have explicit external models here. Actual-service
composition is recorded in the integration report and is not inferred from these
model results.

The corpus includes all 256 setting, currency index and flag bytes, signed
capacity frontiers, symbol lengths through 1,022, null faults, amounts around
powers of ten, every complete layout and decimal-trimming arm, zero and wide
negative values, empty/nonempty fallback suffixes, special separator bytes,
controlled symbol/suffix strings and output aliases with auxiliary fields.
Measured instruction cuts, removed accessed bytes and interrupted callees match.
Eleven current source/data/hook checks and three focused CTests pass on ARM64 C++23 with
`-ffp-contract=off` and `-Wall -Wextra -Werror -Wpedantic`. The retained value and
display components pass their 22,508 and 4,791 comparisons.

The shared guest-call helper adds only `negate_record`, accounting for one
original instruction and preserving the record condition. Removing that new
method reproduces the previous header byte for byte. The other shared members
and accepted arithmetic/string/formatting bodies remain unchanged.

The cumulative Portuguese guide is edition 37. Save/Load is **not retested**;
performance is **not benchmarked**. Original data, generated listings, comparison
logs and the PDF source/QA journal remain private. The isolated acceptance was
followed by the consolidated productive integration
and installed artifact recorded in the integration report.
