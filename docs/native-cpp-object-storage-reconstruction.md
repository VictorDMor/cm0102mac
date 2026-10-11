# Native C++ object-array lifetime and storage reconstruction

This increment reconstructs twelve C++ object-array lifetime, allocation/release
facades and allocation-failure helpers as named semantic C++23 bodies on the
production ARM64 path. The complete engine/runtime modernization goal continues.

## Source and integration

The complete bodies contain 335 indexed original instructions on page `0x10773`.
Counts exclude padding. Full-index flows/fallthrough have zero external interior
edges. The audit pins original fields and bodies and destination hooks, including
both instruction-temporary storage modes and changed/missing/duplicate/misplaced
body/hook and external-interior-flow negatives.

| Entry | End exclusive | Instructions | Role | SHA-256 |
|---|---|---:|---|---|
| `0x10773150` | `0x10773258` | 64 | Construct cookie-backed object array | `a6afda68307478477bd8922640c4873c67f5788eacb600dafc664f56d50f9fe3` |
| `0x10773260` | `0x1077331c` | 46 | Clean partial array construction | `9a9ffcdf4dfb20cb017811be54708dbee238686951a7f960d86b1247cebf2952` |
| `0x10773320` | `0x10773420` | 62 | Construct object range | `91e81b26c6de2bb5f8e6243cb1f973e3ebaa6472f61887fa6ea9747430fb5da6` |
| `0x10773420` | `0x1077349c` | 30 | Destroy object range | `34c36507e32d34a513b1f61a34dcaefe79463b18acf2829530e084547eec6898` |
| `0x107734a0` | `0x10773520` | 31 | Destroy cookie-backed object array | `54ca31aad92b91b5353c2332088aa902f83729fafefab468dbbf61ffbd9f6022` |
| `0x10773520` | `0x1077359c` | 30 | Allocate C++ object storage | `fc0742057d9526d3608997abb956343feb8e37d062f86492bd5e3c0b72cce7a1` |
| `0x107735a0` | `0x107735e4` | 14 | Release C++ object storage | `257718e3cdbaa24cc942a38e938ee0e2f82d1ba82e5114e9caf54865bd06397e` |
| `0x107735f0` | `0x10773610` | 8 | Allocate C++ array storage | `b791ed62d9dec139ca9ad9bf42d03582af82d555681d44cd8254e760bf4f2ea1` |
| `0x10773610` | `0x10773630` | 8 | Release C++ array storage | `184a701e68a139ab7c1f31cf289d8cc55a11c54c659e28869429c9c77bc20025` |
| `0x10773630` | `0x1077367c` | 19 | Raise allocation failure when enabled | `556e2a1e82811199adc13acd050fec0f4112fc9daa79edc05f95f90cf52667da` |
| `0x10773680` | `0x107736d4` | 21 | Destroy allocation-failure object | `53b70fe40e832cc6e2038ccf69d8e9e25e75f3342e681422fff10f536c304bc2` |
| `0x107736e0` | `0x107736e8` | 2 | Read allocation-failure type metadata | `9c50a478e66513910adc7af8ebdbcd62612840d9d710274434a71fe8f280b7bc` |

The accepted 193-entry prefix is unchanged, with digest
`33bf1c60bf7773bc46dd082587c78f6a7566c5f4b9139eadf3b058c52bdbcc50`.
The 205-entry manifest digest is
`9045711f1a6d4d64eb731a58039a9a69cd9e1e113a9e20628031df85a54b7cfc`.
Generation retains 1,953,157 instructions, 1,922 pages and zero missing/pending
targets; it passed in 62.11 seconds. The full consistent Release rebuild passed
in 677.37 seconds after State gained private diagnostics. Every generated unit
uses the same State layout. The adapter owns independent
global/per-entry original selection, one-use original resume, counters,
return/termination boundaries, observers and reset. These remain process-local
and unpersisted. Production attaches no characterization fixture.

## Preserved contracts

The cookie-backed constructor accepts raw storage, constructor/destructor CFM
vectors, word32 stride and count. Nonnull storage receives stride/count in its
first eight bytes and returns raw+16; the other eight cookie bytes and payload
are not cleared. A null constructor still publishes that cookie. Constructor
callbacks receive ascending element addresses and flag 1. Raw-range construction
uses the supplied first element directly and has no null-constructor guard.
Both routines preserve their separate 112/96-byte frames, nonvolatile saves,
ordered guard publication and reloaded completion counters. Their post-loop
guard can trigger reverse cleanup of only completed elements; the stack guard
and destructor pointer remain live guest fields.

