# Preference-session opening and action controllers

Status: **prepared and accepted as an isolated component**. Three complete
operations cover opening an editable preference session and handling its actions.
They contain **371 original instructions**. The installed application and
productive manifest retain **897 owners**, executable SHA-256
`7af41b4b2882a5dbc16d679884cb60caa2175cc7242cf3074733c8076bb9e00d`.
Adding these three roots would reach 900; that is not an installed result.

| Root | Exclusive end | Instructions | Operation |
| --- | --- | ---: | --- |
| `0x1033ce70` | `0x1033cf14` | 41 | Open with a supplied mode |
| `0x1033cf20` | `0x1033d028` | 66 | Open with mode selected from a game-state byte |
| `0x1033f840` | `0x1033fc60` | 264 | Dispatch a preference action |

The source contract has four page fragments, two indexed incoming root edges,
zero external interior edges and five nonindexed alignment words. Five original
pointer/data contracts pin the UI context, primary preference cell, language
panel cell and both callback descriptors. The opening callbacks name the full
presentation builder at `0x1033d030` and action controller at `0x1033f840`.

The **2,562-instruction presentation builder** is separate and remains
unreconstructed. Its original source is inspected and pinned only as context
for identifying the auto-save menu. The adjacent `0x1033fc60` operation opens a
different screen; it is not an options cleanup routine or part of this block.

## Editable-session ownership

Both opening paths first ask the UI manager to select the original callback
pair. Only a nonzero signed low byte of its result continues. They allocate a
`0x150`-byte edit record, copy/link it to the primary preferences when allocation
succeeds, then write two screen user-data slots: the edit pointer and mode.
Allocation failure still writes a null edit pointer and the mode. The controller
does not add a new allocation-failure policy.

The supplied-mode root stores only the mode's low byte. The other root tests
the signed low input byte and stores mode 2 for zero or 3 otherwise. Both keep
their initially loaded UI context. Their primary-cell lookup differs: the
supplied-mode root loads the global cell after allocation, whereas the game-state
root retains the cell address loaded on entry and reads its current contents.
Reentrant model cases discriminate both behaviors.

## Action selection and results

The dispatcher fetches the editable record and mode from the screen user data.
It sign-extends the event index's low halfword and uses the active view's
`0x17c`-byte row. Index -1 has no row; a zero low action halfword uses the UI
context's fallback action. The comparison stages preserve the original repeated
reads, signed truncations, architectural residues and call order.

| Action | Original behavior | Result |
| ---: | --- | ---: |
| 39 | Restore defaults in the edit record | -11 |
| 40 | Apply the primary record, then destroy the edit with deletion enabled | -9 |
| 41 | Apply the edit record, then destroy it with deletion enabled | -9 |
| 42 | Pass a signed low language byte; conditionally refresh the language panel | -11 |
| 43 | Pass a signed low currency byte | -11 |
| 44 | Pass the entire option word | -11 |
| 45 | Select option `0x2000`, reload the active view and pass a signed low interval halfword | -11 |
| 61 | Pass the entire video-mode word | -11 |
| Other | No preference child call | 0 |

Apply/discard ignore the apply child's return value: destruction and result -9
still follow a modeled failure. Language refresh also ignores the language
setter's result. It requires mode's low byte 0 or 1 and a currently nonzero panel
cell; that cell is loaded after the setter. These are preserved original
behaviors, not new transactional or error-handling guarantees.

Auto-save reads the active view again after option selection, while retaining
the initially cached UI context. A callback can therefore change which row
supplies the interval. Independent assertions require this later row's value.

## Auto-save interval meaning

The original presentation builder identifies the interval as **Auto Save**.
The source-context guard pairs each menu label with action 45 and its payload:

| Menu choice | Stored interval value |
| --- | ---: |
| Every Week | 7 |
| Every Two Weeks | 14 |
| Every Month | 30 |
| Every Three Months | 90 |
| Every Six Months | 180 |
| Every Year | 360 |

The [existing preference setter](native-preferences.md) accepts 7..365 inclusive
and defaults to 30. The annual UI choice uses 360, independently of that upper
validation bound. Loading still reads the saved halfword directly. This block
establishes the UI mapping; it does not reconstruct the calendar scheduler or
prove when an automatic career save runs.

## Isolated evidence and remaining integration

The focused component passes **2,476 comparisons**: 2,332 complete returns,
138 matching stops and six deliberate-corruption controls. All **371 own sites**
are observed in complete comparisons starting at their respective roots: 41, 66
and 264 sites. Each root has both an altered-result negative and a transient
write/restore negative that leaves the final memory image equal.

Comparisons include every architectural byte, generated temporary residue,
allocator state, mapped memory, ordered read/write effects with instruction PCs,
dependency entry/return ABI, source-step order and instruction accounting. The
corpus covers all low mode bytes, all language/currency payload bytes, high-bit
aliases, signed event indices, fallback actions, allocation and open failures,
apply/setter failures, current panel/view mutations, instruction-budget cuts,
required-byte faults and dependency interruptions. Ten source/data guards and
the single focused CTest pass.

UI selection, allocation, user data, preference children and language-panel
refresh are **explicit boundary models**. Their record mutations support caller
ABI/order checks; they are not replacements for the previously accepted
preference implementations, real native services, real allocation or actual file
writes. Productive hooks, actual-service/whole-caller proof, focused UBSAN and
protected installation remain for a consolidated milestone. Existing preference
bodies and the shared semantic helper remain byte-identical.

Save/Load is **not retested**; performance is **not benchmarked**. PDF updates
remain paused at the user's request. The complete-game goal remains active.
Private original references, coverage journals, snapshots and protected-file
inventories remain local.
