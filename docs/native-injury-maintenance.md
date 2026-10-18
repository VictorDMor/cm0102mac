# Injury physical maintenance: ownership correction

These five operations and the seven in the
[fitness and randomness exercise](native-injury-fitness.md) were already accepted
under `actor_attributes_adapter`. The separately prepared implementations did
not extend the 734-entry manifest. Duplicate ownership was caught before
production generation or installation; the redundant preparation files have
been removed and the accepted implementations remain unchanged.

| Entry | Exclusive end | Indexed instructions | Existing operation |
| --- | --- | ---: | --- |
| `0x10186a20` | `0x10186cd0` | 172 | Assign capacity and condition increment |
| `0x10186cd0` | `0x10186e0c` | 78 | Merge pending condition update |
| `0x10186e10` | `0x10186e94` | 32 | Find pending condition update |
| `0x10186ea0` | `0x10186fb4` | 69 | Reset condition record |
| `0x10186fc0` | `0x10187450` | 292 | Apply match condition effects |

The redundant maintenance exercise covered 643 indexed instructions and pinned
two padding no-ops. It contributed 304 comparisons: 133 complete agreements,
161 matching interrupted or faulted outcomes and ten rejected negatives. Together
with the seven fitness/randomness operations, historical component evidence
covered 2,018 indexed sites in 894 comparisons. Those totals add no new
reconstructed entries and are not natural gameplay coverage.

The genuinely new operation is the
[daily injury controller](native-injury-daily-controller.md) at
`0x10185410..0x101864a0` (1,060 instructions). It reuses accepted dependencies and
must preserve the exact 734-entry prefix and unique ownership. Private historical
sources and receipts are retained in `.local/`.

Save/Load: **not retested**. Performance: **not benchmarked**. Complete-game
reconstruction remains active.
