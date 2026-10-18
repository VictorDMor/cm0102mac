# Staff contract queries

All three complete staff-contract lookup operations have prepared C++23
implementations and passing independent original comparisons. They cover 145
indexed instructions on two original pages. Production hooks, actual-world
runtime, sanitizer and installation acceptance remain pending for the coherent
contract integration batch. The accepted manifest and installed game remain at
**756 entries**.

## Original boundaries

| Entry | Exclusive end | Instructions | Operation |
| --- | --- | ---: | --- |
| `0x10082f20` | `0x10082f84` | 25 | Find the staff member's primary contract |
| `0x10082f90` | `0x10083108` | 94 | Find the contract for a requested or resolved club |
| `0x10083110` | `0x10083178` | 26 | Find the staff member's secondary contract |

The original index has 891 incoming root edges: 653, 149 and 89 respectively.
No indexed external edge enters a body interior. Five zero words between the
bodies are outside their boundaries. Every instruction and page fragment has a
pinned fingerprint. All three complete bodies were checked against existing
semantic ownership before implementation; none overlaps an accepted owner.

## Preserved behavior

Staff statistics and contract records both use 100-byte strides. The first two
statistics words select the primary and secondary contract indices. A null
staff pointer, an ID failing the original signed staff-count comparison, or a
negative selected contract index returns null. The original does not impose a
separate nonnegative staff-ID check. Index multiplication and pointer addition
retain 32-bit wraparound, including large positive indices.

The club query requires both staff and requested club pointers and a primary
contract. It returns a matching primary contract before considering the
secondary one. When neither matches, it calls the original club resolver at
`0x100ce320`, passing a stack-byte output and flag 1. A nonnull return replaces
the requested club only when that output byte is zero. The query then checks
primary and secondary club IDs again, preserving their priority and live reads.
Missing primary contracts do not fall back to a secondary-only lookup.

The implementation retains repeated staff-ID/count reads, signed comparisons,
stack/register effects and reads after dependency calls. Tests exercise live
staff-ID mutation, contract mutation by the resolver, requested-club aliases,
an alias between the requested club and the resolver's stack-byte output, and
relocated TOC. Redundant original branches remain represented and receive
explicit structural coverage.

Contract creation, slot reservation, binding, negotiation, wages, transfers,
expiry and the original club resolver are outside this lookup block. No game
rule was changed.

## Evidence and integration boundary

The focused Release CTest passes in 1.65 seconds with **2,738 comparisons**:
1,614 complete comparisons, 1,120 interrupted or faulted comparisons, and four
discriminating negatives. The matrix covers every one of the **145 source
sites**. There are 101 comparisons with explicit structural controls; these
are component evidence, not natural runtime traversal.

Comparisons include complete architectural and generated-temporary bytes,
mapped memory, allocator/import state, ordered effects, dependency input/output
ABI, source order and instruction accounting. Only completion, guest budget
exhaustion, an explicit dependency interruption and an unmapped guest-memory
fault are valid outcomes; fixture assertions cannot count as matching faults.
The tests also check independent signed-index arithmetic and club-selection
results. The secondary lookup executes complete original/C++ bodies within
the club query; the external club resolver is a stateful component fixture
with byte-output, caller-clobber and live-mutation behavior.

Eight Python source guards pass, covering complete bodies/fragments, metadata
mutation, missing/duplicate/reordered bodies, external interior edges, a
reversed-contract edge scan, independent reference completeness and ownership.
References contain all original labels and steps with semantic hooks disabled.
Named C++ queries do not replay generated instruction bodies.

The exact accepted 756-entry prefix remains unchanged, with canonical SHA-256
`227fca872a4b07455474488cab1d1d46728cd5bed1cbc2aa82e71ca0b3cb98f9`.
The new CMake target compiles these functions only into the component check;
they are not added to the production library or manifest. No generated game
pages or installed application were replaced during this preparation.

Original instruction data, generated references and execution receipts remain
private under `.local/contract-query-reconstruction`. Save/Load: **not retested**.
Performance: **not benchmarked**. The full reconstruction goal remains active.
