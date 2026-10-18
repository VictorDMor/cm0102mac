# Injury fitness and supporting randomness: ownership correction

All seven original operations below were already accepted under
`actor_attributes_adapter` in the 734-entry production manifest. The separate
implementations published in commits `4354d250` and `498cc2bb` were redundant
preparation work. They added no new reconstructed entries and were never hooked
into the production game.

| Original entry | Exclusive end | Instructions | Existing operation |
| --- | --- | ---: | --- |
| `0x10187450` | `0x10187644` | 125 | Maintain condition scalar |
| `0x10187650` | `0x10187680` | 12 | Read signed condition scalar |
| `0x10187680` | `0x101877dc` | 87 | Read injury recovery phase |
| `0x101877e0` | `0x101878d0` | 60 | Read raw or percentage capacity |
| `0x101878d0` | `0x10187a1c` | 83 | Read effective condition |
| `0x10187a40` | `0x10188488` | 658 | Refresh condition and training recovery |
| `0x101864a0` | `0x10186a18` | 350 | Sample a signed bounded value |

The redundant exercise covered 1,375 original instructions in 590 direct
comparisons: 253 complete agreements, 324 matching interrupted or faulted
outcomes, and 13 rejected negatives. These are historical component results,
not additional production coverage or a new installed-game delivery.

The duplicate sources, reference generators, audits and tests have been removed.
The accepted implementations and owners remain unchanged; the
[daily controller](native-injury-daily-controller.md) reuses them. Generation now
rejects duplicate semantic entry ownership before emitting any pages. The direct
daily checks also execute the accepted sampling and refresh implementations
against their independent original references.

Private historical sources and evidence remain in `.local/`; no original game
code or data is published. Save/Load: **not retested**. Performance: **not
benchmarked**. Complete-game reconstruction remains active.
