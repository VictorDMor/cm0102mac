# Contract wage and end presentation

Status: **prepared and accepted as an isolated component**. Two complete native
operations match independent original references. The installed application
remains at `d0910402c867a5d0faa42830e1b7d1fb55c70541`, with 871 unique semantic
owners. The prospective count of 873 is not an installed result. Productive
integration, actual-service/caller comparisons, focused UBSAN and installation
remain for a consolidated milestone. The complete-game goal remains active.

## Complete boundary and provenance

| Entry | Exclusive end | Indexed instructions | Native operation |
| --- | --- | ---: | --- |
| `0x10372590` | `0x10372724` | 101 | `format_wage_text` |
| `0x10372730` | `0x103727bc` | 35 | `format_contract_end` |

The index contains 49 incoming root edges: 36 direct callers of wage presentation
and 13 of end presentation. There are no external interior entries. All 136
indexed instructions and four nonindexed zero alignment words are pinned.
The 251-byte label bank at `0x1080bde8` and its TOC slot `0x108b6cec` have SHA-256
`67a676a79f654aebe9e5575aa84303a449b1da4f0c23e2b236da5ddb41e545ef`.
Original labels are read through guest memory; they are not embedded in the
native operations.

This is the complete adjacent presentation boundary after draft preparation and
before the separately owned full contract initializer `0x103727c0`. No existing
Date, formatted-output, draft or storage owner is counted again.

## Wage text

The arguments are record, destination, presentation mode and numeric flags in
r3/r4/r5/r6. The function reads word `record+0x0c` and the preference flags at
`*(*(TOC-0x7ac8))+4`. Weekly bit `0x200000` takes priority and leaves the value
unchanged. Otherwise monthly bit `0x400000` multiplies it by four; all other
cases multiply it by 52. Multiplication wraps in 32 bits before the numeric
service observes the temporary word. The record itself is not updated by these
stores.

For a stored value of 1,000, the three period inputs are therefore 1,000, 4,000
and 52,000, before the shared numeric services perform their own conversion and
formatting. This describes display preparation, not an inferred currency or a
change to the stored wage. The conversion/formatting flag is eight for weekly
or monthly, otherwise four, ORed with the **signed low byte** of incoming r6.
The converted r3/r4 pair is forwarded to r5/r6 of the next formatter, together
with a 200-byte temporary buffer and the same signed flag.

Only the low signed byte of the presentation mode is used. Zero requests plain
numeric text; two selects abbreviated period labels; every other nonzero value
selects the long labels. Consequently 256 behaves like zero and 258 like two.
The period preferences are read again after the two numeric calls. Tests include
an explicit helper mutation between the scale selection and suffix selection,
so caching the first preference word would change the observed behavior.

Suffix selection prioritizes weekly, then monthly, then yearly bit `0x800000`.
If no period bit is set in either nonzero presentation mode, the caller's output
is left unchanged, even though numeric conversion has run. In that path r3
retains the reloaded preference word. An unconditional suffix, default output or
normalized destination return would change the original behavior.

## End text

The record and destination arrive in r3/r4. The signed packed type derived from
byte `record+0x38` ignores the low two bits. Type three selects the expired label;
type four selects the free-transfer template with its original empty
substitution; type one selects the rolling label. These paths trust the stored
type: this function does not compare the current date with the contract end.
Every other type delegates `record+0x30` to the existing Date formatter, retaining
the original capacity of 200 and format byte of 255.

All paths preserve the final dependency's return residue. Record, destination
and the wage preference chain remain mandatory where accessed. Positive null
faults are retained. Output aliases with the record, date, packed type, label
bank or wage temporary buffer preserve the original order of reads and writes.

## Isolated evidence

The component passes **4,791 comparisons**: 4,457 complete returns, 330 matching
stops and four rejected deliberate corruptions.

| Root | Comparisons | Complete returns | Matching stops | Negatives | Complete-return sites |
| --- | ---: | ---: | ---: | ---: | ---: |
| Wage text | 4,433 | 4,153 | 278 | 2 | 101 |
| End text | 358 | 304 | 52 | 2 | 35 |

All 136 original sites occur in complete returns, without structural exceptions.
Each comparison checks all architectural registers, unique/temporary residues,
mapped memory bytes, ordered reads/writes with instruction PCs, source order,
dependency entry/return ABI and instruction accounting. The independent reference
disables all discovered productive hooks and retains every original instruction.
The native operations use named stages and structured conditions, not original
body replay or instruction/PC dispatch. The shared guest-call engine is unchanged.

Five explicit external models cover numeric conversion `0x10009f20`, numeric
formatting `0x1000b410`, plain output `0x1077b720`, localized/template output
`0x10727890` and Date presentation `0x10715e00`. Their inputs, output bytes and
volatile residues are observed, but their own algorithms are not proved here.
In particular, model-rendered text is not actual localization or currency-service
evidence.

The corpus covers all eight period-bit combinations against all 256 mode bytes
and all 256 flag bytes; signed/wrapped amount frontiers; upper-byte residues;
late preference changes; all 256 packed type bytes; date fields; output aliases;
final return forwarding; measured budget cuts; removed accessed bytes; null
accesses and interrupted callees. Independent assertions check period precedence,
wrapped multiplication, signed flags, converted-pair forwarding, label selection,
no-suffix output preservation, unchanged stored fields and Date delegation.

Each root rejects an own-result corruption and an extra memory effect restored
before return. The restored-effect negative first requires equal final images,
so an image-only comparator cannot accept it. Nine source/data checks pass,
including the exact 871-owner prefix, absent productive owners, full body and
reference guards, every label-bank byte, padding and external-interior rejection.
The focused CTest `contract_display_component_equivalence` passes on ARM64 with
C++23, `-ffp-contract=off` and `-Wall -Wextra -Werror -Wpedantic`.

The cumulative Portuguese guide, edition 34, explains these rules and the
prepared status. Actual services/callers, productive dispatch, focused UBSAN and
installation remain for a consolidated milestone. Save/Load is **not retested**;
performance is **not benchmarked**. The complete-game goal remains active.
