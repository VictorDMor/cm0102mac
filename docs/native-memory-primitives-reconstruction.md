# Semantic Mac memory primitives

## Scope and evidence boundary

Six original Mac block-memory entries are reconstructed in readable C++23:
block copy through the existing Carbon integration, a destination-returning fill
wrapper, first/last bounded byte search, normalized block comparison and an
aligned fill leaf. Internal NUL bytes remain ordinary block data. This closes
the specified six-entry core, not all memory services, the C runtime or engine.
There is no visual or performance claim.

The starting checkout is `main` at
`91b0cfadea5ffc60994d6911e596af2960ca2955`, freshly checked against the existing
`git@github.com:VictorDMor/cm0102mac.git` remote. No applicable `AGENTS.md` exists
in the repository or inspected ancestors. Fourteen private untracked handoffs
and five tracked prompts are retained. The existing private SQLite instruction
index is read-only input; no new Ghidra export or proprietary listing is published.

Date, RNG, presentation, background, collection and string reports remain
historical evidence. Save/Load, save/exit/reopen/load, career comparison and
serialization auditing are explicitly waived by the user on October 5, 2026.
Persistence is **not retested**, neither passed nor a pending acceptance gate.
No live app session or personal career is opened. Synthetic fixtures prove the
specified execution boundary and are not live gameplay or UI evidence.

## Pinned original boundaries

Fingerprints cover address-ordered original `address/assembly/ops/flows/fallthrough`
records, compact sorted-key UTF-8 JSON. Included partial, changed or duplicate
bodies are rejected. The total is 119 indexed instructions.

| Entry | Exclusive end | Indexed instructions | SHA-256 |
| --- | --- | ---: | --- |
| `0x10739ef0` | `0x10739f2c` | 15 | `05753a5ef9136a3145548dc38d58233103e180dba5e5bd108b49975c7f3f80cc` |
| `0x10778300` | `0x10778330` | 12 | `6b9670af9fecd70c81015ca9a73e470266ec3eb4ff3467391ac3ac859f5db50f` |
| `0x10778330` | `0x10778360` | 12 | `8da31292109a0dec94712af7b85a3179ffeffd3002d8a706600ae37c4fe56b12` |
| `0x10778360` | `0x10778390` | 12 | `32b1246d369b4edcef213b8d941f0bad79142c76d12188af939051d43a4c187b` |
| `0x10778390` | `0x107783e0` | 20 | `f95bcb514d99050544dfba105b2be24722695b33bb32e80c6fc60580e90be7f0` |
| `0x107783e0` | `0x107784a0` | 48 | `0a98cb1d92b0f33ccdd4204b7b7ec3cce64d573dc1bb2c1453dea7e9a59a1a4e` |

Every indexed static flow and fallthrough is inspected for external edges into
the six interiors: none exists. This does not classify unknown dynamic entries.
The original direct indexed CALL counts are 123, 207, 1, 1, 12 and 1 respectively;
these establish static relevance, not execution coverage of every caller.

## Original contracts and semantic implementation

### Block copy `0x10739ef0`

Destination/source/count are r3/r4/r5 word32. The wrapper captures LR in r0,
spills r31, preserves the destination in r31, exchanges the pointer arguments,
stores LR and writes a 64-byte guest frame/backchain. It calls the existing
`0x1077fc28` stub with source/destination/count and the generated transition ABI,
then reloads TOC from frame+20, returns the surviving r31 value, reloads LR,
restores the frame and finally reloads the r31 spill. Spills/TOC may alias the
block, and dependency register clobbers remain observable.

The stub remains generated: it loads the mutable vector at r2-0x7e64, writes
TOC to frame+20, reads entry/TOC and transfers through CTR. Native import state,
call counts, access PCs and faults use the existing runtime. Carbon's
`BlockMoveData` snapshots the complete source via `Memory::copy_out` before one
bulk destination write; it therefore preserves overlap in both directions.
`copy_out` validates its interval and emits no scalar read events. No new events
or bridge implementation are introduced. Count zero still executes the stub and
import, without block data access. A failed bulk validation writes no byte prefix.

### Fill leaf `0x107783e0` and wrapper `0x10778300`

The leaf initially compares the unsigned count with 32, reduces r4 to unsigned8,
sets r7 to that byte and r6 to destination-1. Small fills write progressive bytes;
zero performs the architectural setup and comparison without data stores.
Large fills align to four bytes with individual byte stores, expand nonzero bytes
to a repeated word, store eight separate four-byte words per 32-byte group,
remaining words, then up to three tail bytes. Zero skips word construction while
retaining the original registers. The first group store precedes its counter
decrement; later stores and the updating eighth store follow it. Remaining-word
and byte loops decrement before their stores. No bulk fill replaces these writes.

