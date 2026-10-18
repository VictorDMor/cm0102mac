# AWOL records, daily review and supporting sampling

## Status and complete module boundary

The original `AWOL.CPP` source block contains 21 complete operations and 6,958
indexed instructions at `0x100031b0..0x10009ee8`. No indexed external edge enters
the interior of any operation. Eleven non-indexed padding no-ops are identified.
The following CASH string helper remains outside this source block.

All 21 operations have accepted C++23 implementations, covering all 6,958 indexed
instructions. The consistently generated, built and installed game contains
756 unique entries and passes the actual-world Release matrix, focused sanitizer
checks and installed startup/normal-exit smoke. Every prepared
entry was checked to have no accepted semantic owner before implementation;
existing actor/condition implementations remain unchanged.

## Reconstructed operations

| Entry | Exclusive end | Instructions | Operation |
| --- | --- | ---: | --- |
| `0x100031b0` | `0x100032f8` | 82 | Construct flag storage and three lists; retain load fallback |
| `0x10003300` | `0x100033b8` | 46 | Destroy owned storage with the original signed deletion flag |
| `0x100033c0` | `0x100036dc` | 196 | Read the AWOL archive and its version-dependent third list |
| `0x100036e0` | `0x10003dc0` | 437 | Write flags and three record layouts with original diagnostics |
| `0x10003dc0` | `0x10003ec0` | 64 | Read primary absence status and validate its club association |
| `0x10003ec0` | `0x10003fa0` | 56 | Read secondary absence status and validate its club association |
| `0x10003fa0` | `0x10004068` | 50 | Search the pending-record list by staff ID |
| `0x10006600` | `0x100066d0` | 52 | Remove a primary record by staff |
| `0x100066d0` | `0x100067d0` | 64 | Remove a selected primary record and request its notice |
| `0x100067d0` | `0x10006828` | 22 | Remove a selected secondary record |
| `0x10006830` | `0x10006900` | 52 | Remove a secondary record by staff |
| `0x10007100` | `0x100072e0` | 120 | Populate and route an absence-change notice |
| `0x10006080` | `0x100065f8` | 350 | Sample signed bounds using the original shared random pool |
| `0x10004070` | `0x10006074` | 2,044 | Review primary, secondary and pending absence records daily |
| `0x10006900` | `0x10007100` | 512 | Register a primary absence, physical effects, relationships and notice |
| `0x10008770` | `0x10008b58` | 250 | Construct and route an eleven-field training-absence notice |
| `0x100072e0` | `0x10007a4c` | 475 | Describe primary absences in brief and detailed views |
| `0x10008b60` | `0x1000909c` | 335 | Describe training absences and discipline in both views |
| `0x10009b60` | `0x10009ee8` | 226 | Describe fitness concerns and related player/club links |
| `0x10007a50` | `0x10008770` | 840 | Process training absence, conversion, fitness effects and discipline |
| `0x100090a0` | `0x10009b54` | 685 | Register fitness concerns, choose a reporter and apply discipline |

## Preserved contracts

Construction keeps the original partial-load fallback: it frees the flag buffer
and primary list before recreating storage, without inventing cleanup of other
partially loaded lists. Allocation failures preserve the original subsequent
guards or faults. Destruction retains signed 16-bit narrowing of its delete flag.

Archive operations preserve short reads/writes, partial effects, signed counts,
the numeric version comparison at `0x3e005`, 12/16/20-byte record layouts, scratch
areas, diagnostics and close ordering. Iterator counts and pointers are reloaded
in their original order, including mutations made by dependencies. The six
archive padding words and the original diagnostic text
pointer/bytes are pinned.

Status reads preserve signed staff-ID checks, raw flag handling, cache writes,
club lookup and removal calls. Secondary status returns its original masked
value of two. Item removals retain the original conditional XOR behavior, even
when it differs from a conventional bit-clear operation. Notices preserve signed
byte/halfword arguments, all nine message fields, the ability threshold and the
original broadcast/club routing and repeated pointer loads.

The sampler has the same instruction structure as the accepted actor sampler
after relocating local targets, with a different TOC constant slot. Its two
original floating constants are byte-identical. Its own original-reference
comparison still checks all 350 sites, recursive calls, wide branches, wrap,
reseed and floating status; it does not delegate to a wrapper with different
instruction addresses. Compilation uses `-ffp-contract=off`.

The daily controller retains its 192-byte frame and all three independent list
reviews. Primary expiry compares the recorded date directly; secondary and
pending expiry first use the original date adjustment dependency. Equal dates
are not expired. Each list processes at most one expired item that is removed;
pending records that fail their recovery check keep scanning. A null iterator
result exits the entire controller, rather than just its current list. Signed
counts, cached pointers, live count reloads and missing-profile faults remain
unchanged.

Daily linked comparisons now execute the complete training-event child as well.
After removing an expired secondary item, that original child can set the flag
again and append a replacement event. Independent expectations retain this
composition rather than assuming the post-removal count/flag remain final.

