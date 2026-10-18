# Monetary value conversion, rounding and editing

Status: **prepared and accepted as an isolated component**. Seven complete native
operations match independent original references. The installed application
remains at `d0910402c867a5d0faa42830e1b7d1fb55c70541`, with 871 unique semantic
owners. Productive integration, actual-service/caller proof, focused UBSAN and
installation remain for a consolidated milestone. The separate prepared
[contract-display component](native-contract-display.md) retains its two roots.
Integrating these seven roots alone would reach 878 owners; combining both
prepared blocks would reach 880. Neither number is an installed result. The
complete-game goal remains active.

## Complete scope and source data

| Entry | Exclusive end | Indexed instructions | Native operation |
| --- | --- | ---: | --- |
| `0x10009f20` | `0x1000a008` | 58 | `to_display_amount` |
| `0x1000a010` | `0x1000a05c` | 19 | `from_display_amount` |
| `0x1000a060` | `0x1000a164` | 65 | `increase_amount` |
| `0x1000a170` | `0x1000a26c` | 63 | `decrease_amount` |
| `0x1000a270` | `0x1000a32c` | 47 | `select_standard_amount` |
| `0x1000a330` | `0x1000a440` | 68 | `select_table_amount` |
| `0x1000c770` | `0x1000cbb4` | 273 | `amount_quantum` |

The index has 524 incoming root edges and no external interior entry. All 593
indexed instructions and eleven zero alignment words are pinned. The source banks
are 448 bytes of floating constants, 28 bytes of standard preset words and 28
bytes of diagnostic labels, with their three TOC pointers. The contiguous 24-byte
initial image of the three mutable scale factors is pinned separately. Those
initial factors are zero in the original image; runtime factors are inputs and
are not constrained to that initial value. Native operations load these guest
banks rather than embedding their original bytes.

The arithmetic helpers at `0x10773960`, `0x10773c80` and `0x10773de0`, and floating
suffix entries `0x10773808` and `0x10773858`, retain their existing owners. The
separate numeric text formatter `0x1000b410`, its label/configuration services
and the adjacent text parser are outside this seven-operation editing boundary.
They are not claimed complete by this component.

## Conversion and alignment

`to_display_amount` receives a pointer to a signed word in r3 and flags in r4.
It converts the word to double, multiplies by the mutable display scale at
`TOC-0x3aec`, and calls the existing double-to-signed-pair helper. Unless bit
`0x20` is set, it requests a quantum, constructs its signed half with the
original carry sequence, adds that half with wrapped 64-bit arithmetic, divides
by the quantum using the existing signed-quotient helper, and retains the low
64 bits of the product. It returns the pair in r3/r4. Ordinary nonaliased source
words remain unchanged. Negative and exceptional numeric inputs retain the
existing conversion and signed-division contracts; the expression is not
replaced by a host rounding function.

`from_display_amount` receives destination, high word and low word in r3/r4/r5.
It converts the pair to double, divides by the display scale, applies original
signed-word conversion toward zero and stores the word through the destination.
Its r3 return retains the preceding pair-to-double helper's residue; it does not
normalize that return to the destination or stored word.

The quantum selector retains only the signed low flag byte. Bit four selects
the large-step profile, taking priority over bit eight's finer profile. Their
inclusive thresholds and steps are multiplied by the mutable step scale at
`TOC-0x3afc`. With neither bit, the result is the literal pair `0:1000`, and the
unused constant-bank pointer is not dereferenced. Floating comparisons and
register reuse remain observable, including nonpositive scales and unordered
results. The original f28..f31 save/restore suffixes are retained.

## Increasing and decreasing

Both editors first produce the scaled pair and optionally align it by the same
rule. Increase asks for a quantum using **aligned value plus one**, then adds
that quantum. Decrease asks using the **aligned value itself**, then subtracts
it. Each converts back through the display scale, stores the signed word and
returns that stored word. Flags keep their original widths and order, and carries
and borrows propagate across the pair before any final word conversion.

