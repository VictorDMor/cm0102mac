# Mac register context: reconstruction and integration

## Acceptance boundary

Thirty complete architectural entry components are implemented in named C++23:
all fourteen floating-register save suffixes, all fourteen matching restore
suffixes, scalar nonlocal capture and scalar nonlocal restore. They share four
original bodies containing **91 unique indexed instructions**. Per-entry tails
sum to 299 instructions because the suffixes overlap; that sum is not additional
unique code or a coverage percentage.

All thirty entries run through the default production dispatcher in the
protected installed ARM64 application. The generated manifest contains **264**
entries, SHA-256 `6e438c9dbfe73e5e4466f7ecb2375f14cd8e7ff61acc7838047b4dcb8826531c`.
Its first 234 are preserved exactly from baseline commit
`8ac6653dde59d7853c7b3108e1e341063962168f`, ordered digest
`9f3d57d5077117883869f644578eb0d91f972527febf4b742ab5598f54f043b0`.
Generation retains 1,953,334 indexed instructions / 1,922 pages with zero
missing/pending targets. A full current-layout Release build, focused checks,
UBSAN, protected installation and installed menu/chooser/normal-exit smoke pass.
The complete C++23 port objective remains active.

## Complete pinned scope

End addresses are exclusive. Pins cover ordered canonical `address`, `assembly`,
`ops`, `flows` and `fallthrough` JSON, with sorted keys and compact separators.
All entries occupy page `0x10773`; gaps and padding are not instructions.

| Body root | End | Unique instructions | Role | SHA-256 |
| --- | --- | ---: | --- | --- |
| `0x107737e0` | `0x1077381c` | 15 | Floating save suffix family | `7e6b803134120073784fa22c315b224cc0253cafdaff16ab7093650f0fb6e73a` |
| `0x10773830` | `0x1077386c` | 15 | Floating restore suffix family | `1ab163b6027a9a5a3f503503cde708ae1cd3f0cee9daf62dfe627e2e7f2194b0` |
| `0x10773eb0` | `0x10773f24` | 29 | Scalar nonlocal capture | `e05ca982d2741a838453e7a62994a2526ce7c0b658652d0664962a3b8a80ab95` |
| `0x10773f30` | `0x10773fb0` | 32 | Scalar nonlocal restore | `1e968cb147e30f34867b6035a658f38a0d30d910c84918916fc9394133d42ade` |

Every f18..f31 store/load address is an independently callable suffix entry.
The indexed exterior flows currently target nine starts in each family, with
35 calls per family, plus twelve capture and 21 restore callers. All fourteen
suffix starts are retained and checked, including the five without an indexed
exterior caller. The full flow/fallthrough audit allows those declared suffix
entrances and rejects other exterior interior entrances, including either
family's terminal return. Startup, compiler arithmetic, lifecycle, exception
contexts and vector capture/restore bodies are excluded.

The isolated original reference emits all 91 labels under private template and
dispatcher names. Both finite and unlimited wrappers choose Turbo=false, with
semantic hooks and host-transfer lowering disabled. Body/entry omissions,
changed fields, duplicates and reordered instructions fail closed. Original
records, listings, caller neighborhoods and generated sources remain private.

## Guest contracts and named implementation

`register_context.cpp` implements suffix storage, condition/status word handling
and scalar context capture/restore. These operations do not execute their own
generated bodies, replay P-code or replace guest buffers with host allocations.
Each ordered guest access carries its original instruction PC. Word arithmetic
and addresses remain 32-bit; floating registers are copied as raw 64-bit bits.
NaNs, signed zero and noncanonical scratch bits are stored without a new numeric
conversion or floating-exception policy.

`save_floating_suffix` and `restore_floating_suffix` cover the selected first
register through f31. Register n uses `sp-(32-n)*8`, retaining ascending access
order, original stack/link and untouched earlier registers. The stack address
is read live for each operation. No new frame, memory mapping, heap bookkeeping
or native import is introduced.

The scalar context is 248 bytes:

| Offset | Stored data |
| --- | --- |
| 0 | Live LR captured into r5 |
| 4 | Packed low four bits of each CR field, captured into r6 |
| 8 / 12 | Stack pointer / TOC |
| 16..19 | Original untouched hole |
| 20..95 | Nineteen GPR words r13..r31 |
| 96..239 | Eighteen floating-register words f14..f31, eight bytes each |
| 240..247 | Raw f0 status scratch |

Capture publishes LR/CR/SP/TOC, the general-register suffix, floating registers
and finally status scratch, then returns zero through the current LR. General
word storage retains the live register-address scratch at register offset 2432,
ending at buffer+96; it increments after each successful access. Buffer addresses
are read live where the original does so. Capture packs each FPSCR byte's low
bit into the status word and zero-extends it into f0; existing status bytes and
CR fields retain their original raw values.