Return-from-leave and unapproved-return paths preserve their distinct gates and
trait effects. In particular, two unapproved-return decrements require a
nonzero draw while leave and young-peer decrements require zero. Peer review
retains fifty roster slots, its absent-player lookup argument, contract-bit,
age and ability guards, and the repeated staff-trait bound used for the profile
trait draw. All eighteen inline bounded draws keep their register/scratch
variants, cached-cursor order, wrap/reseed, signed remainder, floating status and
independent sampler call. Named helpers implement these operations directly;
they do not dispatch an instruction table. Five additional daily padding words
and all three complete original page fragments are pinned.

Primary registration keeps the date constructor call before staff validation,
the signed ID guard without an added lower-bound check, the existing flag gate
and OR update, and the three separately sampled floating-point reductions of
profile metrics. It retains fifty live roster reads, both peer-relationship
draws, manager/club relationship calls, mode-dependent signed duration draws,
the twelve-byte record append and all three notice-routing cases. Reseed
callbacks still precede the original live profile reloads. The full four-double
constant block is now pinned, including 1.0 and 100.0.

The training notice preserves its 336-byte frame, all eleven message fields,
signed selector narrowing, manager versus alternate reporter selection, and
the original five-coach sampling loop with its 1,000-attempt limit. Missing
reporters keep all four -1 fields. The eligibility, ability threshold, recipient
channels, repeated pointer loads and callback mutations retain their original
order. Registration and notice helpers preserve every guest access, scratch
layout, ABI residue and floating status; they do not reinterpret original code.

The three message renderers retain their separate 528/928/7,120-byte frames,
signed low-byte view selection, full field-query order, name styles, personal
versus general templates, scratch buffers and optional links. Primary brief
view still queries field six twice, including live changes between the calls.
Unsupported primary types retain the existing scratch text. Training reporter
checks compare exactly against -1, whereas fitness reporter checks use the
original signed >-1 condition. Club/nation gender bytes, signed IDs/remainders,
below-frame argument writes, callback mutations and live table reloads remain
unchanged. No invented ID bounds, null guards or default messages are added.

All 5,916 original AWOL text bytes and their pointer are now pinned. Private
fixture emission advances by each table's page-rounded size, placing the FP
block after the complete text without overlap. An independent emitted-payload
audit reconstructs the initializer's writes and checks both original hashes.

The final two event bodies retain their 176/384-byte frames, constructor-before-
validation order, signed staff checks and live lookups. Training absence keeps
both mood/contract-flag conversion paths, sixteen-byte event append, byte counter
wrap and signed fitness reduction. Fitness concerns retain the twenty-byte
append, ten-field notice, missing-coach fields and original retry loop, including
matching budget exhaustion when no reporter is found. Both preserve manager
eligibility, decoded attributes, eleven additional inline draw variants and both
discipline actions. Register reuse, distinct scratch layouts, correction-register
variants, null-profile mutation faults and callback ordering remain observable.

## Direct evidence

The isolated Release component check, linked against the accepted 735-entry
libraries, passes **1,819 comparisons**: 991 complete agreements, 786 matching
interrupted/faulted outcomes and 42 rejected negatives. It observes every one of
the **6,958 indexed sites**. Ninety-one comparisons use explicit structural controls;
these are component evidence, not natural gameplay observations.

The daily-only matrix adds 455 comparisons: 297 complete agreements, 156 matching
interrupted/faulted outcomes and two rejected negatives, with 54 structural
cases. It covers every one of the 2,044 new sites. Independent expectations
check direct versus adjusted expiry dates, single-item removal, flag isolation,
older-peer contract changes and the nonzero-draw unapproved-return decrements.
Fixtures include all three lists, both record modes, signed trait/date/count
boundaries, live list growth, repeated records, null lookup/profile/iterator,
callback profile mutation, relocated TOC and shared-pool reseeding. Wide and
redundant-zero guard cases are explicitly identified as structural controls.

The registration/training-notice matrix adds 222 comparisons: 112 complete
agreements, 106 matching interrupted/faulted outcomes and four rejected
negatives, with five structural cases. It covers all 762 new sites, including
late pool wraps, zero and wide peer bounds, the alternate reporter branches and
the exhausted coach search. Independent expectations check flag isolation,
record append/date calculation, three percentage reductions, a selected peer
relationship update, message type and manager/assistant/coach/missing reporters.
Boundary fixtures include interrupted calls, sampled missing bytes and measured
budget stops. Live profile/manager mutations, signed metric/selector/ID limits,
relocated TOC and complete registration-to-notice calls are included.

The three-renderer matrix adds 465 comparisons: 231 complete agreements, 228
matching interrupted/faulted outcomes and six rejected negatives. It covers
all 1,036 renderer sites without explicit structural controls. Independent
expectations check every selected template family, retained unsupported-type
text, repeated/live field-six reads and the distinct negative-reporter rules.
Fixtures include personal/general and brief/full views, every selector band,
missing reporters/context/club, signed ID and gender boundaries, live table
relocation, output aliases, changed message fields and relocated TOC. Selected
complete paths also compare measured budget cuts, sampled missing bytes and
each dependency interruption.

