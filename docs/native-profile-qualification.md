# Profile qualifications and category references

Status: **integrated and accepted at the 868-owner milestone**. All four
complete native operations match independent original references and now execute
on the productive path. The exact previous 864-owner prefix is preserved. The
actual-world matrix, completed candidate/recruitment children, focused UBSAN and
protected installation pass. See [integration evidence](native-profile-qualification-integration.md).
The complete-game goal remains active.

## Scope and provenance

| Entry | Exclusive end | Indexed instructions | Native operation |
| --- | --- | ---: | --- |
| `0x10117360` | `0x10117488` | 73 | `combine_qualification_masks` |
| `0x10117490` | `0x10117688` | 126 | `classify_profile_qualifications` |
| `0x1036f670` | `0x1036f6e8` | 30 | `candidate_category_reference` |
| `0x1036f6f0` | `0x1036f7d8` | 58 | `club_category_reference` |

The original index has forty incoming root edges and no external interior entry.
The profile classifier has 36 incoming calls, including two in the already
reconstructed recruitment controller. This establishes dependency reuse, rather
than a claim about measured runtime frequency or performance. The candidate
reference calls both the classifier and the club reference. Mask combination is
called by a separate original path and shares the primary/secondary
qualification representation with the classifier.

All 287 indexed instructions are retained. Nine nonindexed words are pinned
separately: eight zero alignment words and the skipped `0x60000000` NOP at
`0x10117404`. The index has no path into that NOP. Original primary and secondary
mask banks occupy sixteen and twelve bytes; the null-profile diagnostic text
occupies another 47 bytes. Every byte and all three TOC pointers are pinned. No
original bank bytes or text are embedded in the native operations.

## Observed behavior

Mask combination takes two signed 16-bit masks in r3/r4. It copies both original
banks to the stack, finds the last matching primary and secondary indices in
each mask, averages each pair of signed-byte indices toward zero, and combines
the selected bank entries into a signed 16-bit result. Multiple bits therefore
select the last table index, rather than an arbitrary first bit. When a mask
has no matching bit, the original incoming index registers can survive into
the calculation; they must not be silently initialized or normalized. The
component includes those register residues and matching access faults. For
example, masks `0x801` and `0x204` combine to `0x402` with the original banks.

Profile classification takes a profile pointer in r3. Its primary fields use a
threshold descending from fifteen through ten, stopping at the first threshold
that produces any primary bit. Several fields can qualify at that threshold;
the three central fields are tested in the fixed order `+18`, `+20`, `+19` and
only the first matching one contributes. Bytes are signed: a raw value of 255
is negative, not a high rating.

The secondary fields at `+23`, `+24` and `+25` always compare against fifteen.
The original decrements a loop counter when no field qualifies, but its repeated
comparisons keep the fixed threshold. A value of fourteen never becomes a
secondary qualification after another pass. With all primary fields equal to
ten and all secondary fields equal to fourteen, the result is `0x4f`. Raising
the three secondary fields to fifteen produces `0xacf`.

A null profile still performs four diagnostic dependency calls, clears the
original global byte and returns zero if those calls return. Their order,
arguments, stack effects, register residues and interruptions remain part of
the component contract; the null case is not reduced to a C++ early return.

The club reference receives the club in r4 and a category byte in r6. That byte
is sign-extended. Category four compares the club country's raw identifier with
five dynamic globals, then multiplies signed club word `+0x1ec` by 550, 475 or
330. Category five uses 550; category six uses 110. Other signed-byte categories
return `-1`. The result's currency, interface label and real-world country group
names are not inferred from these raw fields.

The candidate reference uses r3 as its retained object argument, r4 as club,
r5 as optional staff and r6 as category. Missing staff or a missing staff
profile returns `-1`. Otherwise it obtains the profile mask and passes it to the
club reference, preserving both calls. The club reference does not consult that
mask in this original body, so the wrapper must not add a qualification filter
or replace the call sequence with a direct multiplication.

## Component evidence

The component performs **3,434 comparisons**: 3,304 complete returns, 122 matching
stops and eight rejected deliberate corruptions.

| Root | Comparisons | Complete returns | Matching stops | Negatives | Complete-return sites |
| --- | ---: | ---: | ---: | ---: | ---: |
| Mask combination | 1,430 | 1,419 | 9 | 2 | 73 |
| Profile classification | 318 | 295 | 21 | 2 | 126 |
| Candidate reference | 299 | 268 | 29 | 2 | 30 |
| Club reference | 1,387 | 1,322 | 63 | 2 | 58 |

All 287 indexed sites occur in complete returns, with no structural exceptions.
Each comparison checks all architectural registers, temporary residues, mapped
memory bytes, ordered reads/writes including instruction PCs, source order,
dependency entry/return ABI and instruction accounting. The independent
reference disables every discovered productive hook and retains each original
instruction. Native operations use named C++ stages and loops, without original
body replay, an instruction interpreter or a PC dispatcher.

The candidate reference also executes both selected native child operations,
compared with their independent original bodies. Separately declared helper
results exercise forwarding. The four diagnostic services use explicit external
models; this is not actual original-service runtime proof.
Successful positive parent comparisons complete 257 calls to each selected
native child. The harness asserts these returns against the original call trace.

Inputs cover every primary/secondary rank pair, multi-bit and missing-bit masks,
incoming register residues, signed bytes and words, primary thresholds and
priority, fixed secondary thresholds, all 256 raw category bytes, all country
comparison groups, missing optional records, measured budget cuts, removed
accessed bytes and interrupted dependencies. Each root rejects an own-result
corruption and an extra memory effect restored before return. The effect
negative requires equal final images, so an image-only comparison cannot pass it.

Ten source/data checks pass, including exact installed-prefix preservation,
changed/missing/reordered bodies, every pinned byte, external interior-entry
rejection, productive hook placement and independent references. The focused CTest
`profile_qualification_component_equivalence` passes on ARM64 with
`-ffp-contract=off`, `-Wall -Wextra -Werror -Wpedantic`.

The cumulative Portuguese guide records this block and practical examples.
The separate consolidated integration proves productive dispatch, prepared
original-world inputs, completed candidate/recruitment children, focused UBSAN
and installation. Actual-world coverage is separate from the isolated 287-site
proof; the null-profile diagnostic branch is not exercised with actual diagnostic
services in this milestone. Save/Load is **not retested**; performance is **not
benchmarked**. The complete-game goal remains active.