Restore loads LR and CR before restoring SP/TOC and the register suffixes. It
loads status into f0 **before** f31, then compares live r4, copies r4 into r3,
normalizes the 32 status bytes from f0's low word and publishes the saved LR as
PC. A zero argument yields one; any nonzero word is retained. Nonzero restoration
returns from the conditional return instruction after 30 original steps; zero
uses its two-instruction suffix and takes 32. XER/CTR/vector/VRSAVE/VSCR and the
other unselected state remain untouched. An unmapped access preserves the exact
prefix, including registers already restored and the current fault PC.

These raw operations publish the architectural destination. The dispatcher
adapter owns selection, instruction accounting, boundary observation and host
continuation transfer. Each destination hook uses `MatchPageCounter`'s semantic
boundary before the original instruction step. Modern execution charges one
entry; tight budgets retain the original stopping prefix using the full tail
bound (including all 32 possible restore instructions). PC profiling retains
the original path, with separate counters. No normal entry replays its own body.

A scoped original-family guard covers every overlapping suffix label until its
original return or fault. It prevents an original f18 tail from reentering modern
f19..f31 hooks. Direct, generated and CFM entry paths use the same adapter.
Return, nonlocal transfer and fault diagnostics are distinct; diagnostic selection,
budget/profiling fallback and dispatch escapes remain process-local.

Scalar capture records the LR/SP values already passed to the existing guest
stores. The association is committed only after the complete capture succeeds.
It belongs to a live dispatcher and adds no memory reads, changes to the context
buffer or serialized host pointers. A semantic caller's captured return routes
to its enclosing dispatcher, abandoning that C++ invocation. A generated callee
with its own continuation, including one sharing its semantic parent's guest
frame, remains within the current dispatcher. A copied buffer restores the same
LR/SP target; recursive invocations retain their separate frame identities.
Only matching State scopes can consume the host transfer. Associations expire
with their owning dispatcher. Foreign/manufactured contexts retain an explicitly
counted stack fallback; such fallback is not evidence of a known capture owner.

## Component evidence

The comparator covers all architectural/register bytes and unique-space bytes,
PC, every guest mapping and byte, heap maps/end/error, native-call count and
ordered read/write bytes at returns and expected faults. Raw semantic bodies
leave `executed` unchanged; the original's completed count remains within its
pinned body bounds. Instruction accounting and process-local diagnostics are
excluded from the architectural snapshot. Nonzero sentinels cover untouched
registers, vector/padding state and populated heap bookkeeping.

| Entry | Tail instructions | Comparisons | Mapping-fault prefixes | Own rejected negative |
| --- | ---: | ---: | ---: | ---: |
| `0x107737e0` | 15 | 98 | 28 | 1 |
| `0x107737e4` | 14 | 96 | 26 | 1 |
| `0x107737e8` | 13 | 94 | 24 | 1 |
| `0x107737ec` | 12 | 92 | 22 | 1 |
| `0x107737f0` | 11 | 90 | 20 | 1 |
| `0x107737f4` | 10 | 88 | 18 | 1 |
| `0x107737f8` | 9 | 86 | 16 | 1 |
| `0x107737fc` | 8 | 84 | 14 | 1 |
| `0x10773800` | 7 | 82 | 12 | 1 |
| `0x10773804` | 6 | 80 | 10 | 1 |
| `0x10773808` | 5 | 78 | 8 | 1 |
| `0x1077380c` | 4 | 76 | 6 | 1 |
| `0x10773810` | 3 | 74 | 4 | 1 |
| `0x10773814` | 2 | 72 | 2 | 1 |
| `0x10773830` | 15 | 99 | 28 | 1 |
| `0x10773834` | 14 | 97 | 26 | 1 |
| `0x10773838` | 13 | 95 | 24 | 1 |
| `0x1077383c` | 12 | 93 | 22 | 1 |
| `0x10773840` | 11 | 91 | 20 | 1 |
| `0x10773844` | 10 | 89 | 18 | 1 |
| `0x10773848` | 9 | 87 | 16 | 1 |
| `0x1077384c` | 8 | 85 | 14 | 1 |
| `0x10773850` | 7 | 83 | 12 | 1 |
| `0x10773854` | 6 | 81 | 10 | 1 |
| `0x10773858` | 5 | 79 | 8 | 1 |
| `0x1077385c` | 4 | 77 | 6 | 1 |
| `0x10773860` | 3 | 75 | 4 | 1 |
| `0x10773864` | 2 | 72 | 2 | 1 |
| `0x10773eb0` | 29 | 243 | 168 | 1 |
| `0x10773f30` | 32 | 247 | 168 | 1 |