Counts/cursors wrap at word32, XER carry and CR record effects are explicit, and
updating cursors commit only after successful memory access. Architectural
residues and faults after already written prefixes are retained. In the small
path r3 remains the destination; in the large path it is reused for counts and
normally ends at zero. The leaf therefore has no unconditional destination-return
promise. Its wrapper preserves r31/LR in a 64-byte guest frame, calls the same
leaf guest entry, restores the destination return and reloads its frame/spills.

### Bounded searches `0x10778330` / `0x10778360`

The requested byte truncates to unsigned8. First search adjusts base-1 and loads
forward; last search starts at base+count with wrap and loads backwards. Both
increment count initially, decrement before each access with word32 carry/CR,
return the first matching address in their scan direction or zero, and treat NUL
as searchable data. Count zero does not access even invalid pointers. Updating
loads preserve the previous cursor when they fault; high counts are not reduced
to signed limits. A matching address zero is retained even though the pointer
result also represents absence; its flags/count distinguish the executed branch.

### Block comparison `0x10778390`

The leaf uses r6/r7 as advancing left/right cursors and r4 as a word32 remaining
count, incremented initially and decremented before each pair. It reads both
unsigned8 bytes, including internal zero, until count exhaustion or divergence.
On divergence it explicitly rereads left, sets r3 to +1, rereads right, then
compares the new bytes. Only a lesser reread left changes the result to -1;
rereads that have become equal still return +1. Results are exactly -1/0/+1,
not the previously delivered string comparator's unsigned-byte difference.
Read order, cursor commits, count/carry/CR and faults remain architectural effects.

## Production integration and private diagnostics

Named semantic operations share small explicit ABI helpers for big-endian guest
accesses, word wrap, flags, updating accesses, guest calls and frames. Normal
leaves never execute a replaced body or interpret opcodes. The copy intentionally
uses the unreplaced stub/import; the fill wrapper calls its independently selected
leaf through generated dispatch.

Hooks on destination labels precede the first original operation on pages
`0x10739` and `0x10778`. Local CALL/goto, page dispatch and existing indirect
transitions reach those same hooks. One regeneration retains 1,953,157 indexed
instructions across 1,922 pages, with zero missing/pending targets. The manifest
appends six entries to the unchanged ordered prior 60, for **66 replacements**;
the audit also pins the prior manifest's compact JSON SHA-256. Date's ascending-
year leap asymmetry and club/competition null-getter guards remain intact.

Global/per-entry original selection is private and independent of all earlier
semantic modules. `CM_AOT_MEMORY_PRIMITIVES_ORIGINAL` selects the probe reference.
Counters and return observers are process-local diagnostics, outside save/snapshot
serialization, adding no guest reads and replacing no existing observers. Defaults
select all six semantic entries. Optional observed original mode executes the real
guest body and records its own return without callback context restoration.

## Focused equivalence and discriminating fixtures

`cm-memory-primitives-checks` / `modern_memory_primitives_equivalence` starts both
modes from identical inputs while all prior modules remain semantic. It compares
every architectural register/temporary byte, CR/XER/LR/CTR/TOC/PC, all mapped
memory, allocation/free state, heap/error/native-import state and the interleaved
read/write/map/clear transcript, including prefixes before faults. Instruction
counts and private diagnostics are excluded. Independent expectations check bytes,
pointers, exact results, guards, read direction/order, store width/order and
count/cursor/carry effects; no candidate output is recorded as the oracle.

The bounded fixture matrix covers repeated/absent/NUL/high/truncated search bytes,
zero/one/before/at/after limits, reversed scans, word32 counts and address wrap;
binary comparisons at first/last bytes, differences outside limits and internal
NULs; controlled divergence rereads that reverse ordering or become equal;
fill values, all four alignments, small lengths, 31/32/33, full word groups and
tails, distinct leaf/wrapper returns and partial store faults; empty/disjoint/
equal/overlapping Carbon copies, invalid/insufficient/extreme bulk intervals,
relocated TOC, partially mapped vector/frame/TOC fields, spill aliases and a
controlled dependency r31 clobber. Extreme counts hit faults in finite mappings.
Minimal mappings are built from original accessed bytes, explicitly including
the bridge's bulk source interval without fabricating read events.

Each entry has its own mutation rejected at its first own return boundary. The
fill wrapper's leaf boundary is checked before the parent; a separate nested
child mutation is rejected before the parent can return. Reset and private mode
selection are checked. Both original-wrapper/semantic-leaf and inverse modes are
compared. Normal witnesses require six nonempty own counters and no original
selected execution; six generated vector transitions exercise the same hooks.

Two generated `0x107784c0` consumers convert empty/nonempty NUL strings to a
length-prefixed buffer, actually reaching semantic string length, the new copy
and Carbon import. Two original `0x1077b680` formats (`%.8s!` / `%.3s!`) cover
normal/truncated precision string output and require the limited-search child's
counter. Formatter fixtures initialize valid IEEE FPSCR. These consumers,
allocator/pools, newline I/O, stub/import, streams and general formatting remain
generated or use their existing integration. No allocator, whole formatter,
Date/RNG, historical replay, season, whole-suite or UI campaign is run.