Partial-array cleanup reads first-element/stride/total/destructor/completed fields
from a five-word guest guard. It skips callbacks when completion is at least the
total or the destructor is null. Otherwise it walks completed elements backwards,
reloading stride, callback and completed count on each iteration. It decrements
the live count only after callback return. The deleting flag is narrowed to signed
16 bits, and positive values release the guard through the C++ delete helper.
Null guards are no-ops with the original return and frame effects.

Range destruction computes the one-past address with a low-word stride*count
product and calls backwards with flag -1. Cookie-array destruction reads stride
and count at payload-16/-12, optionally calls backwards, and then releases raw
storage through the array-delete facade. A null destructor skips cookie reads,
but a nonnull payload is still released. No host array/container, checked
multiplication, corrected wrap, extra validation or double-free policy replaces
these original guest contracts.

C++ object allocation maps a zero request to four bytes and delegates to the
existing semantic global allocator. It captures the TOC-selected new-handler
cell once, then reloads that cell's callback after every failed attempt. An
available handler runs through the original CFM thunk and allocation retries;
an absent handler calls the allocation-failure helper. When that helper returns
with exceptions disabled, the facade preserves the original null return.
Array allocation adds its own 64-byte ABI frame around object allocation.
Object release has the original null guard and 96-byte frame; its epilogue
reloads r1 from the live guest back-chain instead of adding the frame size.
Array release retains its independent wrapper frame. Guest addresses, pool/
arena ownership, allocated/available maps, native NewPtr/DisposePtr services and
all ordered allocator effects remain the current dependencies.

The failure helper reads its TOC-selected enable byte with signed-byte record
effects. A nonzero value publishes both original vtable values in sequence to
the same stack word, loads the live type/destructor arguments and calls the
existing generated exception entry `0x10772ad0`. The failure-object destructor
publishes its two TOC-selected values in reverse order, then conditionally
releases storage using the signed-short deleting flag. The type helper returns
the live TOC-selected metadata pointer. Successful nonreturning ApplicationExit
is recorded at the owning adapter boundary and rethrown, preserving the
existing native application exit behavior.

The exception matcher `0x10772f10`, throw/unwind machinery, nonlocal contexts,
startup/root, heap glues and native heap bridges remain outside this increment.
An enabled failure with an empty fragment registry follows original termination
to native exit; this is failure/termination evidence, not proof of a successful
catch/unwind path. Date's ascending-year leap behavior and the allocator's
productive decreasing-retry guard remain unchanged.

## Validation and delivery

The preliminary unchanged-State component build passed 70 complete-state/effect
comparisons, including one real native termination, against the accepted
193-entry library. Its root bodies were semantic, while calls into the other
new object-storage entries still used generated bodies; previously accepted
modules remained semantic. This is component evidence, not production-hook
coverage.

The production matrix adds direct/CFM calls, independent own and child negatives,
mixed parent/child selection, diagnostic reset, live new-handler and array-guard
mutations, signed deleting flags, TOC relocation, aliases, sparse mappings and
ordered fault prefixes. Full architecture, PC, every mapped guest byte,
allocated/available maps, heap bounds/error, native-call counts, ordered accesses
and native-import register/heap/mapping history are compared. Complete register
and heap state is also captured at CFM boundaries. Only instruction accounting
and process-local diagnostics are excluded. An actual native allocation,
cookie construction, reverse destruction, reuse and release sequence checks
payload/padding preservation, callback addresses/flags and allocator ownership.

The production harness passed **171 comparisons**, including **36 expected
mapping faults**, **two real native terminations** and **22 own/child negatives**,
in 3.57 seconds with empty stderr. All twelve entries have direct and generated
CFM witnesses, observer-free original selection and independent reset evidence.
Own mutations include every entry's return, the enabled failure's terminal
boundary and constructor/cleanup/allocation/release children. They are rejected
at the child's owning boundary before the parent can return or terminate.
Mixed selection remains independent for allocation, release and array release.

