# Native exception runtime reconstruction

The complete block is 21 routines / 1,657 indexed instructions: action/frame
traversal, saved-state copying, cleanup dispatch, exception-specification matching,
unexpected handling, exception-object destruction, context restoration, catch
installation, throw search, scalar/vector throw capture, object cleanup, type
metadata, termination/unexpected callbacks, and encoded type matching.

All 21 entries now execute named C++23 operations in default production dispatch.
The complete Release ARM64 build has 226 semantic entries, with the accepted first
205 unchanged. Component equivalence and production integration cover every
entry; the installed startup/menu/quit smoke is a separate application check.
Save/Load is **not retested**, and no performance improvement is measured.

## Original-code recovery

Read-only Ghidra exports recovered five previously unindexed intervals:

| Interval, end exclusive | Indexed instructions | Purpose |
| --- | ---: | --- |
| `0x107724c0..0x1077259c` | 55 | Unexpected-exception handler and landing pad |
| `0x10772698..0x107726f8` | 24 | Computed vector restore suffixes |
| `0x10772b80..0x10772c9c` | 71 | Vector snapshot suffixes and throw continuation |
| `0x10772d80..0x10772dc4` | 17 | Exception-object cleanup callback |
| `0x10772ee0..0x10772f08` | 10 | Unexpected-handler callback invocation |

The 177 additions produce 1,953,334 indexed instructions. Every prior record is
unchanged and present. All initialized segment payloads are byte-identical to
the accepted export; the original input directory remains unchanged. Discovery
does not alter the saved Ghidra program. New exports and their receipts stay
private under `.local/exception-runtime-reconstruction`.

The unexpected-handler landing pad needs its own discovery root: disassembling
the function entry alone follows an unconditional branch past the landing pad.
The callback invocation is then discovered through an original direct call.
Four non-indexed words inside cleanup dispatch are unreachable `nop` padding,
not additional missing bodies.

`tools/build_aot.sh` uses a separate `.local/recomp-input-exceptions` export when
the cached baseline predates these discovery roots. It preserves the old index.
The recovered full generation has 1,922 pages, zero missing static targets, and
zero pending targets. Those counters alone do not prove complete computed-flow
coverage.

## Complete reconstruction scope and pins

`tools/exception_runtime_audit.py` pins every complete body, instruction count,
and the four page fragments of cleanup dispatch and type matching. The actual
recovered index has no external static edge into any of the 21 body interiors.
Embedded vector suffix-selection helpers remain inside their owning context
bodies. Live destination tables and nonlocal transfers have focused runtime evidence,
including unfamiliar compiled destinations and explicit uncompiled-address faults.

| Entry | End exclusive | Instructions | Role |
| --- | --- | ---: | --- |
| `0x10771640` | `0x107717f0` | 108 | Advance action/frame traversal |
| `0x107717f0` | `0x10771b28` | 206 | Copy saved frame registers |
| `0x10771b30` | `0x10772430` | 572 | Execute cleanup actions |
| `0x10772430` | `0x107724bc` | 35 | Match an exception-specification list |
| `0x107724c0` | `0x1077259c` | 55 | Unexpected-exception handling |
| `0x107725a0` | `0x107725f4` | 21 | Derived exception-object destruction |
| `0x10772600` | `0x10772644` | 17 | Base exception-object destruction |
| `0x10772650` | `0x10772778` | 74 | Restore and transfer context |
| `0x10772780` | `0x10772808` | 34 | Install a catch record and transfer |
| `0x10772810` | `0x10772ac8` | 172 | Search and dispatch a throw |
| `0x10772ad0` | `0x10772b04` | 13 | Select scalar/vector capture |
| `0x10772b10` | `0x10772c9c` | 99 | Vector-capable throw capture |
| `0x10772ca0` | `0x10772d7c` | 55 | Scalar throw capture |
| `0x10772d80` | `0x10772dc4` | 17 | Invoke object cleanup callback |
| `0x10772dd0` | `0x10772dd8` | 2 | Type metadata access |
| `0x10772de0` | `0x10772de8` | 2 | Type metadata access |
| `0x10772e70` | `0x10772e90` | 8 | Abort facade |
| `0x10772e90` | `0x10772eb0` | 8 | Handler facade |
| `0x10772eb0` | `0x10772ed8` | 10 | Termination handler invocation |
| `0x10772ee0` | `0x10772f08` | 10 | Unexpected handler invocation |
| `0x10772f10` | `0x10773148` | 139 | Encoded type matching |