The final two event operations add 238 comparisons: 137 complete agreements,
97 matching interrupted/faulted outcomes and four rejected negatives, with
22 structural cases. All 1,525 new sites are covered, plus selected complete
prepared children. Independent expectations check primary conversion, training
flag/date/record effects, fitness subtraction, byte wrap, both discipline actions,
fitness notice type/missing reporter fields and the original retry budget stop.
Fixtures include both conversion flags, signed mood/traits/IDs, live lookup
changes, profile mutation, missing manager/profile, every late draw wrap and
relocated TOC. Shared draw extensions retain the previously validated default
register/scratch behavior; the complete 21-operation component matrix passes.

Renderer dependency services in the component matrix are stateful fixtures.
Their name, formatting, output and link effects and input/output ABI are compared.
The combined actual-world matrix below separately exercises original language,
message and rich-text services. Natural management-UI traversal is not claimed.

Comparisons cover full register/temporary/mapped-memory and allocator/import
state, ordered effects, dependency ABI, source order, instruction accounting and
archive input/output bytes. Representative budget cuts, four sampled missing
bytes per selected fixture and selected dependency interruptions remain focused.
Controlled constructor-to-reader, status-to-removal and recursive-sampling cases
execute complete original and C++ components through their dependency boundary.

Eight source-guard tests pass, including complete boundaries, metadata mutations,
independent reference labels, table/pointer/padding mutations, absence of an
instruction replay path, unique ownership and productive hook placement.
An explicit reversed-contract negative verifies that the index scan still
rejects external entrances into the highest-address body independently of
contract insertion order.

```sh
python3 -m unittest discover -s tools -p test_awol_records_audit.py -v
```

Private original references, component compile/link recipes and receipts remain
under `.local/awol-reconstruction`. Source-guard tests requiring the original
private input skip when it is absent. No original game code, data, snapshots or
saves are published.

## Combined integration milestone

The candidate appends all 21 entries after the exact accepted 735-entry prefix.
Full generation retains 1,953,334 instructions, 1,922 pages and 1,923 C++ files,
with no missing or pending targets. Exactly six generated pages change:
`page_10003.cpp`, `page_10004.cpp`, `page_10006.cpp`, `page_10007.cpp`,
`page_10008.cpp` and `page_10009.cpp`. Original bodies and global, per-entry and
profiling controls remain available. Eight source/hook/ownership guards and
59 focused historical manifest methods pass. Historical test edits change only
the current total and bound the preceding daily-controller suffix.

The consistent Release build passes the focused AWOL CTest. Actual-world
validation retains all 153,796 staff and prepares missing management services
with their original constructors and publication sequences. A selected real
player and club gain their contract through the original population helpers;
the checks use original list allocation, message builders, rich-text output and
native private-file services. No gameplay dependency is stubbed.

All 21 roots pass **189 paired comparisons**: 105 ordinary completions,
63 budget stops and 21 discriminating negatives, plus **21 separate matching
pointer faults**. Direct, generated and CFM paths, global/per-entry original
controls, complete state, ordered effects, import ABI, file bytes and instruction
accounting agree. Runtime witnesses reach **2,057 source sites**; full-body
coverage comes from the independent component matrix. This is prepared
actual-data coverage, not natural career-UI traversal. Another **42 profiling
controls** keep the selected roots original. The existing daily injury seam
passes eleven paired cases plus its separate pointer fault.

Archive setup preserves the original rejection of duplicate component names by
using a populated load archive and a separate empty original header for saves.
Interrupted comparisons release outstanding original native write forks before
restoring the same private baseline for the modern run. This host cleanup does
not access guest memory or add guest imports; guest state and effects remain
fully compared. Four interrupted native forks exercise that cleanup.

The focused sanitizer build instruments all fifteen semantic units, the adapter,
runtime diagnostics, both check harnesses and the independent reference. Its
component matrix passes all 1,819 comparisons and 6,958 sites; the actual-world
matrix repeats all 189 comparisons and 21 pointer faults. Both runs finish with
empty sanitizer stderr, and all twenty source identities and ten shared/new
header identities are verified. Unchanged dependencies and generated pages
remain Release. This is not a whole-engine sanitizer claim.

The strict-signed arm64 application is installed with executable SHA-256
`626df9fa62ec13a0ebd5a3c19f96d0b60e996eb40df88872fdb563d8d71b61d2`.
Candidate and installed identities and all four original payload files agree.
The installed process visibly reaches **Preparar Jogo**, displays the original
quit confirmation and exits normally. Its exact process identity and native
normal-exit log are retained. Startup smoke is separate from the dedicated
actual-world checks and does not establish natural career traversal.

The installation preserves all 846 protected runtime/handoff files, including
the user's careers and eighteen existing private feature prompts. The previous
735-entry application is retained as a private reference; the installed Match
Debugger is unchanged. Original inputs, snapshots, comparison recipes, source
identities and installation receipts remain private under
`.local/awol-reconstruction`.
Seven reproducible linked check executables were removed after acceptance,
releasing 1,018,839,712 logical bytes. Their objects, libraries, build recipes,
logs and original references remain available for relinking.
Save/Load: **not retested**. Performance: **not benchmarked**. The complete-game
reconstruction goal remains active.