## Validation and protected delivery

One regeneration and a consistent Release ARM64 build completed without
compiler warnings or errors. Every dependent generated object was rebuilt after
the State diagnostic layout extension. The focused CTest group passed on its
first run; no production or runtime-fixture correction was needed.

| Executed check | Result |
| --- | --- |
| `modern_memory_primitives_equivalence` | PASS: 270 comparisons, 96 unmapped-access faults, complete architecture/memory/ordered effects |
| Own return controls | PASS: six entry-specific negatives, independently checked nested leaf and parent, one nested child negative |
| Normal dispatch | PASS: six nonempty own entries, zero original-selected executions in normal transition witnesses |
| Mixed fill modes | PASS: original wrapper/semantic leaf and semantic wrapper/original leaf |
| Generated indirect transitions | PASS: all six entries through the existing guest vector mechanism |
| Generated consumers | PASS: two Pascal conversions, two actual precision `%s` search compositions |
| `modern_string_core_equivalence` | PASS: existing string/dependency equivalence regression |
| `original_decimal_formatting` | PASS |
| `native_bridges` | PASS: existing native bridge regression including overlap |
| `python3 tools/test_memory_primitives_audit.py` | PASS: five body/hook/edge/manifest/dependency tests, no private-index skips |
| Updated string/collection/background audits | PASS: 5/5/4 tests; only manifest extension expectations changed |
| Focused UBSAN | PASS: all 270 comparisons and own/nested controls, empty stderr |

UBSAN instruments exactly `memory_primitives_adapter.cpp` and
`memory_primitives_checks.cpp`, with C++23/ARM64, `-O1`, `-ffp-contract=off`,
`-fsanitize=undefined` and `-fno-sanitize-recover=all`, linked to the consistent
engine libraries. It is not whole-engine instrumentation or ASan. Four focused
CTests and nineteen Python audit tests close the specified technical boundary;
no green group is broadened or repeated without a concrete affected failure.

### Protected installation

The prior installed app was copied once to the new private
`.local/memory-primitives-reconstruction/reference/installed.app` with resource
forks and extended attributes, then fully compared. It retains the immediately
previous strings build, not the new candidate. All three named background/
collection/string references remain intact; fifteen additional historical
reference/candidate/linker bundles are also checked and unchanged.

The compiler/linker bundle is retained separately as `source-linker.app`.
Original payloads were included before ad-hoc signing the build source and
retaining `candidate.app`. The protected `python3 tools/install_native.py` ran
**once**, on October 5, 2026, from 23:58:15.273810 to 23:58:15.943756 UTC
(20:58:15.273810–20:58:15.943756 in São Paulo). The existing-game and private
English club-index compatibility checks passed.

| Artifact | Architecture | Executable SHA-256 |
| --- | --- | --- |
| Retained compiler/linker output | ARM64 | `9f888b81f9e8aa35079663a599fd5f794af719d06218cdfcf4900d100bd2a634` |
| Signed build source | ARM64 | `6c00be6327cbbee65a57078320bfd2b6c530e2d1a7d9d34ba8a001e59b20e6b5` |
| Signed candidate | ARM64 | `6c00be6327cbbee65a57078320bfd2b6c530e2d1a7d9d34ba8a001e59b20e6b5` |
| Installed app | ARM64 | `6c00be6327cbbee65a57078320bfd2b6c530e2d1a7d9d34ba8a001e59b20e6b5` |
| New prior-installed reference | ARM64 | `733a5fca03917f5e74fdd35109de93caac221b522cd8b8098b0c9f509fe0e721` |

Signed source, candidate, installed bundle and new reference pass
`codesign --verify --deep --strict`. All 4 bundled original files match the
existing private inputs in all four. Source/candidate/installed production
plists are identical, with no diagnostic/validation keys or `LSEnvironment`.
Installed executable identity is checked directly, independently of bundle-folder
modification dates. The retained unsign-adjusted compiler/linker output has its
own architecture/hash receipt.

The one before/after personal preservation check confirms identical content,
resource forks, Finder information, extended attributes, modes, modification
times and flags: 809 Game files, 5 Runtime files and 46 Runs files.
These are current observed counts, not enforced historical invariants.
Presentation.plist remains absent. The LFS snapshot, original inputs, all
19 retained prompts (fourteen private handoffs plus five tracked prompts), the
new reference and all protected historical references/candidates are unchanged.
No personal career or live app session was opened. Installation identity adds
no fresh UI, gameplay or persistence evidence.

Only product sources, focused checks and technical reports are published to the
existing private `origin` / `main`, freshly reconfirmed with ADMIN permission.
Private handoffs, payloads, saves, snapshots, fixtures, logs, receipts, screenshots
and bundles remain protected/untracked; ignore and LFS rules are unchanged.
Normal commit/push is followed by a fresh remote-SHA verification. There is no
force push or broader acceptance campaign.
