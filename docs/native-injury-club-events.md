# Club injury event queue

Three complete original operations now have named C++23 bodies under a separate
`injury_club_events` owner. The accepted manifest extends from **711 to 714 entries**,
retaining its exact prior prefix and all prior semantic owners.

| Original range (end exclusive) | Operation | Indexed instructions |
| --- | --- | ---: |
| `0x1018bb50`..`0x1018bdd0` | Eligibility, growth, insertion and sorting | 160 |
| `0x1018bdd0`..`0x1018c180` | Daily processing, roster selection and expiry | 236 |
| `0x101900b0`..`0x10190180` | CFM sorting date comparator | 52 |
| Total | Three complete bodies | **448** |

## Preserved contract

This is the controller's 20-byte club-event queue, distinct from the preceding
24-byte start/end queue. Insertion preserves signed low-byte eligibility, the
type-4 skip, type-3 modulo-15 gate, other types' modulo-4 gate, original club
predicate and all pool/salt/wrap effects. Capacity grows by thirty records before
allocation/reallocation; failure preserves diagnostics, partial writes and flag
clearing. Date fields retain ordered accesses, live base/count reloads, optional
context identity and the qsort CFM descriptor chain
`0x108b6630` -> `0x108b8b44` -> `0x101900b0` with TOC `0x108bba90`.

Processing preserves its 192-byte frame, five-day cutoff through the original
date API, signed-day/unsigned-year comparisons, future-event early stop, expired
prefix count, type-dependent event count, twenty-attempt roster loop and original
injury-registration ABI including its stack argument. Club/player guards, live
last-type reads, post-callback state, overlapping compaction and final count write
retain original order and fault behavior. Insertion retains its 144-byte frame;
the leaf comparator retains below-stack scratch, padding reads, aliasing and early
returns. No component dispatches or interprets its own original body.

Whole source fields, page fragments, indexed incoming edges and the three-word
CFM descriptor chain are pinned. No indexed external edge enters a selected
interior. Complete originals remain available for global, per-entry and profile
controls. Diagnostics extend private metadata without changing serialized State.

## Accepted evidence

Full generation retains 1,953,334 indexed instructions, 1,922 pages and zero
missing/pending targets. Only page_1018b.cpp and page_10190.cpp change among
1,923 generated C++ files. The consistent shared build consumes the completed
generation and preserves the exact 711-entry prefix. Nine new source/scope/
reference/descriptor/hook audits and eighteen audits of the two preceding injury
owners pass, together with 52 historical manifest methods. Fifty older test files
change only current manifest totals; the preceding progression audit additionally
freezes its accepted suffix at entries 707 through 710.

Release and affected-unit UBSAN each pass **54,873 component comparisons**:
562 completions, 54,306 interruptions and five discriminating own/child negatives.
All **448 indexed source sites** are covered. The 74 paired structural cases
control an otherwise unreachable comparator condition and remain separate from
ordinary inputs. Independent original references compare registers/temporaries,
mapped memory, ordered effects, dependency input/output ABI, source order and
accounting. Independent expectations check event identity binding and date order.
Representative paths compare every instruction-budget cut, every individually
missing accessed byte and every dependency interruption; additional paths use
sampled budget cuts with the full missing-byte/dependency matrix.

A paired **798-advance complete match** preserves state, imports and per-tick
accounting. The three new roots have **zero natural calls** in that stepper.
Prepared actual-data evidence instead uses original allocation/date/reset calls,
a real club/player/controller, canonical dates, controlled roster references and
pool values. Original BLs `0x1018aea8` and `0x10186488` traverse complete insertion
and processing bodies. Insertion produces one call/return and six real qsort CFM
comparator calls/returns at `0x107685ec`; processing produces one call/return and
reaches the existing reconstructed injury-registration owner. This is prepared
runtime coverage, separate from unmodified management-UI coverage or traversal of
the complete outer registration/management parents.

Three captured roots and eleven variants cover skip, allocation/reallocation,
context identity, empty/expired/future/type-3 events, empty roster and date order.
They pass **238 production comparisons**: 140 returns including fourteen negative
controls, and 98 budget stops, plus **three separate actual pointer-fault
comparisons** with matching partial state/effects/import ABI/accounting.
**42 original-profile comparisons** pass in Release. Prepared runtime source
coverage is 110, 216 and 44 sites; complete coverage comes from the component matrix.

**154 genuine nonlocal comparisons**, including **88 restore/handler faults**,
pass in Release and UBSAN. Both memory-observer channels cover initial saves,
frame/data boundaries, leaf spills/reads, actual enqueue-to-qsort/comparator and
process-to-registration nesting. A surviving parent owns original context capture;
interrupted continuations are abandoned without false returns, leaked original
scope or unknown transfers. State, effects, actual import ABI, measured accounting,
original-context and turbo controls agree. The comparator's transfer input uses
equal dates to reach its later scratch accesses.

All four focused CTests pass. Eight affected units carry UBSAN instrumentation;
all three full sanitizer runs exit successfully with empty stderr. Final hashes
cover those sources and five shared/new headers. Other archives and generated
pages are consistent Release builds; whole-engine sanitizer coverage is not claimed.

## Installed game and retention

The strict-signed ARM64 game is installed with executable SHA-256
`39b59e694b81691c7b44c1861c6feaa63e3be6cfabb23249a3a1f4ac5ded6c24`.
Candidate/installed identities and all four original payloads agree. Prepare Game
rendering, original exit confirmation and normal exit pass. Startup smoke is
separate from dedicated prepared runtime coverage.

All 845 protected original/save/handoff files, eighteen existing untracked prompts,
the previous installed-game reference and installed Match Debugger are preserved.
Exactly six reviewed linked checks are removed (864,395,480
logical bytes), retaining objects, archives, apps, references, snapshots and recipes.
Transparent APFS compression preserves ordinary-read hashes, paths, permissions
and modification times for 45 selected snapshots, retaining
5,713,553,610 logical bytes and reducing allocated storage
by 4,455,542,784 bytes. Free space afterward is
73,745,797,120 bytes. Protected originals, saves and app bundles
are outside the compression targets.

Save/Load: **not retested**. Performance: **not benchmarked**. The complete port
goal remains active; 714 accepted entries are an integration count, not a
completion percentage.
