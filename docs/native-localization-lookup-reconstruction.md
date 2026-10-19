# Localization record and catalog lookup reconstruction

Three complete original operations have semantic C++23 component implementations:
record initialization, current-language lookup and translation-record lookup through
both the in-memory catalog and the disk file. They cover **441 original instructions**.
The component was accepted independently and is now integrated by default in the
[905-owner localization milestone](native-localization-integration.md). The report
below retains its original component contracts and distinguishes that model-based
matrix from the subsequent actual-service and installed evidence. The full-game
reconstruction goal remains active.

## Complete source and data contracts

Intervals are end-exclusive; fingerprints use the project's canonical original
instruction records (`address`, `assembly`, `ops`, `flows`, `fallthrough`).

| Entry | End | Instructions | Operation | SHA-256 |
| --- | --- | ---: | --- | --- |
| `0x10727860` | `0x1072787c` | 7 | initialize record | `2d1bbb77be56c7eaa0f5c598eb246f7fb89a0ee566aa0586251cab299a2c124d` |
| `0x10727880` | `0x10727888` | 2 | current language | `56de1d43dd86aac328437bcc5de8b256270e2425feac4d5217501a98b8be1178` |
| `0x10728400` | `0x10728ac0` | 432 | lookup | `71d6d04d61a18f50f69ebb629b4b8658d4aea068a8d10d6de93775472dae3593` |

All three complete bodies and their page fragments are pinned. The original index
has 134 external root edges and **zero external interior edges**. Three alignment
words are excluded from the instruction count and pinned separately. Seven live
pointer/data regions and six original input defaults are pinned. These pins establish
original inputs, not constant runtime values: semantic execution reads the live TOC,
selected-language byte, catalog mode, row count, language count, selected column,
record pointers and buffers. The independent reference disables every productive hook.
Original listings, reference output, payloads and detailed journals remain private.

## Original-compatible behavior

`initialize_record` zeroes only the three leading pointer words through the original
ordered stores; it does not initialize the ten trailing metadata words or release
prior pointers. `current_language` returns the unsigned live byte without interpreting
or validating its value.

The lookup ABI takes the destination record, query and optional cached-row index
pointer in r3/r4/r5. Its 0x510-byte frame, saved r24..r31, LR, original temporary
residues and instruction accounting remain observable. `normalize_key` removes braces
and spaces and decodes the original newline and quoted-character escapes. It retains
the original trailing scan, including the read preceding an empty normalized key.
No host string or safe-bounds substitution replaces those guest accesses.

A mode byte equal to one selects the cached path. It allocates a normalized temporary
key and searches the live catalog using the original 52-byte row width and comparator
descriptor. A found row replaces three owned destination strings, copies ten additional
word32 fields and optionally stores the original reciprocal-multiply row quotient.
A miss preserves the destination and index. Pointer loads, allocation stores, callback
mutations and temporary cleanup preserve their original ordering and aliases.

Other mode values select the file path. It opens the original path/mode, seeks past
its count-dependent header and scans fixed-width source records. File-key matching
removes spaces and decodes newline escapes; it intentionally retains braces and quoted
escape spellings that the query path treats differently. A hit copies the source,
descriptor and selected language strings, reads ten little-endian fields through the
original item-read ABI and reorders their bytes with individual live reads. Language
columns are skipped through the original read loops, and misses use the original
relative seek. The optional row-index output is unchanged on this path.

A scanned file miss may still replace the destination source string. Original
unchecked short-read results are retained. A failed open follows the original error
reporting, clears the original application-status byte, writes its error text and
returns false. An opened stream is closed even after an empty or unsuccessful scan.
No new fallback, normalization correction or allocation policy is introduced.

## Paired component evidence

The focused Release ARM64 target and CTest pass **614 comparisons**: **466 complete
returns**, **142 matching stops** and **six discriminating negatives**. All **441 own
source sites** occur in complete starting-root comparisons.

| Root | Complete returns | Matching stops | Negatives | Complete own sites |
| --- | ---: | ---: | ---: | ---: |
| `0x10727860` | 4 | 11 | 2 | 7 / 7 |
| `0x10727880` | 260 | 3 | 2 | 2 / 2 |
| `0x10728400` | 202 | 128 | 2 | 432 / 432 |

The matrix compares every architectural and temporary byte, PC, mapped memory,
allocator/import state, executed instruction count, dependency entry/return register
bytes, and ordered read/write/clear/map payloads with their instruction PCs. It also
compares the modeled stream cursor, close outcome, allocations and released pointers.
Each root has an independent result/effect corruption and an inserted-then-restored
write that must be rejected despite the same final memory image.

Independent assertions check all 256 language-byte values, query normalization,
cached strings/metadata and optional row quotients, disk source/descriptor/column
selection, ten byte-reordered metadata fields, miss preservation and open-failure
status. Cases include empty and escaped strings, high bytes, long strings, all three
cached row positions, nullable prior strings/index, multiple language columns, empty
catalogs, short reads, live dependency mutations, relocated TOCs, unaligned records,
record/catalog/query/index aliases, null faults, missing mapped bytes, finite budgets
and interrupted dependencies.

The original initialized temporary cleanup and trailing-trim branches normally have
zero/null inputs. Dedicated, equal observer mutations of those live guest bytes
exercise their otherwise unused branches. This controlled coverage is distinct from
natural catalog execution; complete source coverage is not a claim that every branch
occurs in an ordinary game session.

Nine focused source/data/reference guards pass without skips. String, allocation,
comparator search, file and error-reporting children use explicit component models.
They do not establish actual native-service or productive child coverage. Productive
hooks, actual-service composition, focused UBSAN and installation are now accepted
in the [joint localization report](native-localization-integration.md).

## Component history and current integration

At standalone component acceptance, all **846 protected files**, eighteen unrelated
private prompts and the accepted dependency sources retained their baseline hashes.
Production then contained 901 owners, with installed ARM64 executable SHA-256
`0bef3b58a6880f6f68f8cf85842b8b8d81f5354d605ac93dcedec861fc8a173c`.

The lookup and whole formatter now share
[905-owner productive integration](native-localization-integration.md), preserving
the exact previous prefix, raw bodies, independent references and shared helper.
Current actual-service evidence, sanitizer scope, installed identity and visual
acceptance are recorded in that joint report. Catalog lifecycle/sorting and the
large substitution child retain their existing separate ownership.

Save/Load is **not retested**; performance is **not benchmarked**. PDF updates remain
paused. Whole-game completion is not established.
