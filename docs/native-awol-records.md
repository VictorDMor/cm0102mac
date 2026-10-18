# AWOL records, daily review and supporting sampling

## Status and complete module boundary

The original `AWOL.CPP` source block contains 21 complete operations and 6,958
indexed instructions at `0x100031b0..0x10009ee8`. No indexed external edge enters
the interior of any operation. Eleven non-indexed padding no-ops are identified.
The following CASH string helper remains outside this source block.

Fourteen operations now have prepared C++23 implementations and passing direct
comparisons. They cover 3,635 indexed instructions. **Production integration is
pending** until the remaining seven operations complete the coherent AWOL batch.
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

## Direct evidence

The isolated Release component check, linked against the accepted 735-entry
libraries, passes **894 comparisons**: 511 complete agreements, 355 matching
interrupted/faulted outcomes and 28 rejected negatives. It observes every one of
the **3,635 indexed sites**. Sixty-four comparisons use explicit structural controls;
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

Comparisons cover full register/temporary/mapped-memory and allocator/import
state, ordered effects, dependency ABI, source order, instruction accounting and
archive input/output bytes. Representative budget cuts, four sampled missing
bytes per selected fixture and selected dependency interruptions remain focused.
Controlled constructor-to-reader, status-to-removal and recursive-sampling cases
execute complete original and C++ components through their dependency boundary.

Six source-guard tests pass, including complete boundaries, metadata mutations,
independent reference labels, table/pointer/padding mutations, absence of an
instruction replay path, and rejection of overlap with already accepted owners.
An explicit non-sorted-contract negative verifies that the index scan still
rejects external entrances into the higher-address notice body after appending
the lower-address daily root.

```sh
python3 -m unittest discover -s tools -p test_awol_records_audit.py -v
```

Private original references, component compile/link recipes and receipts remain
under `.local/awol-reconstruction`. Source-guard tests requiring the original
private input skip when it is absent. No original game code, data, snapshots or
saves are published.

## Remaining coherent batch

The remaining seven operations contain 3,323 instructions: registration
(`0x10006900`), and the record/event/message bodies at
`0x100072e0`, `0x10007a50`, `0x10008770`, `0x10008b60`, `0x100090a0` and
`0x10009b60`. Complete instruction boundaries and call dependencies are recorded;
remaining semantic/table characterization, implementation and direct comparisons
are pending.

Production hooks, generation/build, actual runtime, sanitizers and one installed
application smoke are deferred to the combined AWOL integration milestone.
Save/Load: **not retested**. Performance: **not benchmarked**. The complete-game
reconstruction goal remains active.
