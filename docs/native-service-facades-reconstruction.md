# Framed Mac native-service facades

## Scope and source contracts

This block reconstructs the complete framed facades for `AEProcessAppleEvent`,
`NewPtr` and `DisposePtr` as named C++23 service calls. Each body contains nine
indexed instructions. The complete program index has exactly these three bodies
with this call/frame shape; all three have zero exterior edges into their
interiors. Existing native event, allocation and release services retain their
implementation and policy. The allocator's arena and pool metadata remain guest
memory, including its previously delivered decreasing-retry guard.

| Entry | End exclusive | Import glue | Original-body SHA-256 |
|---|---|---|---|
| `0x10770a80` | `0x10770aa4` | `0x10780528` (`AEProcessAppleEvent`) | `a475f3fc2cf643639103d80c2e64d2a080009d768ee2c1395ee40b6c24364f53` |
| `0x10778d50` | `0x10778d74` | `0x1077fc70` (`NewPtr`) | `0435d27f27f18b8ae917b17f81b1ac625d2653baad43357e426c55f5475f1cf9` |
| `0x10778d80` | `0x10778da4` | `0x1077fc58` (`DisposePtr`) | `8e1bf4a84ed975fa5aec82dcde0d089b30b76dd59451a01f925c1fb1493e02b5` |

Fingerprints serialize the indexed `address`, `assembly`, `ops`, `flows` and
`fallthrough` fields in ascending address order with sorted JSON keys and compact
separators. Missing, reordered, duplicated or changed records fail the audit.
Selected roots or interior fragments cannot claim a complete absent body.
Destination hooks precede the first original operation on both generated pages.

Complete generation appends these three sorted entries after the accepted 536
records, giving **539** semantic replacements. The first-536 digest remains
`c4f5be3bde9d551349be581de51abab54a2c88d8f7a4ed6375f75b490aa2e2c4`;
the new complete digest is
`dd0b654f24290a715e0fbe252d807513955ad3827cf5eed0b7baad0d21291fbd`.
Generation retains 1,953,334 instructions, 1,922 pages, the five accepted source
fixups and zero missing/pending targets. Original references contain exactly the
27 facade instructions, emitted privately without any semantic hook.

## Caller and service behavior

`ServiceCall` preserves the caller, dispatches the live import glue and resumes
the caller. The prologue captures LR into r0, saves it at live SP+8, and stores
the previous SP in a 64-byte frame. The frame address is captured before the
store observer; the published SP uses that captured address. The call publishes
the caller TOC in the existing generated residue at register offset 2440,
publishes the facade continuation as LR, and enters the existing native
dispatcher. Argument registers and the import's effects remain observable.

The epilogue reads caller TOC at live SP+20 and saved link at live SP+72, adds 64
to live SP, restores LR from live r0 and returns through live LR. None of those
values is replaced by a cached entry value. Stack arithmetic remains word32;
unaligned mappings, aliases and observer mutations preserve original access
order, PCs and register residues. GPR/FPR, CR/XER/CTR/LR/PC/TOC/FPSCR, unique
storage and heap/import effects are compared without normalization.

The new core accounts for each original facade instruction individually. Finite
and dynamically shortened budgets preserve exact access prefixes and accounting
without executing the facade's original body. Diagnostic original selection is
global or per entry; profiling uses a separately counted original selection.
Original body guards are scoped and independent of import-descriptor guards.
Callbacks use the existing dispatcher and scalar-context ownership machinery.
A child fault or nonlocal transfer unwinds the semantic frame without running
its remaining guest epilogue.

Diagnostics are process-local, unsaved and independently reset. The default
production path uses the semantic bodies. `CM_AOT_NATIVE_SERVICE_ORIGINAL` is a
private diagnostic option, and normal exit reports separate per-entry calls,
returns, faults, budget stops, transfers and profile/original selections.

## Validation and delivery

Acceptance completed with 12 unique focused CTests, 24 Python audit suites /
160 unique tests with zero final skips, and ten UBSAN compilation units / six
successful runs with empty stderr and output identical to final Release.
The initially skipped exception-contract suite was rerun against the accepted
private source. The final Release contains 1,922 freshly compiled generated
units and 100 native AOT units, with fresh UI/audio/probe objects. No previous
State-layout libraries are linked into the delivered application.

