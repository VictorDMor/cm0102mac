# AWOL records and supporting sampling

## Status and complete module boundary

The original `AWOL.CPP` source block contains 21 complete operations and 6,958
indexed instructions at `0x100031b0..0x10009ee8`. No indexed external edge enters
the interior of any operation. Eleven non-indexed padding no-ops are identified.
The following CASH string helper remains outside this source block.

Thirteen operations now have prepared C++23 implementations and passing direct
comparisons. They cover 1,591 indexed instructions. **Production integration is
pending** until the remaining eight operations complete the coherent AWOL batch.
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

## Preserved contracts

Construction keeps the original partial-load fallback: it frees the flag buffer
and primary list before recreating storage, without inventing cleanup of other
partially loaded lists. Allocation failures preserve the original subsequent
guards or faults. Destruction retains signed 16-bit narrowing of its delete flag.

Archive operations preserve short reads/writes, partial effects, signed counts,
the numeric version comparison at `0x3e005`, 12/16/20-byte record layouts, scratch
areas, diagnostics and close ordering. Iterator counts and pointers are reloaded
in their original order, including mutations made by dependencies. The six
padding words within these prepared bodies and the original diagnostic text
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

## Direct evidence

The isolated Release component check, linked against the accepted 735-entry
libraries, passes **439 comparisons**: 214 complete agreements, 199 matching
interrupted/faulted outcomes and 26 rejected negatives. It observes every one of
the **1,591 indexed sites**. Ten comparisons use explicit structural controls;
these are component evidence, not natural gameplay observations.

Comparisons cover full register/temporary/mapped-memory and allocator/import
state, ordered effects, dependency ABI, source order, instruction accounting and
archive input/output bytes. Representative budget cuts, four sampled missing
bytes per selected fixture and selected dependency interruptions remain focused.
Controlled constructor-to-reader, status-to-removal and recursive-sampling cases
execute complete original and C++ components through their dependency boundary.

Five source-guard tests pass, including complete boundaries, metadata mutations,
independent reference labels, table/pointer/padding mutations, absence of an
instruction replay path, and rejection of overlap with already accepted owners.

```sh
python3 -m unittest discover -s tools -p test_awol_records_audit.py -v
```

Private original references, component compile/link recipes and receipts remain
under `.local/awol-reconstruction`. Source-guard tests requiring the original
private input skip when it is absent. No original game code, data, snapshots or
saves are published.

## Remaining coherent batch

The remaining eight operations contain 5,367 instructions: daily processing
(`0x10004070`), registration (`0x10006900`), and the record/event/message bodies at
`0x100072e0`, `0x10007a50`, `0x10008770`, `0x10008b60`, `0x100090a0` and
`0x10009b60`. Complete instruction boundaries and call dependencies are recorded;
remaining semantic/table characterization, implementation and direct comparisons
are pending.

Production hooks, generation/build, actual runtime, sanitizers and one installed
application smoke are deferred to the combined AWOL integration milestone.
Save/Load: **not retested**. Performance: **not benchmarked**. The complete-game
reconstruction goal remains active.
