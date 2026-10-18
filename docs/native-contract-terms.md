# Contract terms, presentation and expiry bands

This complete **21-operation, 1,203-instruction** block is implemented and
directly compared with the original. It is **integrated and installed** in the
783-entry game, preserving the exact accepted 762-entry prefix. The
[integration report](native-contract-terms-integration.md) records actual-service,
focused sanitizer, installation and startup acceptance.

## Original scope

| Entry | Exclusive end | Instructions | Operation |
| --- | --- | ---: | --- |
| `0x10374570` | `0x10374ae8` | 350 | Sample a contract range |
| `0x10374af0` | `0x10374b34` | 17 | Enable a clause option |
| `0x10374b40` | `0x10374b54` | 5 | Set a bonus |
| `0x10374b60` | `0x10374b74` | 5 | Clear a clause option |
| `0x10374b80` | `0x10374b98` | 6 | Clear a bonus |
| `0x10374ba0` | `0x10374bd4` | 13 | Reset term flags and markers |
| `0x10374be0` | `0x10374ca4` | 49 | Describe a bonus |
| `0x10374cb0` | `0x10374cd8` | 10 | Describe a clause option |
| `0x10374ce0` | `0x10374d04` | 9 | Query expired kind |
| `0x10374d10` | `0x10374d44` | 13 | Calculate end-date delta |
| `0x10374d50` | `0x10374d74` | 9 | Query rolling kind |
| `0x10374d80` | `0x10374f44` | 113 | Describe contract end |
| `0x10380270` | `0x103802f4` | 33 | Name a bonus |
| `0x10380300` | `0x103803ac` | 43 | Name a clause option |
| `0x103803b0` | `0x10380400` | 20 | Query bonus eligibility |
| `0x10380400` | `0x10380520` | 72 | Query clause eligibility |
| `0x10380520` | `0x10380554` | 13 | Query exact release flags |
| `0x10380560` | `0x10380630` | 52 | Adjust primary numeric value |
| `0x10380630` | `0x10380700` | 52 | Adjust alternate numeric value |
| `0x10380700` | `0x10380708` | 2 | Set release amount |
| `0x10380710` | `0x10380c04` | 317 | Classify expiry band |

All complete bodies and both source pages have pinned fingerprints. Initial
characterization found 146 incoming root edges, no existing semantic owner for
these roots and no external entrance into an interior. Offer construction, negotiation and
the larger satisfaction/relationship routines remain outside this scope.

## Terms and eligibility

The original string table confirms Goal Bonus, Assist Bonus and Clean Sheet
Bonus. Clause indices zero through four are Non Promotion Release Clause,
Minimum Fee Release Clause, Non Playing Release Clause, Relegation Release
Clause and Manager Job Release Clause. These are confirmed labels; they do not
establish all transfer or negotiation behavior associated with each clause.

Enabling index one clears all five option bytes and then enables index one.
Enabling another signed low-byte index clears index one before writing the
selected byte. Thus minimum-fee release is mutually exclusive with the other
four ordinary options. Invalid indices retain the original signed arithmetic
and wrapping writes rather than acquiring new range checks.

Bonuses are four-byte values selected by the signed low-byte index. Clearing a
bonus writes -1. Reset clears the five option bytes and byte `0x54`, then writes
-1 to the three bonuses and word `0x58`; unrelated fields retain their values.
The separate release-amount setter writes word `0x24`. The release query is
exactly `option[0] == 2 || option[1] == 1 || option[3] == 2`, not a test for any
nonzero option.

Bonus eligibility returns zero without a profile. Profile role byte 20 accepts
only bonus index two; other profiles accept every signed-byte index except two.
Clause eligibility resolves the live club/competition table before selecting
the clause. Indices zero and three call distinct competition virtual methods,
including the original CFM ABI and TOC reload. Index one returns true, index two
false, and index four tests the signed low-byte age against 31. Other indices
return false. This standalone query does not add the competition type-byte
guard found in the earlier initializer's inline rule.

## Dates, presentation and numeric adjustments

