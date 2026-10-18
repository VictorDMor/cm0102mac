# Club shortlist storage, reset and persistence

Seven complete operations from the original `SHORTLIST_MANAGER.CPP` are
prepared and compared independently. They cover **1,945 indexed instructions**.
They have no production adapter or replacement ownership yet: the accepted
installed application remains at **832 entries**. A later integration milestone
must validate the real service composition before adding these seven owners.

## Complete boundaries

| Entry | Exclusive end | Instructions | Operation |
| --- | --- | ---: | --- |
| `0x10356410` | `0x103567a8` | 230 | `construct_shortlist_storage` |
| `0x103567b0` | `0x103568d4` | 72 | `destroy_shortlist_storage` |
| `0x103568e0` | `0x10356e3c` | 342 | `reset_shortlist_storage` |
| `0x10356e40` | `0x103573b8` | 350 | `sample_shortlist_range` |
| `0x10358020` | `0x10358af0` | 690 | `load_shortlist_storage` |
| `0x10358af0` | `0x10358b24` | 13 | `initialize_auxiliary_record` |
| `0x10358b30` | `0x10358f14` | 248 | `save_shortlist_storage` |

The index has 24 edges targeting these roots, including calls within this
block, and no external body-interior edges. Five nonindexed padding words are
separately pinned to `60000000`; they do not count as indexed source sites.
The neighboring `0x103563e0` date helper is already owned by engine
initialization and is excluded. The absence service at `0x100031b0` and
`0x10003dc0` is already owned by AWOL and is also excluded.

## Storage and construction

The 224-byte object contains two global pair-array pointers at +0/+4, their
counts at +8/+12/+16/+20, two banks of sixteen category pointers at +24/+88,
sixteen counts at +152, maximum category capacity at +216 and a club-table
pointer at +220. Category meanings and complete field labels remain open.

The club table has twelve bytes per club. Each entry points to forty 44-byte
records; it also stores a signed halfword copied from club+0x1ec and two
initialized bytes. Construction writes record kind 11, a zero halfword, zero
value words at +4/+8/+12 and identity sentinels -1 at +16/+20/+24. It sets
selected bits at +40, byte +36 and a five-byte date at +28. Other destination
bytes and bits retain their previous values. The date dependency is called
with the original arguments 1 and 1997; no new calendar interpretation is
introduced.

With an archive argument, construction invokes the reader instead of filling
new club records. It returns the object without introducing a success check.
Fresh construction clears the global arrays and invokes the reset operation.
Destruction frees each per-club pointer, the club table, both category banks
and both global arrays in original order. A positive signed low halfword of
the deletion flag also deletes the object. Null-object behavior is preserved.

## Rebuilding category lists

Reset frees both category banks, obtains the largest eligible count among
sixteen categories by calling the external membership predicate, allocates
both banks at that capacity and clears their counts. It then processes staff
only while `index < staff_count - 16`. Thus capacity includes the last sixteen
staff while population excludes them. This difference is retained; its
intent is not established by this block. The incoming r4 flag is unused.

Each twelve-byte category entry stores a staff index, a career value from
career+0x68 and a third value. The first bank gets staff+8; the second gets a
signed contract byte from +0x44, defaulting to 100 without a matching contract.
The primary lookup also updates the original shared contract scratch and
derives the club passed to the second lookup.

A missing staff career can trigger a draw from the career count and writes
the selected career pointer back to staff+12. Zero count selects index zero
without drawing. Narrow bounds retain signed remainder behavior; wide bounds
use the original floating conversion, recursive correction and clamp. Capacity
overruns and allocation failures retain diagnostics and the exit dependency.
If that dependency unexpectedly returns, the original continuation is retained.

## Shared range sampling

The complete 350-instruction sampler has the same relocated control flow as
the accepted contract sampler. Exactly five instructions differ: its table
slot is TOC-0x4ddc and four bias reads use table+8 instead of +0xc0. The divisor
is table+0. `range_sampling_detail::Layout` now has an explicit bias offset
defaulting to +0xc0, retaining the existing contract and offer layouts.
The common semantic-call helper is unchanged. No original body is replayed by
the named implementation.

## Reading and writing

The reader obtains a global pair count, allocates both arrays, reads the first,
copies it through the original copy dependency and sorts the second through
the original comparator boundary. Counts and repeated pointer reads retain
their source order and alias behavior. The CFM constructor and comparator
vectors are pinned alongside the diagnostic strings and both sampling doubles.

Version comparisons are signed. Before `0x3e016`, the reader also consumes and
discards a temporary array of 24-byte legacy records. Versions through
`0x3e016` read fifteen category counts and convert twenty-five records per
present club into forty records. The remaining fifteen receive default fields.
From `0x3e017`, sixteen category counts and forty raw records are read directly.
Before `0x3e017`, each bank skips its sixteenth persisted category, retaining
the original sentinel write through object+0x54, and the reader invokes reset
after loading the club records. Loaded club scores are raised to the current
club score when their signed stored value is lower.

Writing exports the active pair array, sixteen category counts, maximum
capacity, both category banks, the club table and forty records for every
nonnull club entry. Short transfers preserve partial writes/reads, diagnostic
scratch, the error flag and early-return residue. No rollback, format cleanup
or additional null protection is added.

## Acceptance and limits

The independent original references disable every productive semantic hook.
All architectural bytes, generated temporary residues, mapped memory, access
order, source-site order, instruction accounting, dependency register images,
stream bytes and stream position are compared at returns and matching failures.

- **1,536 comparisons**: 553 complete agreements, 969 matching interruptions
  or faults, and 14 deliberately changed negative controls rejected.
- **1,945/1,945 source sites**, excluding budget-exhausted observations:
  1,835 occur in complete returns; 110 are observed in matching error paths.
- 28 comparisons use explicit structural register/condition controls to reach
  deep sampler, constant-guard or wide-career paths. These are not natural
  career-runtime coverage.
- **13 source/data guards** pin complete bodies/fragments, absent candidate
  ownership, original tables/vectors/padding, the five sampler differences,
  all 25 diagnostic blocks and isolated original-reference generation.
- Independent assertions cover forty default records, retained bytes and
  packed bits, signed contract status, the 18-capacity/2-populated example,
  current stream length, copied/sorted arrays and legacy tail initialization.
- The shared sampler change also passed all **5,242 contract-term** and
  **7,559 transfer-valuation** comparisons.

The new CMake target compiles the five semantic translation units only into
the component executable. No new production hooks, generated production
pages, installed executable or protected game assets were changed. Membership,
selection, pricing, scheduling, allocation, date, archive, copy, sort, version,
diagnostic and contract services remain explicit dependency boundaries. Their
fixture models are not claims of reconstructed internals. Full shortlist AI,
natural-career persistence and performance are not established here.
Save/Load: **not retested**. Performance: **not benchmarked**.
