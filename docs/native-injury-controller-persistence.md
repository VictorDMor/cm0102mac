# Injury controller archive loading and writing

Two complete original operations have named C++23 bodies under the separate
`injury_persistence` owner. The accepted manifest extends from **732 to 734
entries**, retaining the exact preceding prefix.

| Original range (end exclusive) | Operation | Indexed instructions |
| --- | --- | ---: |
| `0x10184530..0x10184e58` | Load injury controller archive and convert legacy records | 584 |
| `0x10184e60..0x10185404` | Write injury controller archive and list records | 361 |

All **945 indexed instructions** are pinned. Two unindexed interior words,
`0x1018484c` and `0x10184e10`, remain outside the indexed contract. Complete
bodies, page fragments, original edges and productive hooks are audited; no
indexed external edge enters either replaced interior. The exact preceding
732-entry manifest has canonical SHA-256
`44124ee4dcf6c2b935e30d238e570d2807a8e1aff95d31ac59917f8abb220582`.
The original string pointer and all 124 bytes through the `injury.dat`
terminator are pinned independently of the instruction fingerprints.

Loading preserves the null-path diagnostics, original archive open mode and
metadata, live staff count, wrapped allocation sizes, partial object writes,
read-result checks and failure flag. Older versions retain the 52-byte record
load, 56-byte destination stride and both signed date conversions. Current
versions retain the bulk record read. Optional arrays preserve their live counts
and capacity mirrors. The optional list retains signed short counts, original
replacement/allocation/append calls, unchecked null outcomes and cleanup order.

Writing preserves each serialized field, original array order, optional counts,
list cursor reset, original iterator and archive close. The original last-record
write-error path deliberately leaves the archive open. Earlier errors close it;
null paths and failed opens never close a nonexistent archive. No added cleanup
changes the interrupted prefixes or original failure behavior.

Frames remain 768 and 528 bytes. Registers, flags, temporary residues, link
registers, every ordered memory effect, dependency ABI and instruction accounting
are compared. Named source helpers express complete control flow without
interpreting or dispatching either body's own instructions. Shared guest-call
helpers and serialized State layout are unchanged; only private diagnostics grow.

Release and affected-unit UBSAN each pass **34,717 component comparisons**:
167 completions, 34,546 interruptions and four discriminating own/dependency
negatives. All 945 indexed source sites are covered. Representative paths test
every instruction-budget cut, individually missing accessed bytes and every
dependency interruption. Cases include short reads/writes, every allocation
failure, missing paths, both format branches, signed counts, live count mutation,
aliasing and relocated TOC. Independent checks verify the serialized byte order,
loaded capacity mirrors and the original late-write open-file behavior. All
three final sanitizer runs have empty stderr. Eight affected units are
instrumented: two bodies, adapter, private diagnostics, independent original
reference and three harnesses. Six selected new/shared headers are fingerprinted.
Existing dependency libraries remain consistent Release builds.

## Production and interruption evidence

Nine new source/index/string/hook/reference audits, nine preceding bootstrap
audits, **56 historical manifest methods** and four focused CTests pass.
Fifty-four historical files change only the current manifest count; the preceding
bootstrap audit also freezes its six-entry suffix. Full generation retains
1,953,334 instructions, 1,922 pages, 1,923 C++ files and zero missing/pending
targets. Only `page_10184.cpp` changes: both entry hooks lie on that page, while
the original-only continuation on `page_10185.cpp` stays byte-identical.

A paired **798-advance complete match** preserves complete state, imports and
per-tick accounting. Both roots have zero calls inside its stepper. Prepared
actual-data original BLs at `0x101842d4` and `0x1034d308` reach and return through
the respective bodies with dedicated nonzero counters. The two primary inputs
and three variants pass **85 production comparisons**: fifty returns, including
five discriminating negatives, and 35 budget stops. Two additional actual pointer
faults preserve partial state, ordered effects, imports and accounting. Runtime
source-site counts are 204 and 113; the complete component matrix covers all
945 sites. **15 Release original-profile controls** also pass.

The independent match factory provides 153,796 staff rows and a zero archive
format discriminator. The prepared archive fixture retains the first two actual
staff rows and up to two actual injury/optional records, with explicit current-
format context at `0x3e024`. This bounds file IO without changing either body.
A complete 264,008-byte empty archive header is written through original stream
and metadata routines before paired writes. Legacy data is written through the
original archive writer with 52-byte records; its load uses the discriminator
`0x3e023` and reaches both date conversions. Additional inputs cover optional
arrays and lists allocated, appended and iterated through original routines,
using actual injury-record bytes. These are prepared runtime inputs, distinct
from traversing the complete management UI or loading a user's career.

The **112 nonlocal comparisons**, including **64 restore/handler faults**, use
genuine original context capture/restore and a surviving parent. Both framed
roots cover three writes and three reads over both observer channels. Nested
boundaries enter the original archive-open dependency. Ordered effects, import
ABI, measured accounting, original-context/turbo controls and abandoned host
continuations agree. Context scratch is separate from live archive strings.

Original guest faults/transfers can abandon an open write fork. The independent
replay releases only that original run's private host lock after observing its
outcome. Guest memory, open-reference evidence and import traces stay intact;
matching guest open-fork counts are required. This fixture isolation adds no
cleanup to the game. Initial overlarge-archive, missing-version, invalid-empty-
header, host-lock and scratch-overlap failures and their corrections are retained
privately. No semantic body changed after the first complete component comparison.

## Installation and remaining scope

The strict-signed ARM64 game is installed at
`/Users/victordmor/Applications/CM 01-02 Native Prototype.app` with executable
SHA-256 `1aad17166edc1f9229baa02118fc804ae5822971460d41aa2eb801157fb660ac`.
Candidate, installed and running executable identities agree. Prepare Game renders
and the original exit confirmation completes normally after
4,154,366,305 statically recompiled instructions. This startup smoke
is separate from the dedicated runtime evidence above.

Debugger sources and the installed debugger remain unchanged; its executable
SHA-256 is `2054db1cc3f620b5cf4923b857a8beb65e16cd42498dbe474eb493185456eb89`.
All **846 protected files**, including both user career saves, and **18 preexisting
feature prompts** retain their hashes. Seven owned linked checks are removed,
retaining reusable objects, libraries, apps, references, source/link recipes and
evidence. Verified transparent APFS compression covers **18 task-owned snapshots**,
preserving ordinary-read hashes and metadata. Allocated snapshot storage decreases
by 1,804,464,128 bytes while retaining
2,285,495,388 logical bytes. Original archive/history backing
remains outside that whitelist.

Save/Load is **not retested**; private archive/snapshot checks do not establish
whole-product persistence acceptance. No gameplay change or performance benchmark
is part of this cycle. The complete injury controller, injury system, match engine
and whole-game reconstruction remain unfinished; entry count is not a completion
percentage. The full goal remains active.