| Entry | Normal semantic calls in the production harness |
|---|---:|
| `0x10773150` | 18 |
| `0x10773260` | 18 |
| `0x10773320` | 15 |
| `0x10773420` | 13 |
| `0x107734a0` | 9 |
| `0x10773520` | 26 |
| `0x107735a0` | 27 |
| `0x107735f0` | 13 |
| `0x10773610` | 15 |
| `0x10773630` | 12 |
| `0x10773680` | 12 |
| `0x107736e0` | 4 |

These counts exclude faults, and include the successful nonreturning boundaries.
Default cases require zero original calls and escapes. Callback-driven mutations
exercise both reverse-construction cleanup guards, live partial-array strides,
the captured handler cell and callback rereads on retries. The native composition
checks ascending constructors, descending destructors, preserved cookie padding/
payload, arena release, reuse of the same guest address and final ownership maps.
Faults cover frame saves, cookie/guard publication, CFM vectors, live fields and
the enabled failure's double-store/type/destructor prefix. Budget exhaustion is
not counted as a normal or expected mapping-fault success.

Seventeen Python audit suites passed **102 tests** in 43.00 seconds. Four related
CTests (`modern_allocator_equivalence`, `modern_runtime_lifecycle_equivalence`,
`modern_code_fragment_equivalence`, `native_bridges`) passed in 12.73 seconds.
The new harness was run directly; its registered CTest was not repeated. UBSAN
instrumented the three new implementation units, modified `aot_runtime.cpp`
and the harness, linked against the final consistent Release libraries, and
passed in 5.30 seconds. Its output matches Release exactly and stderr is empty;
unchanged library internals are not claimed to be instrumented. Compiled unit
and header hashes are checked before sanitizer linkage.

One protected final installation delivered
`/Users/victordmor/Applications/CM 01-02 Native Prototype.app`. Signed source,
candidate, installed and running ARM64 executable identity is
`6d9ed067922846327fac9fc294bac76feb15acb932cd8a403c4a0322a1913647`.
The retained unsigned linker output is
`7cb580ae6a474cc1640f5f424b75584f587e74f4f01eebd51d73e7614dcfab75`;
the previous installed reference is
`cf03e10179523d539f03ff222af074560c82eee58f1cfd84cd18b14acdb832b3`.
Strict signatures, normal production plist, absence of LSEnvironment and all four
original payload identities passed. Fresh checks found no live user game before
installation or smoke. The running process's open executable path and ARM64 hash
match the installed candidate.

Prepare Game rendered, named `Quit CM 01/02` dispatched through the original
action, and the original confirmation appeared. Owned process 63146 closed
normally after Return. The new private log `app.nLf9T0/startup.log` records
4,977,661,890 statically recompiled instructions. Installed
per-entry evidence follows; every entry remained modern with zero original
calls and zero escapes.

| Entry | Installed semantic calls | Returns | Native terminations |
|---|---:|---:|---:|
| `0x10773150` | 2 | 2 | 0 |
| `0x10773260` | 0 | 0 | 0 |
| `0x10773320` | 10 | 10 | 0 |
| `0x10773420` | 8 | 8 | 0 |
| `0x107734a0` | 2 | 2 | 0 |
| `0x10773520` | 39857 | 39857 | 0 |
| `0x107735a0` | 39851 | 39851 | 0 |
| `0x107735f0` | 39537 | 39537 | 0 |
| `0x10773610` | 39537 | 39537 | 0 |
| `0x10773630` | 0 | 0 | 0 |
| `0x10773680` | 0 | 0 | 0 |
| `0x107736e0` | 0 | 0 | 0 |

The UI exercised eight entries; the other four are covered by the production
harness. There was no fatal AOT error. The log contains the nonfatal catalog
-120/-43 diagnostics; no clean-catalog claim is made. Instruction accounting or
startup call counts are not a performance benchmark.

Immediately after installation, before smoke, full content/forks, xattrs, modes,
timestamps and flags were unchanged for Game (809 inventory entries), Runtime
(5), Runs (70), save snapshots, 23 handoffs, the installed reference, original
payloads and 151 older private bundles. After smoke, Game, existing saves,
snapshots, payloads, handoffs, reference and retained bundles remain protected;
run logs and preferences may legitimately change. Receipts and reference/
candidate/linker bundles remain private under
`.local/cpp-object-storage-reconstruction`. No original/generated payload, save,
run log or bundle is published.
Save/Load: **not retested**. Performance: **not benchmarked**. No career, date,
season or match campaign was added for this bounded runtime increment.