Existing fragment metadata, global destruction, C++ object/array storage,
allocator, streams, and lifecycle bodies retain their accepted manifest entries.
The prior 205-entry digest is
`9045711f1a6d4d64eb731a58039a9a69cd9e1e113a9e20628031df85a54b7cfc`.
The complete 226-entry compact sorted-JSON digest is
`cd2bf157adfb6e441959706bdfc12f8f4998aa07683b390e03c1ca31d1cfad0c`.

## Vector storage and language intrinsics

Guest vector values are sixteen big-endian bytes, with no host SIMD dependency.
The register array now includes Ghidra's complete `v0..v31` interval
`0x4200..0x43ff`, followed by a private architectural VSCR slot at `0x4400`.
Floating registers alias the first eight bytes of VSR0..31 at
`0x4000 + register_index * 16`, as verified in the actual language export.
The temporary array includes recovered vector address scratch through `0x44b03`.
All compilation units require the same expanded State layout.

Vector memory reads and writes check all sixteen mapped bytes before accessing
memory. Each completed operation has one ordered observer event of width 16;
a rejected operation changes no guest byte and emits no completed access.
Scalar/vector register aliases retain the same underlying bytes.

The generator resolves four language intrinsics by exported name: vector word
load/store selection and VSCR moves. Unknown intrinsics retain the explicit
unsupported stop. Unsupported vector arithmetic fails generation rather than
truncating a 128-bit value to 64 bits. Encoded `rA=0` supplies a zero address base,
including the `lvewx` constructor whose Ghidra input otherwise names live `r0`.

