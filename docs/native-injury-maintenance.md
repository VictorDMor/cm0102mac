# Injury physical maintenance

## Status

Five complete injury-maintenance operations now have named C++23 implementations
and passing direct comparisons with their original bodies. They extend the
prepared [fitness and randomness batch](native-injury-fitness.md) to **12 roots
and 2,018 original instructions**. Production integration is pending. The
accepted manifest and installed game remain at 734 entries.

This component follows the requested larger-batch cadence: retain direct
comparison for changed code, then perform production generation, integration,
sanitizers, installation and visual verification together after the related daily
injury controller is complete.

## Complete operations

| Entry | Exclusive end | Indexed instructions | Operation |
| --- | --- | ---: | --- |
| `0x10186a20` | `0x10186cd0` | 172 | Apply staff fitness and a signed condition increment |
| `0x10186cd0` | `0x10186e0c` | 78 | Merge pending injury type and severity by staff ID |
| `0x10186e10` | `0x10186e94` | 32 | Find a pending injury by staff ID |
| `0x10186ea0` | `0x10186fb4` | 69 | Remove pending injuries and reset physical state |
| `0x10186fc0` | `0x10187450` | 292 | Apply physical effects for both match rosters |

These bodies contain 643 indexed instructions. Two additional original words at
`0x10186d24` and `0x10186e4c` are `0x60000000` padding no-ops. Neither has an indexed
incoming edge or indexed fallthrough. Their raw words are pinned rather than
silently treated as missing executable instructions. There are no indexed
external entrances into the five function interiors.

## Preserved contracts

Staff fitness writes precede the condition adjustment, including repeated staff
ID and row-pointer loads. The original code narrows the increment to signed 16
bits and selects among floating condition-debt adjustment, direct addition,
bounded random addition and the original upper guard. The unreachable wide
random branch remains implemented and has explicit structural-control evidence.
Floating multiply/subtract retains separate rounding with `-ffp-contract=off`.

Pending injury operations retain allocation and constructor order, iterator
reset, each repeated dependency call, signed byte comparisons, the severity cap
at 100, and the different-type replacement rule. A failed allocation retains the
original subsequent null fault. Iterator marker no-ops execute on initial entry
only; loop back edges retain their original destinations. Reset preserves
removal during iteration, injury/training dependency order, date loads, sentinel
fields and the single physical-metric flag clear.

Match processing visits both 20-slot rosters with the original strides. It retains
starting/substitute minute selection, competition and age-dependent penalties,
random draws and reseeds, date suppression, pending injury iteration, original
classification arguments, and the distinct friendly/competition record calls.
For pending severity at or below 50, the original code can pass the existing
nonvolatile `r18` value to classification; reconstruction preserves that behavior.

## Direct evidence

The isolated Release batch passes **894 comparisons**: 386 complete agreements,
485 matching interrupted/faulted outcomes and 23 rejected negative controls. It
observes every one of the 2,018 indexed sites. Of these comparisons, 49 use
explicit structural controls; that count is not natural gameplay coverage.

The maintenance addition contributes **304 comparisons**, including 133 complete
agreements, 161 matching interrupted/faulted outcomes and ten negative controls.
Budget cuts, sampled missing bytes and dependency interruptions remain focused
on the selected normal boundary fixtures. Comparisons cover full register and
temporary state, mapped memory, ordered effects, dependency ABI, source order,
instruction accounting and allocator/import state.

Two controlled condition-increment cases execute the prepared random component
through its real dependency boundary. A match-to-fitness comparison executes
both complete original bodies and both new C++ components, independently checking
the resulting fitness and condition fields. Other dependencies use deterministic
fixtures. Four source-guard tests pass for all three prepared owners, including
mutated original instructions, constants, pointers and the two padding words.

```sh
python3 -m unittest discover -s tools -p test_injury_fitness_batch.py -v
```

Private references, component compile/link recipes and receipts remain in
`.local/injury-maintenance` and `.local/injury-fitness`. The standalone component
checks link against the accepted dependency libraries; they do not establish
production dispatch or installed-game acceptance.

## Remaining work

Complete the daily injury controller at `0x10185410..0x101864a0`, then integrate
the related prepared owners as one batch. Production adapters/hooks, prefix
verification, actual runtime acceptance, focused sanitizers and application
installation remain pending for these new operations. Save/Load: **not retested**.
Performance: **not benchmarked**. Complete-game reconstruction remains active.
