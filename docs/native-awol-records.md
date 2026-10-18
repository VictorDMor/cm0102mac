# AWOL records, daily review and supporting sampling

## Status and complete module boundary

The original `AWOL.CPP` source block contains 21 complete operations and 6,958
indexed instructions at `0x100031b0..0x10009ee8`. No indexed external edge enters
the interior of any operation. Eleven non-indexed padding no-ops are identified.
The following CASH string helper remains outside this source block.

Nineteen operations now have prepared C++23 implementations and passing direct
comparisons. They cover 5,433 indexed instructions. **Production integration is
pending** until the remaining two operations complete the coherent AWOL batch.
The accepted manifest and installed game remain at 735 entries. Every prepared
entry was checked to have no accepted semantic owner before implementation;
existing actor/condition implementations remain unchanged.

## Prepared operations

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

## Direct evidence

The isolated Release component check, linked against the accepted 735-entry
libraries, passes **1,581 comparisons**: 854 complete agreements, 689 matching
interrupted/faulted outcomes and 38 rejected negatives. It observes every one of
the **5,433 indexed sites**. Sixty-nine comparisons use explicit structural controls;
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

Renderer dependency services are stateful component fixtures. Their name,
formatting, output and link effects and input/output ABI are compared, but this
does not claim real language-service/layout acceptance or natural management-UI
traversal. Those remain part of the complete AWOL actual-runtime milestone.

Comparisons cover full register/temporary/mapped-memory and allocator/import
state, ordered effects, dependency ABI, source order, instruction accounting and
archive input/output bytes. Representative budget cuts, four sampled missing
bytes per selected fixture and selected dependency interruptions remain focused.
Controlled constructor-to-reader, status-to-removal and recursive-sampling cases
execute complete original and C++ components through their dependency boundary.

Seven source-guard tests pass, including complete boundaries, metadata mutations,
independent reference labels, table/pointer/padding mutations, absence of an
instruction replay path, and rejection of overlap with already accepted owners.
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

## Remaining coherent batch

The remaining two operations contain 1,525 instructions: the record/event
bodies at `0x10007a50` (840 instructions) and `0x100090a0` (685 instructions).
Complete instruction boundaries and call dependencies are recorded;
remaining semantic/table characterization, implementation and direct comparisons
are pending.

Production hooks, generation/build, actual runtime, sanitizers and one installed
application smoke are deferred to the combined AWOL integration milestone.
Save/Load: **not retested**. Performance: **not benchmarked**. The complete-game
reconstruction goal remains active.
