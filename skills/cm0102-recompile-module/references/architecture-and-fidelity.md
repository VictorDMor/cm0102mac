# Architecture and fidelity

Use these decisions when recovering and implementing a module. The owning
contracts remain the current project instructions and source, especially
`native/guest_abi.hpp`, `native/guest_values.hpp`, `native/guest_fpu.hpp`,
`native/floating_point.hpp` and the module's callers.

## A small compatibility boundary around ordinary C++23

Follow the established shape where useful:

| Responsibility | Typical location | Contract |
| --- | --- | --- |
| Typed routine signatures | `native/<module>.hpp` | Map every original entry to a complete named implementation. |
| Shared record views and constants | `<module>_detail.hpp` or existing domain headers | Give offsets, sizes, flags and sentinel values domain names; avoid competing layouts. |
| Functional blocks | `<module>_<area>.cpp` | Express algorithms with typed locals and normal control flow. |
| ABI and checker integration | `<module>_adapter.*` | Marshal arguments, results and required guest effects; preserve original bypass for differential checks. |
| Entry ownership | `<module>_routines*.inc` and module audit | Pin original entry contracts and distinguish drafts from production entries. |

This is a fit-to-project example, not a mandatory file count. Reuse existing
files and views when they already serve the responsibility. Keep functions
focused, ownership explicit and invariants local; introduce abstractions only
when they remove real repetition or clarify a domain contract. C++23 is the
language baseline, not a requirement to use every new language feature.

Domain variables should describe clubs, players, records, counts and decisions.
Do not disguise `r3` as a named variable while keeping the algorithm in the guest
register bank. Keep required register writes next to the calls or boundaries
that expose them, using the project's helpers. A typed routine may still take
`guest::Cpu&` and use compatibility bookkeeping where observable behavior
requires it. Record why unavoidable low-level details remain.

## Guest memory and ownership

- Treat guest addresses as guest addresses, with the original 32-bit width.
  Use checked, big-endian guest accessors/views; never reinterpret guest records
  as native structs with host padding, pointers or byte order.
- Preserve record sizes, alignment, sign extension, signed comparisons,
  sentinels, TOC references, aliasing and overlapping-copy behavior. Define
  wrapping arithmetic explicitly rather than relying on C++ signed overflow.
  Check shifts, division, remainder, narrowing and exceptional arithmetic
  against the actual original operations.
- Preserve writes visible to guest memory observers and the game's untouched
  bytes. Do not zero padding or initialize an original's uninitialized field for
  tidiness. Do not suppress observers with bulk host writes without proving the
  existing memory contract permits it.
- Keep allocation sizes, failure behavior, ownership, allocation/free order and
  object identity when observable. Use RAII for host resources where its cleanup
  timing is compatible; do not silently move guest ownership or guest frees into
  a destructor. Do not serialize native layouts or replace guest storage with
  host collections that change addresses, iteration order or persistence.

## ABI, stack and calls

Characterize register arguments and return values, preserved registers,
condition fields, TOC, LR/return addresses, frames, callbacks, varargs and
nonlocal control flow. Use existing `guest::Frame`, `enter`, `link`, `Kept` and
related helpers according to their current implementation.

Guest stack bytes and registers matter when the game later reads them.
Original code can read leftover stack and spill apparently unused registers
through varargs, `mfcr`, callbacks or exception context saves. Reproduce those
effects, including red-zone stores and callee stack residue, or prove they cannot
be observed along the affected paths. Since 2026-11-06 stack bytes follow the
per-reader contract in `CLAUDE.md`: only bytes that a reader listed by
`--stack-audit` reads must be reproduced (see `docs/cpp-rewrites.md`);
`--scramble-stack` shows whether a leftover matters. Do not ignore
a mismatch simply because a register is ABI-volatile. Conversely, current
`CLAUDE.md` makes instruction counts and unobserved scratch diagnostic; do not
rebuild instruction-by-instruction execution solely to match them. A bounded
callee scan alone is insufficient when a relevant reader lies beyond its bound.

Inspect registers at call boundaries as well as at return. A fixture that answers
a callee without consuming its arguments can hide a wrong call; use argument
traces and their negative controls. `tools/call_registers.py ENTRY` helps locate
definitions, but per-path values and indirect calls still need reference checks.

Call rewritten routines directly, with the correct guest call boundary where
needed. External generated callees and platform imports may remain, through
named existing interfaces. A call to a generated routine *owned by this module*
is a remaining reconstruction gap. Keep an explicit dependency list so an
external dependency does not accidentally become hidden module fallback.

Check callback transition vectors, comparator contracts, indirect calls,
interior entry points and `setjmp`/`longjmp` or transfer resumes. Production
resumption must not quietly enter an owned generated body. Do not impose host
exception handling or destructor behavior that changes the original transfer.

## Numerical and simulation fidelity

- Preserve RNG algorithm, state, number of draws and their order relative to
  branches, iteration, allocations and calls. Do not batch, parallelize or
  reorder simulation work without an equivalence argument and required checks.
- Preserve original precision, rounding and observable FP status. Reuse the
  instruction-appropriate existing helpers for single/double arithmetic and
  fused multiply-add with one rounding. Ordinary host expressions can contract,
  reassociate or round differently; inspect rather than assume equivalence.
- Keep `-ffast-math` and equivalent relaxed behavior off. Do not remove FPSCR
  bookkeeping merely because direct branch decisions do not use it; context
  saves can expose it to game memory.
- Preserve sorting/comparator quirks, stable or unstable tie behavior, traversal
  order, side effects and original short-circuit behavior. Avoid host algorithms
  whose call order or preconditions differ from the original's contract.

## Persistence, errors and presentation

Keep exact file formats, byte order, record counts, legacy versions, partial IO,
retry paths, allocation failures, assertions and original error reporting.
Do not add a new guest-facing validation rule, clamp, default value or recovery
path as a modernization shortcut. Reproduce reachable original quirks with a
short evidence-based comment; propose behavior changes separately.

Keep screen contents, strings/encoding, localization, selection, event handling
and user-visible results equivalent when the module controls them. Architecture
improvements are internal; they must not alter the simulation or UI contract.