The [AltiVec programming environments manual](https://www.nxp.com/docs/en/reference-manual/ALTIVECPEM.pdf)
defines address-selected big-endian word lanes and leaves the other loaded lanes
boundedly undefined. The new reference lowering chooses zero for those lanes;
the C++ context helper uses the same explicitly defined choice. This does not
establish equality with every possible original hardware value in undefined
lanes. VSCR moves retain its NJ/SAT bits in the final vector word.

The retail context code stores the address-selected first word of `mfvscr` at
an aligned context offset `0x200`. Its restoration loads that first word before
`mtvscr`. This legacy layout is preserved, including its zero VSCR result under
the documented undefined-lane choice; reconstruction does not move the access
to the final word to repair the layout.

## C++ context operations

`exception_runtime::restore_context` restores the vector suffix selected by
the highest active bit in the low twelve VRSAVE bits, GPR13..31 and FPR14..31,
condition fields, TOC, VRSAVE, stack pointer, and the live guest backchain before
transferring to the supplied LR destination. Zero mask skips vector-register
loads; a nonzero mask restores the entire suffix from the first selected vector.

`exception_runtime::capture_vector_throw` captures the corresponding vector,
floating, and general registers into the aligned original frame. It preserves
the two VRSAVE stores, original r30/r31 corrections, condition fields, throw
arguments, source PCs, and ordered memory operations. This is the capture prefix
operation: it stops at `0x10772810`. The complete `throw_vector` entry now composes
that prefix with C++ search and its returning epilogue or successful nonlocal
transfer. The explicit prefix remains useful for isolated storage/fault checks.

`capture_scalar_throw` provides the corresponding prefix for the 55-instruction
scalar body at `0x10772ca0`. It saves the GPR/FPR suffixes, original r30/r31
corrections, CR and throw arguments into the aligned original frame. It preserves
the incoming vector/VSCR state and does not initialize the unused vector-context
slots. `throw_scalar` composes that prefix with C++ search and the original
returning tail. Both complete capture entries restore the caller through live
r30, its saved link and r30/r31 spill words if the selected destination is their
returning epilogue. Faults in that tail preserve the original source PC and
completed access prefix.

`throw_dispatch` reconstructs the complete 13-instruction selector at
`0x10772ad0`. It allocates the original 64-byte frame, reads the live byte at
`r2 - 0x352a` and invokes the scalar or vector C++ child with its original link
and TOC scratch. Any nonzero flag selects vector capture. A successful transfer
abandons this invocation; a returning child executes the selector's own
original stack/link epilogue. No capture entry calls its own generated body.

Default execution calls the C++ bodies through destination-label hooks.
Successful guest transfers unwind nested semantic callbacks without executing
abandoned guest epilogues, as described below. Diagnostic-original selection
remains independent for each entry and for semantic children.

## C++ action and frame operations

`exception_runtime::advance_action` implements the pinned 108-instruction
action/frame body at `0x10771640`. Its guest state contains the descriptor,
function base, action pointer, code/data bases and TOC at offsets `0..20`, followed
by the current frame at `24`, selected frame at `28`, and saved-register slot at
`32`. An absent action or its end-of-frame bit advances through the live
backchain and the existing metadata resolver. A descriptor with saved GPRs
refreshes the saved-register slot through the existing frame-word helper;
descriptor flag `0x10` chooses that slot instead of the current frame.
Frames without an applicable action continue the traversal.

Sequential actions read the destination table through the live TOC slot
`r2 - 0x3b88`. The C++ dispatch uses the actual guest code destination, allowing
a relocated/mutated table to redirect a kind to a different original case.
The cases advance by 4, 8, 10, 12, 14 or 20 bytes, or by
`12 + 4 * live_unsigned_halfword_count`. Both twelve-byte cases selected by
original kinds 6 and 7 reread the action pointer; the kind-5 case retains its
earlier pointer. A following kind-1 record redirects relative to the descriptor.
All pointer arithmetic retains the original low word32 behavior. The known
termination destinations call the existing termination routine. An unfamiliar
computed destination resumes generated dispatch until the owning entry's saved
caller boundary. It increments the unknown-transfer counter; an uncompiled
address also increments the escape counter and preserves its fault.

`exception_runtime::copy_saved_registers` implements the pinned 206-instruction
body at `0x107717f0`. It reads the caller through the context's live frame pointer,
copies the saved floating and general suffixes into their context slots, then
rereads the descriptor before the optional vector suffix and saved CR. The
floating count is `(descriptor_halfword >> 6) & 31`; the general count is the
first descriptor byte shifted by three. An odd floating count skips eight bytes
before the general saves. Optional vector saves use the third descriptor byte's
count and alignment bit, including its 64-byte gap and aligned-down source.
The final stores update the context's caller pointer and return its saved LR.

The scalar copies preserve eight-element batching and tail accesses. Vector
saves use four ordered word loads/stores per register and four-register batches,
including the original cursor updates around pending stores. These are not
atomic sixteen-byte accesses: overlap and a missing later word expose earlier
stores and scratch state. CR, XER carry, CTR, LR, GPR/FPR scratch, source PCs and
every architectural byte remain observable. Neither C++ operation executes its
own generated body in default mode. Both have production destination hooks.

## C++ cleanup dispatch

`exception_runtime::cleanup_actions` implements the complete pinned
572-instruction body at `0x10771b30`, including both original page fragments.
It walks guest cleanup records until the selected catch/specification boundary,
recovers caller registers when an action is absent, and resolves the next frame's
metadata through the already delivered resolver. Descriptor flag `0x10` selects
the recovered r31 slot; the ordinary path selects the updated caller pointer.
An action's end-of-frame bit clears the live action pointer after that action.

The operation reads the mutable cleanup destination table through
`r2 - 0x3b8c` and dispatches on the actual code destination. The 28 original
slots describe descriptor-relative redirects, inline objects, saved-GPR/frame
pointers, byte/halfword conditions, adjusted pointers, reverse array destruction,
plain callbacks, active exception objects, catch/specification stops, and
termination. Each applicable layout supports its original signed-halfword or
word32 offset fields. Word counts retain signed loop conditions, low-word
count/stride products, and cursor arithmetic; negative word counts skip calls
after computing the original end cursor.

Named C++ helpers retain the distinct callback arguments and scratch state.
Conditional records can read conditions and pointers from different locations;
plain callbacks retain the original r4 scratch. Array cursors descend while
the callback descriptor is reread for each element. Completed callbacks restore
TOC from the guest frame and reread the current action pointer before advancing.
An active exception object preserves its cleanup callback in context offset
`0x3b4`; another object invokes that callback immediately. Unknown computed
destinations use the same separately counted generated continuation as action
traversal, retaining the saved entry caller and avoiding another C++ epilogue.

The caller transition composes the C++ saved-register operation directly with
the existing metadata resolver. It does not execute the generated cleanup body
or its own generated frame-copy child in default mode. Central child invocation
retains per-entry selection, counters and entry/return/transfer observers.

## C++ encoded type and specification matching

`exception_runtime::match_types` implements the complete 139-instruction body
at `0x10772f10`, including its two page fragments. It clears the caller's offset
word before inspecting the expected type. A null expected pointer accepts any
type without reading the actual pointer; a concrete expected type retains the
original null/missing-byte faults. Overlapping output and encoding strings can
therefore change the subsequent match.

Ordinary encodings compare guest bytes and recursively process pointer/reference
prefixes. Const/volatile additions retain the original ordering and restrictions.
The original void-pointer shortcut accepts pointer and encoded-class-pointer
types, including its legacy qualifier behavior. Encoded class hierarchies use
the original `!` delimiters and search each base name. A match accumulates its
offset using unchecked signed-byte decimal arithmetic and low word32 results:
non-digit bytes, empty fields and overflowing values are preserved rather than
validated or normalized. The digit probe and accumulation reread remain distinct
ordered accesses. No host string or type-information API replaces these reads.

`exception_runtime::matches_specification` implements the complete 35-instruction
body at `0x10772430`. It rereads the live unsigned-halfword count at record offset
`2` and visits type pointers starting at offset `12`. Each child invokes the C++
matcher directly with the original stack output word, link state and TOC scratch.
Count or entry-pointer changes can change the next iteration. The original
saved-register prologue, frame restoration and early-return ABI remain visible.
Neither operation executes its own generated body in default mode. Production
hooks and the central child adapter also exercise independent original selection.

## C++ throw search and catch installation

`exception_runtime::search_throw` implements the pinned 172-instruction body at
`0x10772810`. It resolves the initial live metadata, selects the current frame or
saved-r31 frame, and builds a separate search cursor without consuming the
original cleanup cursor. A rethrow traverses actions to the active exception
record and recovers its object/type before restarting the handler search.

The live destination table at `r2 - 0x3b9c` selects action advancement, encoded
type matching, specification matching, or termination. Type/specification and
action/frame children compose the C++ operations directly. A matching catch runs
cleanup from the original cursor, publishes object/type/callback fields, and
retains the signed-halfword frame offset and unsigned-halfword landing offset.
Encoded pointer exceptions publish an extra indirect pointer slot with the base
adjustment. A rejected specification composes catch installation directly.
Returning termination callbacks preserve the original subsequent behavior.
Unfamiliar computed destinations resume generated dispatch with separate
unknown-transfer and uncompiled-address escape evidence.

`exception_runtime::install_catch` implements the pinned 34-instruction body at
`0x10772780`. It cleans up to the selected specification, publishes the original
exception fields and specification pointer, then uses live word32 block/landing
offsets and the metadata TOC to restore context. Both operations preserve their
ordered guest accesses, child link/TOC scratch, register spills and source PCs.
Neither operation executes its own generated body. If restoration deliberately
selects the operation's returning epilogue, that tail is also implemented in C++.

These component comparisons stop at the restored destination; they do not
execute the handler body. A separate composed harness now executes restored
handlers across nested host dispatch/callback scopes. Production acceptance adds
all 21 roots, mixed children, actual unexpected-callback transfer into the
landing continuation and native termination.

## C++ exception objects and handler callbacks

`destroy_derived_exception` and `destroy_base_exception` reconstruct the original
deleting destructors. They read each vtable through the live TOC and preserve the
derived-to-base store order, signed-halfword deletion flag, null-object path and
returned object address. Positive flags call the already delivered guest object
release operation. Its free-list stores can overwrite the freed object's payload;
the independent assertion checks the destructor stores before release rather
than requiring the final freed payload to retain a vtable. The derived destructor's
second branch consumes the still-live CR. Aliasing the object with the base-vtable
TOC slot therefore changes the following live load, as in the original.

`cleanup_exception_object` reads the object and then its cleanup descriptor from
the live three-word exception record. A null object or descriptor skips dispatch;
a null record retains its original mapping fault. The callback receives the
object and word32 `-1`, and a returning CFM callback restores the caller TOC.
The two type-metadata leaves read their own live, relocatable TOC slots.

`invoke_termination` and `invoke_unexpected` load the current CFM descriptors from
`r2 - 0x3520` and `r2 - 0x351c`. They preserve arguments, descriptor mutation,
link/frame state, TOC scratch and the original returning epilogues.
`termination_facade` composes the termination child directly. `abort_facade`
calls the delivered runtime-exit operation; the harness distinguishes an explicit
returning import fixture from an actual `ApplicationExit` signal. No C++ exception
object, host allocator or host type-information implementation replaces guest
objects or encodings.

## C++ unexpected-exception handling

`handle_unexpected` reconstructs the 55-instruction body at `0x107724c0`, including
its separate landing pad. Its normal entry saves GPR26..31, reads the live bad-
exception metadata, vtables and cleanup descriptor, allocates the original
112-byte frame, and invokes the unexpected callback. A returning callback proceeds
through termination and the original epilogue.

`resume_unexpected` implements the computed continuation at `0x107724f4` without
rerunning the entry prologue. The continuation first matches the caught type
against the original specification. An allowed type is rethrown through the C++
selector. Otherwise it tries the two original bad-exception encodings in order;
an accepted encoding publishes the base and derived vtables in its original
stack object and throws that object. If all three checks reject, it cleans the
original object, restores the saved backchain and stack, invokes termination,
and restores the original caller's link and GPR suffix.

The continuation's host scope retains the owning function identity `0x107724c0`
and marks the actual r31 frame. This avoids treating the interior landing PC as
a different function when a later throw abandons that invocation. Four component
paths independently check rethrow, each bad-exception object offset, and cleanup/
return. Their missing-byte comparisons cover every observed access's first and
last byte, including specification matching, object publication, nested capture/
search, cleanup, backchain changes and the returning tail. Production integration separately executes an actual
throwing unexpected callback into the landing hook and all four continuations.
That hook belongs to the existing unexpected entry, not a 22nd manifest entry.

## Host dispatch and nonlocal context transfers

`GuestDispatchScope` records active dispatchers and `GuestNativeScope` records
active semantic invocations, their function entries and allocated guest frames.
The links are thread-local, state-specific, process-local bookkeeping. They add
no guest-memory reads, guest state fields, serialized data or host allocations.
`MacAbi` owns the semantic guard and marks its frame after the original frame
store and r1 update have completed.

Search/catch installation records the function base already loaded from guest
metadata. Restoration records the unwound frame already loaded from context
offset `0x39c`. After its original return updates PC, restoration raises a
separate `GuestContextTransfer` signal. It is not a `std::exception`; ordinary
guest-fault catches do not consume it. The selected owning dispatcher resumes
from the restored PC. Abandoned semantic/import calls skip guest epilogues,
completed-import counters and return observers. Existing diagnostic-original
wrappers clear their pending selection while propagating the signal.

Function and frame identity distinguish recursive semantic invocations. A
generated child with a different known function can share its semantic parent's
frame and retain that parent. Other generated or diagnostic-original frames use
the unwound frame and dispatch-entry stack boundaries to find their owner. The
restored r1 alone is insufficient: the original can retain a deeper stack address
while replacing its backchain with the recovered caller's. A different State,
even with identical function/frame addresses, cannot consume the owner's signal.
Outside an active dispatcher, restoration keeps the explicit component boundary.

A third bounded reference contains the same 1,439 pinned original instructions
with four host-bookkeeping markers after the original function/frame loads and
return. The original guest operations and their ordered effects remain unchanged;
the markers integrate the statically compiled reference with host dispatch. Its
page/template symbols are independently named. The audit verifies marker count,
ordering after loads/PC update, exact bodies and symbol isolation.

The composed harness passes 20 original/C++ comparisons: parent/child handlers,
generated frames, shared frames, outer/inner recursive handlers and peer-state
isolation with vector restoration disabled/enabled; restoration and handler
faults; and an actual throwing array-constructor callback through direct
scalar/vector capture and both live selector branches. It compares complete
architecture, mapped guest bytes, ordered effects and import/callback prefixes,
then independently checks which host continuations survived. Two corrupted
transferred-stack controls are rejected. Every successful handler executes once;
interrupted child/throw continuations remain unexecuted. Reusing the mechanism
after success/failure leaves no stale dispatch scope.

The array fixture executes the existing original or C++ cookie-array constructor.
The first element completes, the second writes and throws, and exception cleanup
calls the actual partial-array cleanup entry. Only the completed first element
is destroyed; its completed count becomes zero, the failed element's writes
remain visible and the caller catch receives the exception object. The C++ path
records one constructor and one partial-cleanup call with zero storage escapes.
Attaching the storage observer also exercises the diagnostic-original wrapper.
Both modes record constructor entry and partial-cleanup entry/return, with no
constructor return boundary after the transfer. The original wrapper propagates
the signal and leaves no pending original-resume selection.
The throwing callback does not acquire a completed-import count. This is a
successful synthetic catch/unwind composition, not installed-app exception
coverage. The production integration harness supplies the complete per-entry acceptance.

## Production dispatch and acceptance

The generator requires the complete indexed subsystem and places each of the
21 hooks at its destination label before the first original operation. The
interior landing label `0x107724f4` has a separate continuation hook belonging
to `0x107724c0`; it never adds another manifest entry or repeats the prologue.
Six bounded references explicitly disable semantic hooks. Original-mode context
transfers retain the four host-bookkeeping markers after their guest effects.

The central adapter counts modern and diagnostic-original calls separately.
Returning, transferred and terminated modern boundaries have separate counters,
as do the landing continuation, unfamiliar destinations and uncompiled escapes.
Semantic children use that same adapter instead of bypassing selection or
observers. Reset touches only private diagnostics, preserving architectural
state, guest bytes, heap bookkeeping and neighboring subsystem diagnostics.
The landing retains its owning function and frame; its completion result avoids
using a throw's scratch LR as the original caller or adding a guest read.

The production integration harness passes 79 comparisons and 23 discriminating
root/landing/child negatives. Its 63 root comparisons cover all 21 entries in
normal direct, generated CFM-indirect and observed diagnostic-original modes.
Five mixed-selection cases separately select actual children. Every normal
root has nonzero modern direct/CFM witnesses and zero original calls, escapes
and unfamiliar transfers. The specification parent's corrupted matching child
is now rejected at a real adapter return boundary before the parent resumes.

Four additional cases cover three compiled unfamiliar returning destinations
and one uncompiled destination with its original fault. Six actual unexpected-
callback throws cover all four landing branches, scalar/vector capture and both
selector branches. The interrupted callback never resumes; the unexpected root
records a transfer, and the landing independently records return or transfer.
One actual native-exit case records termination. Independent reset and one-shot
original selection are verified after normal returns, faults and transfers.

Two integration fixture errors were corrected without changing the original
oracle or semantic behavior: the callback TOC needed the live exception/fragment
slots, and its metadata needed the original saved-r31 frame-selection flag.
Before each correction the original and C++ paths faulted identically.

## Validation

Six generator tests pass, including changed intrinsic IDs/names, malformed
operand shapes, zero-base addressing, full-width loads/stores/copies, and rejected
unsupported arithmetic. Eleven complete-scope audit tests pass, including changed,
missing, duplicate, pre-recovery, page-fragment and external-interior negatives;
removed, duplicated and moved hooks at all 21 roots and the landing; and all six
exact bounded original references with isolated symbols and transfer markers.
The original input identities and exact preservation of prior records have been
rechecked against fresh files.

The complete consistent Release library and its 1,922 generated units compiled
successfully from the recovered export. Native execution passed 90 storage and
original-context checks plus 52 C++ context comparisons, including 18 deliberate
mapping faults. The comparison covers every architectural/register and temporary
byte, every mapped guest byte, heap bookkeeping, PC, and ordered access prefixes.
It includes all twelve single-bit vector masks, empty/full/combined masks, upper
VRSAVE bits, disabled vector restoration, unaligned context and incoming stack,
and live backchain restoration.

The component reference now statically compiles 1,233 pinned original instructions:
173 context, 314 action/frame, 572 cleanup and 174 type/specification instructions.
Calls
to bodies outside that selection return explicitly to the test dispatcher;
frame helpers execute the existing production dependencies. Capture still stops
at `0x10772810`. The full production AOT page can follow that same-page call
internally past a `run_until` stop; stopping the component there must not be
confused with full throw/catch execution. No selected original instruction is
modified to force equality or a successful outcome.

Its dispatcher claims only the selected instruction addresses, allowing other
callbacks and dependencies on the same page to use the full dispatcher. Both
the page entry function and any Turbo-template implementations have private
reference names. The combined harness found and corrected a template-symbol
collision that had replaced the production callback page at link time. The audit
checks isolation and exact selected-address dispatch as well as pinned bodies.

A separately named search reference compiles 1,439 pinned original instructions,
adding all 206 search/catch instructions. Its context, page and Turbo-template
symbols remain distinct from both production and the 1,233-instruction capture-
prefix reference. This keeps existing capture-prefix comparisons at their
explicit search boundary while exercising the complete search/catch bodies.
Both references use exact selected-instruction dispatch. Unchanged output is
marked current after a generator dependency changes, avoiding repeated custom
commands for each consumer.

The complete capture reference contains 1,507 original instructions, adding all
68 scalar/selector instructions to the search/catch reference. It also records
the four host transfer markers, uses exact selected-instruction dispatch and
has independent page/template symbols. A separate 55-instruction scalar-prefix
reference stops at the explicit search call and has no host transfer markers.
The audit checks every original instruction, dispatcher address and all three
capture returning tails, as well as isolated symbols and the prefix boundary.

The full reference contains all 1,657 pinned original instructions, including the
unexpected landing pad and remaining object/callback routines. It retains all
four transfer markers, exact selected-address dispatch, and separate page/template
symbols. The audit confirms every root and landing label exactly once without
semantic hooks replacing an original body.

The frame harness passed 197 original/C++ comparisons, including 42 missing-
byte faults and exact ordered access prefixes. It checks every suffix count
`0..31` independently, varied initial CR/SO/carry state, batching/tails, odd
floating padding, enabled/disabled
vector and CR saves, vector alignment gaps, unaligned frames, overlapping source
and destination, and a descriptor changed between phases. Action checks cover
all normal original table destinations, descriptor-relative redirects, variable
counts through `65535`, word32 wrapping, relocated TOC/table contents, the
case-specific live pointer rereads, and one/two-frame metadata traversal.
Configured returning termination callbacks preserve their CFM argument/history
and restored TOC, including the original subsequent fault when metadata is absent.
Two deliberately corrupted root returns are rejected. Two deliberately
corrupted dependency returns are rejected at the original frame-word/metadata
boundaries, before the parent can publish another effect.

The eight component CTests and the directly related object-storage,
fragment-metadata and lifecycle checks pass in the current consistent Release
build (eleven tests, 39.74 seconds). The new production integration CTest passes
separately (2.09 seconds): twelve focused tests in total. No full test suite or
career campaign was run for this block.

Twenty Python suites pass 125 unique test functions. The optional full-export
local-temporary audit initially skipped its default input; it then passed against
the actual recovered index (9.139 seconds), covering all 1,953,334 instructions
and 6,437,694 P-code operations. The generator's 2,080 C++ differential local-
temporary fixtures also pass. Earlier module audits retain their exact original
manifest prefixes while accepting the new total of 226.

Fresh UBSAN covers 29 units: vector access, all exception operations, the central
adapter, host transfer ownership, runtime dispatch, existing object-storage
adapter/array bodies, nine harnesses and all six bounded original references.
All nine instrumented executables exit 0 with empty stderr and output identical
to Release (102.039 seconds). Source and header identities were verified again
before candidate preparation. Library internals outside these units remain
uninstrumented. Older partial sanitizer receipts are historical evidence.

The cleanup harness passes 479 original/C++ comparisons, including 306 missing-
byte faults, two root/dependency corruption negatives, and 241 CFM callback
boundary witnesses. Every normal original destination is exercised in both
applicable offset widths. Conditions cover frame/register sources and
empty/positive/signed byte or halfword values; arrays cover
empty/single/multiple/signed-negative counts and independent reverse-address
expectations. Additional cases cover negative offsets, live table/TOC changes,
action-pointer changes during callbacks, per-element callback replacement,
active-object deferral/null callbacks, returning termination callbacks,
selected versus skipped stop records, and initially absent/end-of-frame actions.
Caller transitions recover nonempty FPR/GPR/vector suffixes and CR, including a
newly recovered r31 selection. Faults cover exact architectural/memory/access
prefixes through these transitions. A corrupted metadata child is rejected before
the cleanup parent resumes.

The matching harness passes 203 original/C++ comparisons, including 124 missing-
byte faults and three own/child corruption negatives. It covers ordinary and
binary-byte encodings, nested pointer/reference qualifiers, the original void
shortcut, first/middle/last or absent class bases, unchecked offsets, catch-all
entries, empty/limited specification lists, overlap, unaligned output, live count
and entry changes, and the digit reread. Fault comparisons preserve every
architectural byte, mapped guest byte and ordered access prefix. Both root return
corruptions are rejected. A corrupted matching child is rejected at its final
offset-store effect before the specification parent resumes; this is a final-
effect boundary witness, not a production child-return observer.

The search/catch harness passes 655 original/C++ comparisons, including 620
missing-byte faults, three own/child corruption negatives and 60 CFM callback
boundary witnesses. Independent results cover direct specification installation,
signed offsets, landing wrap, both selected frames, enabled/disabled vector
restoration, class-base and indirect pointer adjustments, catch-all and skipped
handlers, accepted/rejected specifications, live destination changes, rethrows,
inline destruction, and nonempty caller FPR/GPR/vector/CR recovery. Both original
returning epilogues are exercised. A returning termination callback resumes
handler publication; missing initial metadata retains its subsequent mapping
fault. Both root transferred-state corruptions are rejected, and a corrupted
metadata child return is rejected before the search parent resumes.

CFM witness counts record descriptor-load boundary snapshots, including fault
prefixes; the aggregate does not assert that every observed callback returned.
Successful composed cases additionally assert the transferred state and the
independent destructor arguments/history.

The capture harness passes 143 original/C++ comparisons, including 130
missing-byte faults and six own/child corruption negatives. Scalar-prefix
checks cover aligned and two unaligned incoming frames, independent saved
arguments/FPR values, untouched vector/VSCR registers and untouched VRSAVE
context storage. The fault inventory removes the first and last bytes of every
observed scalar-prefix access. Complete checks cover direct scalar/vector
capture, both selector branches, each returning tail, installed catch fields,
a live capability flag changed before the loaded value is consumed, and
publication/save/epilogue faults. Corrupting each root's architectural output
is rejected. A corrupted search child is rejected at the first capture-
epilogue load, before the capture parent consumes its return word.

The object/callback/unexpected harness passes 1,273 original/C++ comparisons,
including 1,234 mapping faults, one actual native-exit signal and thirteen
root/landing/child corruption negatives. Each of the ten remaining roots has
its own negative and missing-byte access inventory. Independent expectations
cover signed deleting flags, actual guest release, object/TOC aliasing, live CR,
relocated metadata, null object/cleanup behavior, replacement CFM descriptors,
and all four unexpected continuation branches. The continuation fault inventory
adds 1,057 comparisons, preserving every architectural byte, mapped guest byte,
ordered access and callback/import prefix. A corrupted termination child is
rejected before its facade consumes the return word. A corrupted actual object-
release child is rejected before its deleting destructor restores the caller.

## Protected installation and installed smoke

The validated private full Release bundle was promoted to the installer's fixed
build-source path after preserving the previous source bundle. No historical
object files were reused to create the candidate. Four unchanged initialized
payloads were bundled, and strict ad-hoc signature verification passed.

Source, candidate, installed and running ARM64 executable SHA-256:
`8454b2568e865c76675c4fa73ca5601bdd4cb910366bc947ae0fdc18e4d592a5`.
The private linker executable, before bundle signing, is
`052b0624835cc4728938f7db7915dfe4e186c7cb906f673ae5ba049316a11557`.
The previous installed and build-source references remain preserved with
`6d9ed067922846327fac9fc294bac76feb15acb932cd8a403c4a0322a1913647`.
The installed bundle has normal production metadata, no validation environment
or import fixture, and the same four original payload identities.

One owned installed process, PID 52542, displayed Prepare Game, opened the
original Load Game chooser through its named command, canceled through named
Back or Cancel, and returned to Prepare Game. Original Quit opened its original
confirmation; Return confirmed it. The owned process exited and its log records
normal application exit after 21,593,344,557 statically recompiled instructions.
The accessibility query after Return reported the already-exited process; no
second launch was needed.

All 21 exception-root counters and the landing counter are zero in this smoke,
with zero original calls, escapes and unknown transfers. This confirms installed
startup/menu/quit compatibility, not installed execution of the new exception
routines. Their direct/CFM/original/mixed and nonlocal-transfer evidence comes
from the production harness described above.

Full content/metadata inventories preserve Game, existing user saves, the four
original payloads, 23 handoff files, the previous reference and 154 older bundles.
Runtime logs/preferences can change during UI execution; they are not claimed
byte-identical after the smoke. Private exports, generated sources, receipts,
saves and untracked handoff prompts remain outside the published change.

The complete 21-entry exception block is delivered; the full native port remains
in progress. Save/Load is **not retested**; no performance benchmark, career,
date-advancement, season or match campaign is claimed.
