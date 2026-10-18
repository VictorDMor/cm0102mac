# Contract storage creation

The complete storage creation scope contains three original operations and
2,016 instructions. Row reservation and primary binding are prepared and
directly validated. The full 1,900-instruction staff-contract initializer is
characterized and remains required implementation work. This report does not
claim the complete storage creation block is reconstructed.

The accepted manifest and installed game remain at **756 entries**. These
prepared functions compile only into their component check; production hooks,
actual-world runtime, sanitizers and installation remain a combined contract
integration milestone with the [prepared queries](native-contract-queries.md).

## Original scope

| Entry | Exclusive end | Instructions | Operation | State |
| --- | --- | ---: | --- | --- |
| `0x10078e70` | `0x10078ef8` | 34 | Bind a primary staff contract | Prepared and compared |
| `0x10078f00` | `0x10079048` | 82 | Reserve or reuse a contract row | Prepared and compared |
| `0x103727c0` | `0x10374570` | 1,900 | Initialize a staff contract | Characterized; implementation pending |

All three complete bodies have pinned fingerprints and page fragments, no
accepted semantic owner and no indexed external entrance into an interior.
The index records nine incoming root edges. The row reservation diagnostic
identifies `CONTRACT_MANAGER.CPP` and
`CONTRACT_MANAGER::get_free_staff_contract_space()`.

## Prepared behavior

A zero signed mode byte scans existing 100-byte rows for a staff ID of -1 and
returns the first such index without incrementing the used count. A nonzero
byte appends directly. Values such as 256 therefore retain the zero-byte path.
The scan rereads the live used count on each iteration.

Appending grows capacity by 200 only when used count equals capacity. It
stores the new capacity before requesting `capacity * 100` bytes, using the
original 32-bit wraparound. A populated controller reallocates; an empty one
allocates. A null result follows the original diagnostic sequence and returns
-1. The changed capacity and stored null pointer remain visible. Success
increments the used count and rereads it before subtracting one for the result.
No new capacity checks or allocation recovery rules are introduced.

Primary binding calls reservation with mode 1. A nonnegative signed result is
written into the staff statistics' primary index. The binder rereads the staff
ID, writes it into the selected contract row, reloads the controller's storage
pointer and invokes the initializer with the original staff/data arguments.
Negative reservation results skip these mutations. The component matrix runs
complete original/C++ reservation bodies within the binder.

Tests preserve allocation relocation/failure, capacity and index wraparound,
live count and staff-ID changes, relocated TOC and aliasing between statistics,
the controller and destination record. The alias cases retain original effects
before a pointer fault. The initializer is currently a stateful component
dependency with register and field effects; its full body is still pending.

## Evidence and remaining work

The focused Release CTest passes in 1.31 seconds with **509 comparisons**:
237 complete comparisons, 268 interrupted or faulted comparisons, and four
discriminating negatives. It covers all **116 prepared source sites**.
Complete architectural/temporary/mapped-memory state, ordered effects,
dependency input/output ABI, source order and instruction accounting agree.
Only normal completion, guest budget exhaustion, explicit dependency
interruption and unmapped guest-memory faults are valid outcomes. Fixture
assertions cannot count as matching faults.

Nine Python guards pin the complete 2,016-instruction original scope, all page
fragments, three original tables totaling 780 bytes, their pointers and two
external padding words. They reject source/data mutation, missing/duplicate/
reordered bodies, partial pages, semantic ownership overlap and external
interior edges, including reversed contract order. Independent references
retain every original label and step with semantic hooks disabled. Generating
the complete reference does not establish native coverage of the initializer.

The initializer's 1,900 instructions are mapped into eleven complete logical
phases, with 45 direct field-write sites. Its two floating-point tables retain
26 doubles and five singles. Five 72-instruction sampling bodies have identical
assembly after register, stack scratch and branch-address normalization; the
sixth has a distinct scratch-register order. Country selector variables remain
live inputs. This characterization supports the required next implementation;
it is not equivalence or runtime acceptance of those rules.

Original data, references, mappings, fixture-correction history and execution
receipts remain private under `.local/contract-storage-reconstruction`.
Save/Load: **not retested**. Performance: **not benchmarked**. The full
reconstruction goal remains active.