Kind queries read the signed packed byte at `0x38`: kind three is expired and
kind one rolling. These queries inspect the stored kind, rather than inferring
expiration from today's date. The date-delta operation calls the original date
service and narrows its return to signed 16 bits.

End descriptions retain brief/full selection, original text, live date-helper
results, all twelve computed month cases and the year reread after the date
callback. Changed month-table targets preserve partial-case entry and the
original continuation boundary. Unknown targets resume with the saved caller
and existing link register rather than silently becoming a normal return.

The expiry classifier compares the end date against the current date and seven
live reference dates, retaining unsigned-year/signed-day comparisons, exact
boundary equality and its below-stack cache. It returns codes zero through 14;
rolling and nonfuture paths return 14. Those codes are classification values,
not established durations in days or months. UI meanings for the reference
dates remain outside the prepared component evidence.

Two numeric adjustment operations preserve preference-bit precedence. Bit
`0x800000` selects multiply-by-52, service mode four and signed division by 52;
otherwise bit `0x400000` selects multiply-by-four, service mode eight and signed
rounding back. The direct path uses service mode eight without that scaling.
The two original adjustment services remain distinct. No unverified currency
or user-facing preference name is assigned to these paths.

## Shared random sampling

The complete 350-instruction range sampler retains zero/signed/wide bounds,
single/double rounding, correction draws, recursion, clipping, live pool cursor,
wrap/reseed and shared salt ordering. Structural normalization proves its
original assembly equals the accepted AWOL sampler after local address and TOC
relocation. The two consumed floating constants are byte-identical, including
the relocated conversion bias. The C++ implementation is still compared with
its own complete independent original reference; equality is not inferred from
the earlier sampler's test result.

## Component evidence and limits

The focused Release CTest passes **5,242 comparisons**: **4,438 complete
agreements**, **773 matching interruptions or unmapped-memory faults**, and
**31 rejected own/child negative controls**. The matrix requires all **1,203
original source sites**. Its **25 structural comparisons** are separately
identified and are not natural career traversal.

Each comparison checks complete architectural, temporary, mapped-memory and
allocator state; ordered effects; dependency input/output ABI; source order;
and instruction accounting. Only completion, guest-budget exhaustion, explicit
dependency interruption and unmapped guest-memory faults are valid outcomes.
Fixture assertions cannot count as successful agreement. Representative paths
exercise four budget cuts, six sampled missing-byte boundaries and every
dependency interruption. Own negatives cover every root; child negatives
exercise roots with dependency calls.

Cases cover all byte selectors and packed kinds, upper selector bits, signed
and overflow numeric boundaries, every month, competition/profile/age gates,
exact reference dates, null pointers, callback mutation, relocated TOC and month
tables, aliased output, partial and unknown computed continuations, all reseed
paths and recursive sampling. Independent expectations check minimum-fee
exclusion, reset fields, bonus/release writes, exact release flags, packed kinds,
narrow samples and reference-date band equality. External services remain
stateful fixtures, including date, formatting, adjustment and competition
services. This is not real-world integration evidence.

Twelve Python guards pin the complete bodies, both pages and three original
tables totaling 1,122 bytes with their live pointers. They verify sampler and
numeric-adjustment normalization, reject changed/missing/duplicate/reordered
bodies and partial pages, reject ownership overlap and external interior edges,
require productive hooks before original accounting, and require every
independent reference label/step with hooks disabled. Named
operations do not replay the generated bodies. Compilation uses
`-ffp-contract=off` and treats warnings as errors.

The initial competition fixture omitted one table indirection and faulted in
both versions. Its completion assertion correctly rejected that run; the fixture
was corrected before the accepted matrix. Earlier incomplete coverage runs are
diagnostic history, not accepted evidence. Logs, original data and generated
references remain private under `.local/contract-terms-reconstruction`.

Production adapters, manifest selection, consistent generation, actual-service
comparisons, focused sanitizer acceptance and installation pass in the combined
integration milestone. The component services described above remain fixtures;
actual-service evidence is reported separately. Save/Load is **not
retested**. Performance is **not benchmarked**. The cumulative Portuguese guide
is updated for this cycle. The whole-game goal remains active.
