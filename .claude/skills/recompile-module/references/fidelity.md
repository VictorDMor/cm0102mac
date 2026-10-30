# Fidelity decisions

`docs/cpp-rewrites.md` holds the mechanics (frames, registers at calls, checking).
This file covers the choices a rewrite faces that are easy to get subtly wrong.
Helpers live in `native/guest_abi.hpp`, `guest_values.hpp`, `guest_fpu.hpp` and
`floating_point.hpp`; read their current code rather than trusting a summary.

## Shape

| Responsibility | Typical location |
| --- | --- |
| Typed routine signatures | `native/<module>.hpp` |
| Record views and constants | `<module>_detail.hpp` or an existing domain header |
| Algorithms | `<module>.cpp`, `<module>_<area>.cpp` |
| ABI and original bypass | `<module>_adapter.*` |
| Entry ownership and digests | `<module>_routines.inc` |

Fit this to the module rather than treating it as a file count. Variables should
talk about clubs, players and decisions: renaming `r3` while the algorithm still
lives in the register bank is not a reconstruction. Keep required register writes
next to the calls that expose them. A routine may still take `guest::Cpu&`.

No framework, host-owned shadow world, new dependency or unrelated redesign.

## Guest memory

- Use the checked big-endian accessors and named views. Never reinterpret guest
  records as native structs, since host padding and byte order would differ.
- Preserve record sizes, sign extension, signed comparisons, sentinels, aliasing
  and overlapping-copy behavior. Write wrapping arithmetic explicitly, because
  C++ signed overflow is undefined. Check shifts, division and narrowing against
  the actual instructions.
- Don't zero padding or initialize a field the original leaves uninitialized,
  however untidy it looks. The original's results depend on such bytes.
- Keep allocation sizes, failure behavior, order and object identity when
  observable. RAII is fine for host resources, but don't move a guest free into
  a destructor, and don't replace guest storage with host collections that change
  addresses, iteration order or persistence.

## Stack, registers and calls

Keep the stack image exact (Victor's decision, 2026-10-23): the original reads
bytes it never wrote, and those reads can change RNG and records. Every byte the
original leaves below the stack pointer stays, including what its callees leave.
Giving that up needs proof that nothing reads it, from the audit plus replays:
`cm-world-replay --stack-audit` lists uninitialized reads, and `--scramble-stack`
shows whether they matter. A bounded callee scan is not enough when a reader lies
beyond the bound. `CLAUDE.md` calls leftover stack scratch a diagnostic, in a line
from 2026-10-20 that predates this finding. Read it as covering only bytes shown
to be unread: until that is shown, they are memory the game reads.

Instruction counts and r0, r11, r12, XER and CTR are diagnostics (the last group
only with `--strict-volatile`), so don't rebuild instruction-by-instruction
execution to match them. The registers that do reach memory later are listed in
`docs/cpp-rewrites.md`, because varargs calls and `mfcr` spill "unused" ones.
`tools/call_registers.py ENTRY` shows what each register holds at every call.

Check transition vectors, comparator contracts, indirect calls, interior entries
and `setjmp`/`longjmp` resumes: production resumption must not quietly enter an
owned generated body. Host exceptions and destructors must not alter the original
transfer of control.

## Numbers and order

- RNG: same algorithm, state, number of draws and order relative to branches,
  iteration, allocations and calls. No batching or parallelizing without an
  equivalence argument.
- Floating point: same precision, rounding and observable FPSCR. Use the builder's
  and `floating_point.hpp`'s helpers, multiply-add with one rounding. Plain host
  expressions can contract or round differently. Don't drop FPSCR bookkeeping just
  because no branch reads it: context saves can expose it to game memory.
- Sorting: same comparator call order and tie behavior. A host algorithm with
  different call order or preconditions is not equivalent.

## Persistence, errors, presentation

Keep exact file formats, byte order, record counts, legacy versions, partial IO,
retry paths, allocation failures, assertions and error reporting. Don't add a
validation rule, clamp, default or recovery path as a modernization: reproduce a
reachable quirk, with a short comment citing the evidence, and propose any change
separately. Screen contents, strings, selection and events stay equivalent when
the module controls them.