Final acceptance: **2,883 component comparisons / 756 mapping-fault prefixes /
30 individual negatives**. Each memory access is probed by removing its first
or last byte. Coverage includes every suffix, minimal mappings with the context
hole unmapped, unaligned buffers/stacks, word32 buffer wrap across 4 GiB,
64 randomized cases per entry, raw floating/status/condition bytes, literal
zero/nonzero/signed-boundary jump arguments and complete save/restore round trips.

Live observers exercise changing stack/context addresses, a subsequent general
register source, subsequent floating slots, status before capture, f0 before its
final publication, f31 after status load and r4 before its result comparison.
Every entry has its own corrupted result rejected: saved memory, a restored
floating register, captured LR or resumed result as appropriate. The initial
harness assumed a one-step nonzero-return difference; original execution showed
the two-step suffix. That failed bound run is retained privately and excluded
from the final acceptance run.

## Production and continuation evidence

Every entry has two normal direct, two generated and two CFM witnesses (Turbo
disabled/enabled), plus four independent original-selection witnesses. Normal
root calls have no original/budget/profile selection or dispatch escape. Thirty
additional boundary-result negatives are rejected. Integration acceptance has
**1,169 comparisons / 598 exact budget stops / 60 mapping faults / 30 negatives**.
Every insufficient-budget prefix matches the original's executed count,
architectural state and ordered effects. Original selection covers complete
shared suffixes, including observation disabled. Profiling supplies sixty
separate original-path cases. Manufactured restore contexts deliberately have
no live capture association and expose the independently counted fallback;
these cases are not known-owner continuation evidence.

Actual nested capture/restore integration adds **80 comparisons**, including
sixteen restore/handler faults, original/global and two mixed selections, both
Turbo settings, parent/child continuation, generated children, shared frames,
recursive outer/inner frames, copied contexts, peer-State propagation and expired
owners. Completed comparisons require zero routing fallback/escape, exact
surviving-parent counts and no interrupted-child or jump-caller continuation.
Memory markers reject stale epilogue writes. Restore faults stop before transfer;
handler faults occur after the owner resumes. Peer State cannot consume the
owner's signal, and completed/faulted dispatch scopes leave no stale association.

Ten focused CTests pass: the four new component/production/profile/transfer
checks, three compiler-arithmetic regressions on the shared generated page, and
exception transfer/capture/production integration on the shared host machinery.
The profiling check was independently rerun after correcting its environment
variable; the initial failed configuration is excluded. Fifteen affected Python
suites pass **101 tests, zero skips**, including eight new register-context audit
tests. The earlier unbounded arithmetic enrollment assertion was narrowed to its
unchanged eight-entry slice; prior-prefix content pins remain strict. An initial
exception audit with unavailable-source skips is excluded; the accepted run uses
the full current private source.

Fresh UBSAN instruments eleven units: both register-context implementation units,
runtime/guest continuation machinery, all three new harnesses, isolated original
reference, shared generated page and the two directly affected exception
harnesses. All six executable checks pass; stdout matches the full Release
executables byte for byte. Stderr is empty except the explicitly enabled profiling
check, whose original-PC samples also match Release exactly. The private two-page
preflight is separate from the full 1,922-page production build and acceptance.
No legacy State-layout library is used. ASAN was not run.

## Protected installed application

The prior installed arithmetic executable is retained privately with its content,
resource forks, xattrs and metadata; SHA-256
`6814c65faad2615a547a5d3ce97a3b6f840ec20c58bd09c7cac8b7a76ae3beb3`.
Existing linker outputs, candidates and reference bundles are preserved.
The current linker output has its own private identity. The signed source,
signed candidate and installed internal ARM64 executable match SHA-256
`05f50e3350ac9244a0796093ddf186e7171f3c5a7e34c954e4d7741e162a42f1`.
Strict signature, production plist and all four legacy OriginalProgram payload
hashes are verified. The installer uses the accepted consistent private build;
it runs once, after confirming no user game process.

Content and full metadata protection pass before installation, immediately after
installation and after smoke for game data, existing saves, Git LFS save snapshot,
original payloads, twenty-three handoffs and 164 historical application bundles.
Runtime/Runs/preferences may change during smoke and are not claimed identical.
The owned installed PID 25295 renders the main menu, opens the named Load Game
chooser, cancels without loading a career, and completes original Quit/Return
confirmation. It exits normally after 9,058,366,828 statically recompiled
instructions. The installed hash remains unchanged and no game process remains.

The smoke records **337 modern captures/returns and one modern restore/transfer**,
zero original selections, budget/profile fallback, faults, dispatch escapes or
unknown transfers. All twenty-eight floating suffix UI counters are zero.
Installed smoke therefore covers capture/restore only; individual evidence for
all thirty entries comes from the separate production harness.

Save/Load is **not retested**. No career, day advance, match, broad gameplay
campaign or performance benchmark is run. Whole-game/runtime completion and
measured speed gains remain unproven.
