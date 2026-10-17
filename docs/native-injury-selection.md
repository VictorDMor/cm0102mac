# Injury selection, classification and current-record lookup

This cohesive block reconstructs three complete original operations in
`native/actor_injury_selection.cpp`, extending the accepted semantic manifest
from 701 to 704 entries while preserving its exact prior prefix:

| Entry | Exclusive end | Indexed instructions | Operation |
| --- | --- | ---: | --- |
| `0x10188930` | `0x10188cbc` | 226 | Select an injury from a live category group |
| `0x1018c180` | `0x1018c198` | 6 | Read an injury's category byte |
| `0x1018c340` | `0x1018c448` | 66 | Refresh a player record and return its current injury |

All indexed incoming edges enter the roots. The selector has four original BL
callers, classification has seventeen, and lookup has fifty-two. No indexed
external edge enters an interior. Each complete body and page fragment is
fingerprinted independently. The unindexed alignment word at `0x10188a64` is
pinned to `0x60000000` and is neither executed nor counted as a source instruction.

The TOC slots `0x108b55bc` and `0x108b55c0` point to the original injury details
at `0x108cb8e0` and category groups at `0x108cbcb8`. All 984 detail bytes and
36 group bytes are pinned. Runtime reads still follow the live pointers and
bytes; the pinned data is an audit, not a replacement for guest reads. No range,
null, category or injury-index checks are added beyond those in the original.

The selector first queries the current injury. Types 0–4 and 76–81 permit a new
selection; any other type returns its raw unsigned byte immediately. The previous
injury contributes only when its category matches the signed requested category.
Group start/end and detail severity/weight bytes retain their signed narrowing.
Severity -1 accepts every candidate; a detail severity of -1 is a wildcard;
requested severity 3 skips candidates. The inclusive group end is read again on
every iteration, including after a reseed callback.

Each eligible candidate receives an original signed score. Repeated injuries use
a modulo-three draw minus one; other candidates use their signed weight, with
zero weight consuming no draw. Strictly smaller scores replace the current
choice, preserving the first candidate on ties. All cached pointers, cursor/end
comparisons, intermediate cursor writes, wrap/reseed calls and salt reads keep
their original order. Signed overflow, negative remainders and byte narrowing
remain observable. The preserved scaled floating-point/sampler path requires
explicit structural controls because an ordinary signed-byte weight cannot
reach its wide-bound branch. Those controls are reported separately.

Classification ignores the incoming controller and uses the signed low byte of
the injury type to address a 12-byte detail row. It returns the category byte
unsigned, without a frame or dependency call. Lookup retains the original
112-byte frame, individual register-save order and null-player diagnostics. Its
signed upper-bound guard has no lower-bound check. After the accepted daily
refresh dependency returns, lookup rereads the player's ID, controller record
table and injury base. For an in-range record, injury index -1 selects the
explicit null result; other indices retain the original wrapping pointer
calculation without an added validation step.

## Accepted evidence

Full generation retains 1,953,334 original instructions across 1,922 pages, with
no missing or pending targets. Only `page_10188.cpp` and `page_1018c.cpp` gain
hooks among 1,923 generated C++ files. Each hook precedes source accounting;
complete original bodies remain available for explicit controls. Ten focused
source/scope/reference/table/hook audits and 50 historical manifest-only methods
pass. The 48 historical test-file changes update only current manifest totals.

Release and affected-unit UBSAN each pass **641,223 component comparisons**:
16,570 completions, 624,612 interruptions and 41 discriminating own/child
negatives. All **4,405 original sites** across the 22-entry actor/condition owner
are covered. This block adds **154,601 comparisons**, including 2,625 completions,
five negatives and all 298 new source sites. Of the 41,375 aggregate structural
cases, 4,975 belong to the new wide-weight and redundant-guard controls; these
are distinct from ordinary gameplay inputs.

Complete independent original references compare every register and temporary,
mapped memory, ordered read/write effect, dependency input/output ABI, source
order and instruction count. Independent expectations additionally check signed
classification indexing, unsigned category results, blocked current types,
strict first-candidate ties, repeated-injury scores and live record reloads.
Fixtures cover every type byte, category upper-bit encodings, signed severity and
weight boundaries, zero weights, empty and signed groups, negative salts and
remainders, cursor overflow/wrap/reseed, relocated TOC, invalid indices, null
pointers and callback mutation. Representative paths compare every budget cut,
each individually missing accessed byte and every dependency interruption.

The actual-data proof executes original BLs at `0x1018736c`, `0x10188a18` and
`0x1018897c`, using a captured real player/controller, live original tables and
original reset/registration preparation. These are controlled callsite runs,
not traversal of every complete management-screen parent. Fifteen inputs cover
the three roots, repeated injuries, each ordinary severity and severity 3,
ordinary/repeated wrap, the final group, final/negative category indices and
active/guarded lookup. They add **252 production comparisons** through direct,
generated and CFM paths, global/per-entry original controls, budgets and own
negatives. Classification ignores its first argument and is not assigned a
spurious null-controller-fault requirement.

The aggregate Release/UBSAN production matrix passes **994 comparisons**:
590 returns, 404 budget stops and 59 negatives, plus nineteen separate null-first-
argument fault comparisons. **177 original-profile controls** pass in Release.
The selected root uses its original body in original/profile controls.

A paired **798-advance complete match** preserves full state, imports and
per-tick instruction accounting. The current-injury lookup executes naturally
**13,777 times**, with identical original and C++ calls/returns. Selection and
classification have zero natural calls in that stepper; their dedicated coverage
comes from the controlled actual-data BLs and prepared variants. The accepted
post-match scenario also retains real history-file effects while calling the
new selector and its dependencies. Earlier actor/condition scenarios still pass.

**861 nonlocal comparisons**, including **492 restore/handler faults**, pass in
Release and UBSAN. This block adds 119 comparisons covering saves/frame writes,
live table/pool/record reads and writes, both memory channels where applicable,
and interrupted selector-to-lookup/classification calls. Classification has no
writes, so its two read boundaries use the read-capable observer. Parent and
child scopes both abandon their interrupted host continuations. Full state,
effects, actual import ABI, measured accounting, original-context and turbo
controls agree, with no false return or unknown transfer.

All four focused CTests pass. Sixteen affected units carry UBSAN instrumentation;
all three complete sanitizer runs have empty stderr and recorded final-source
hashes. Remaining archives/generated pages are consistent Release builds. This
is not a whole-engine sanitizer or performance benchmark.

## Installed game and retention

The signed ARM64 game is installed with executable SHA-256
`65a7bdd7fcd78afca56cf8e3d84fea15430176dc1dd14e1dda734454764efe81`.
Candidate and installed identities and the four original bundled payloads agree.
Prepare Game rendering, the original exit confirmation and normal exit pass.
Startup smoke is separate from the dedicated runtime evidence above.

All 845 protected original/save/handoff files, the previous installed-game
reference and installed Match Debugger remain unchanged. Cleanup removes only
six reviewed linked Release/UBSAN checks (867,934,680
logical bytes), retaining apps, objects, archives, references, snapshots, recipes
and evidence. Transparent APFS compression verifies and preserves
135 selected snapshots' ordinary-read SHA-256,
permissions and modification times at the same paths. Their logical contents
remain 17,140,660,830 bytes; allocated storage decreases
by 13,663,543,296 bytes. Protected originals and saves
are outside these targets. Free space afterward is
78,483,574,784 bytes.

Save/Load: **not retested**. Performance: **not benchmarked**. The full conversion
goal remains active; 704 accepted entries are an integration count, not a
completion percentage.