The component harness records **198** exact comparisons; production records
**213**. Each mode has **57** exact budget stops and one discriminating negative
per facade. All architectural bytes, unique storage, mapped bytes, allocator
maps, heap/error state, native-call counts, ordered accesses, original PCs and
import boundaries are compared. Each facade executes directly, through its
generated destination and through the generic CFM descriptor tail in production.

| Entry | Component comparisons / faults | Production comparisons / faults | Negatives per mode | Production direct / generated / CFM |
|---|---|---|---|---|
| `0x10770a80` | 63 / 48 | 68 / 48 | 1 | 14 / 1 / 1 |
| `0x10778d50` | 69 / 48 | 74 / 48 | 1 | 20 / 1 / 1 |
| `0x10778d80` | 66 / 49 | 71 / 49 | 1 | 16 / 1 / 1 |

The matrix covers relocated TOCs, aliases between descriptor/TOC-save words,
unaligned addresses, live stack changes during saves/restores, captured frame
addresses, word32 stack wrap, every accessed byte removed from minimum mappings,
finite/dynamically shortened budgets and native faults. Real pointer bridges
prove interval reuse, uncleared payload, alignment and requested-size metadata,
null allocation on the existing upper limit, null release, invalid release and
retained mappings/content after disposal. Global/per-entry/unrelated selection,
interior continuations, independent reset and separate profile selection pass.

The transfer harness records **96** original/modern/mixed comparisons, including
**48** restore/handler faults. Captured contexts reach their real surviving
parent, including recursive CFM calls; interrupted callbacks and facade
frames do not run epilogues. No architectural state or memory/import effects
are normalized. The already delivered semantic scalar-context dependency
charges one operation instead of its original instruction body. The transfer
check measures those dependency charges separately and requires exact remaining
facade/import accounting in mixed modes; aggregate accounting also matches when
both scalar dependencies are original. The initial aggregate mixed-accounting
assertion failed for that documented dependency difference, and the corrected
check then passed. The facade core and delivered application were unchanged by
that harness correction.

An actual AppKit launch event passes through the real event service and executes
the original four-instruction launch callback exactly once. A second facade call
returns the consumed-event error, and the caller SP/TOC/LR are restored. Separate
original and semantic processes both pass, including UBSAN. Related regressions
are allocator equivalence and guarded replenishment, native bridges, CFM
composition, production import descriptors and scalar context transfers.

## Protected installation and installed evidence

The raw linker executable SHA-256 is
`9d2dc21c9e01de5f2cf6b226bd914d2c9980311526660ce0c5f885ba83ef3ffe`.
The signed candidate and installed ARM64 executable both have SHA-256
`dd0d8a6e67eba0165719e2acd19ba554bc6412735e16157f1ba096335e8b90bd`.
Strict signature checks, bundle identity, all four bundled original payloads and
complete candidate/installed content and metadata agree. The previous installed
application and installer source are retained in new private references. The
installer ran once after technical acceptance, with no active user game session.

Protection passes before candidate creation, before installation, immediately
after installation and after the installed smoke. Game content/metadata, user
saves, the LFS save snapshot, original payloads, 23 handoffs and 185 older bundles
are preserved. Runtime logs/preferences legitimately change during smoke and
are not reported as byte-identical after interaction.

Owned installed smoke PID 56841 rendered the start menu, opened the original
chooser through the named Load Game command, returned through named Back or
Cancel, and exited through named Quit plus the original confirmation. The log
records a normal exit and the owned PID is gone. Installed diagnostics show
**185 semantic calls/returns**: event facade **2**, NewPtr facade **93**, and
DisposePtr facade **90**. All three have zero original/profile selection, fault,
budget stop, transfer or escape in this smoke. The previous handled catalog
status -120 appears during startup; there is no fatal bridge, dispatch or
instruction-budget error.

Save/Load is **not retested**. This block does not establish a gameplay speed
gain or complete engine/runtime reconstruction. The whole-port goal remains
active; remaining engine and native-service work is still required.