With all three scales set to one and fine-profile flag eight, a stored 50,000
aligns to 50,000, increases to 55,000 and decreases to 49,500. The current value
selects step 500, while 50,001 selects step 5,000. The latter step comes from the
original cached f28 value, loaded from floating-bank offset `0x168` and forwarded
at `0x1000ca88`. These are controlled component inputs, not a named currency,
a natural career observation or a performance result.

## Preset selection and diagnostics

The standard selector accepts the signed low byte of r4 only for indices zero
through six, then calls the complete table selector with the original seven-word
bank. The table selector receives destination, index, table and length in
r3/r4/r5/r6. Index is a signed byte; length is a signed halfword. A null table,
negative index, or index outside that signed bound takes the diagnostic path.
High-byte residues therefore matter differently from a wide unsigned API:
256 selects index zero, while 255 becomes negative and fails validation.

Valid selection reads a signed table word, multiplies it by the preset scale at
`TOC-0x3af4`, converts through the existing pair helpers, divides by the display
scale, stores the original signed-word result and returns it. Invalid selection
calls four diagnostic services, clears the shared diagnostic byte, then reads
and returns the destination word. With distinct diagnostic storage the existing
value is preserved. Null destination accesses and output aliases with a table,
call frame, diagnostic flag or scratch text retain the original access order.

## Per-entry evidence

The component passes **22,508 comparisons**: 22,143 complete returns, 351 matching
stops and fourteen rejected deliberate corruptions.

| Root | Comparisons | Complete returns | Matching stops | Negatives | Own complete-return sites |
| --- | ---: | ---: | ---: | ---: | ---: |
| Convert to display | 2,835 | 2,809 | 24 | 2 | 58 |
| Convert from display | 261 | 247 | 12 | 2 | 19 |
| Increase | 2,837 | 2,809 | 26 | 2 | 65 |
| Decrease | 2,837 | 2,809 | 26 | 2 | 63 |
| Standard preset | 309 | 282 | 25 | 2 | 47 |
| Table preset | 419 | 380 | 37 | 2 | 68 |
| Quantum | 13,010 | 12,807 | 201 | 2 | 273 |

Every root has its own complete-return coverage, filtered by the entry whose
comparison began; the 593-site claim is not inferred only from a union of child
calls. There are no structural exceptions. Positive complete parent comparisons
also complete 32 modern table-selector children and 9,887 modern quantum
children. These are component connections, not original-world runtime proof.

Each comparison checks architectural registers, unique/temporary residues,
mapped bytes, ordered memory effects including instruction PCs, source order,
callee entry/return ABI and selected-body instruction accounting. The reference
retains all original instructions and disables every discovered productive hook.
The native bodies use named stages and structured conditions without original
body replay or instruction/PC dispatch.

Three accepted arithmetic bodies and two accepted floating suffix bodies run on
both sides of their dependency boundaries; their original proofs and production
adapter accounting are not redefined here. Four diagnostic services have
explicit external models. The corpus covers all 256 flag bytes and preset index
bytes, inclusive threshold frontiers, negative values, zero and nonunit scales,
wide signed-pair/word frontiers, signed lengths, aliases, final helper residues,
measured budget cuts, removed accessed bytes and interrupted callees.

Each entry rejects a wrong own result and a temporary extra memory effect
restored before return. Equal final images are required for the latter. Ten
money-value source/data checks and nine retained display checks pass. Both
`money_value_component_equivalence` and
`contract_display_component_equivalence` pass on ARM64 with C++23,
`-ffp-contract=off` and `-Wall -Wextra -Werror -Wpedantic`.

The shared guest-call class adds unsigned high-word multiplication, extended
addition and register subtraction with carry. Removing exactly those three new
methods reproduces its previous header byte for byte; existing members are
unchanged. The retained 4,791-comparison display component also passes with the
extended class.

The cumulative Portuguese guide, edition 35, explains the conversion, step
boundary and preset rules. Productive dispatch, real services/callers, focused
UBSAN and installation remain for a consolidated milestone. Save/Load is
**not retested**; performance is **not benchmarked**. The full reconstruction
goal remains active.
