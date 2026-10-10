# Semantic Mac formatted I/O, characters and floating input

Delivered on October 9, 2026. Production ARM64 executes 22 new semantic C++23
entries and the missing `CarbonLib::dec2num` / `CarbonLib::nan` bridges. The
manifest contains 100 replacements; the preceding 78 remain in the same order
and retain compact sorted-key SHA-256
`fd8333c4a205239921b990433f4397bbab56c5acb0eb43ea0e584cf3de85219f`.
The full 100-entry manifest hash is
`dee0bb555af0657479f3defa0e8a2b763aabd5717e716a213d9c25d8c75004b3`.

The current starting revision was `ad46216a4e3f94f2e81a4f5ca996f7279c135300`
on main, matching a fresh origin/main query. The earlier scanner report remains
historical: its missing decimal/NaN services were matching faults then. This
increment retains those fixtures with independently expected successful results.
No visual or performance improvement is claimed.

## Closed scope and pins

The export contains 3,902 indexed instructions in these entries, with gaps.
Hashes pin address/assembly/ops/flows/fallthrough records, sorted by address,
in compact sorted-key UTF-8 JSON. Bridges are imports, not additional entries.

| Entry | Exclusive end | Indexed instructions | Body SHA-256 |
| --- | --- | ---: | --- |
| `0x10778db0` | `0x1077961c` | 537 | `9d77e7c9292f155e8d5f82dbae9a95eac10fa25b31809bf25a8335d2b715d018` |
| `0x10779620` | `0x10779874` | 149 | `d04782e3a2562e22408ade74e133079fe657227d1e56b841db2f4f00e3403716` |
| `0x10779880` | `0x10779b70` | 188 | `73cee661ead3f7dc13187e6ed3b7a1592571e86e073e79b7d378eaee87d6f0b3` |
| `0x10779b70` | `0x10779ec4` | 213 | `05dd8372702a1f5eaf71461f600caaf06beaa3f1c5f8c4a8d9ab77fbbec019bf` |
| `0x10779ed0` | `0x10779ff0` | 72 | `d5d8a676a3785276a70f5b4528ad1d674dbe9b73398d0ea665b0415eaf449ba7` |
| `0x10779ff0` | `0x1077a000` | 4 | `a7b6b9884fc4e62cda472cea5e0e690bc527320cf6962c8486a60bbe8f490b3d` |
| `0x1077a000` | `0x1077a688` | 416 | `d2efad11f7bacef895ef80e09dc1551740f6f194658cca520d40cbfbee95a1a3` |
| `0x1077a690` | `0x1077b4bc` | 905 | `f2f03b913302837f0e3cdf83c7443b6b33e03cf54a117784c64c8405a07213ce` |
| `0x1077b4c0` | `0x1077b518` | 22 | `151c7ee9e4e169a7948965366ff9ef7d5d7f77bdd15b3d53211006f2ff1e285a` |
| `0x1077b520` | `0x1077b58c` | 27 | `04f3344b694f63cebd1ef4015c0f84e627edfbcc1ab30442bc9120cbc9a73895` |
| `0x1077b590` | `0x1077b60c` | 31 | `b4299df76c0e97fbff2af11d9bccd17652b2668fc00659e58c5e6c085beba492` |
| `0x1077b610` | `0x1077b680` | 28 | `63859c1a7c3e18edff44de9076e0ad885a53a22f6126e14e906faa12cf20816d` |
| `0x1077b680` | `0x1077b6f0` | 28 | `ca9ddb3c231f9890952b7781f60951318408bc84aa32fa8f5b679f682e0d7478` |
| `0x1077b6f0` | `0x1077b71c` | 11 | `d09d94e55aa60fcb3079d02a4f31baf6e9a576301b62bea953f6b53ff1f9ad3b` |
| `0x1077b720` | `0x1077b768` | 18 | `1f9eadd348ef0ecc80ff3046885dcf156a1da6d0066242cf4729ab41c70715e0` |
| `0x10777f60` | `0x1077804c` | 59 | `18e9c5e242d2a7bc609170c8b8b46a41a8f917b8c9b0d24504e3fe8bfb8478d4` |
| `0x10778050` | `0x10778170` | 72 | `ee7bbc5ebd365ac62f88d0c618e9ee1fe816970d04ce4dc534235bec448475a6` |
| `0x10778170` | `0x10778190` | 8 | `f81d79e971a21aa38f0ac88fcd49a741524566e51711a35d9b0ae5f02ae3caf0` |
| `0x10778190` | `0x10778234` | 41 | `d8f444a8742586897868cba5be5309ced8e2c0eafea674c44c577133d5779db8` |
| `0x10778240` | `0x10778260` | 8 | `d1bc2f230c1440eabdfae5512df65af3610f5e07148fd0f1b77d9f798ffbd795` |
| `0x10778260` | `0x10778300` | 40 | `7fe292fa32e6a1dfb0820710fdc91ad15d90a518237d5f791b989132d123c37e` |
| `0x1077de10` | `0x1077ee14` | 1025 | `d8a7ca0ccef635d1d52ac5f2964656fddcd485635321da6552d2668ff9e8c873` |

Output occupies manifest positions [78:93], characters [93:99], floating input
[99:100]. Older module slices are unchanged; the scanner slice is [71:78].
Whole bodies and all eight complete cross-page fragments are pinned by the
three audit modules. Missing bodies/pages, altered records and misplaced hooks
fail before a full manifest can be emitted.

The single indexed incoming interior edge is
`0x10779ff4 -> 0x10779ed8`. Both the edge and destination hook are explicit:
the four-instruction length helper shares the decimal reset operation. Its
removal, mutation or an additional interior edge is rejected. The shared tail
is not a public manifest entry. Generated and mixed transitions exercise it.

## Semantic operations and Mac ABI

Output format parsing preserves the 60-byte record, including uninitialized
special-path padding, frame/record/cursor aliases and ordered field copies.
Flags, literal/dynamic width and precision, negative fields, length modifiers,
percent, invalid/truncated conversions, auxiliary delimiter strings and packed
lanes retain their guest contracts. Classification and destination tables are
read live; known destinations select named operations. Unknown destinations
retain the runtime transfer boundary and separate private escape accounting.

`0x10779620`, initially described tentatively as a rounding helper, is the
word32 reverse integer formatter. Its live dispatch selects signed decimal,
unsigned decimal, octal or hexadecimal. The word64 formatter keeps the existing
division helpers and their observable register/carry residues. Decimal rounding
and length/reset preserve ties, trailing zeroes, carry and exponent effects.
Finite decimal/exponential/general output and hexadecimal output use the existing
`num2dec` glue and bridge, guest constants, strings and formatting records.
No host printf, format, to_chars or textual parser produces substituted output.

The descriptor formatter preserves its 768-byte frame, nonvolatile registers,
f31 and the distinction between calculated count, written bytes and callback
error. Every emit uses the generated `0x10781320` stub and reloads descriptor
entry/TOC; callback clobbers and aliases remain observable. Strings, `%n`, signs,
padding, packed alignment and delimiter emission are separate operations.
The existing semantic string/memory helpers remain dependencies.

The bounded callback rereads written and returns 1 even when truncating.
The bounded facade NUL position follows the original calculated count/capacity
comparison, including a write at destination-1 for nonnull destination and zero
capacity. Null pointers follow their original paths. Variadic/vector wrappers
preserve separate frames and original register spills.

Stream tests use artificial guest objects only: type/buffering halfword 0x0440,
state byte at +8 equal to 0x20, buffer base/capacity/current/remaining at
+32/+36/+40/+44. Chunks fit the existing buffer and require no flush, file or OS
write. Standard-stream tests redirect the guest slot. Orientation refusal,
helper error, zero count and exact-count callback return are checked. Stream
orientation/write implementations remain generated dependencies.

The character chain classifies one-to-three-byte sequences and converts wide16
without importing ARM64 wchar_t or modern Unicode validation. Continuation checks,
legacy overlong handling, short-circuit reads and final-byte masks are retained.
Encoding spills two live TOC halfwords before a null destination return, stores
bytes in original order and preserves partial-string behavior. Scanner `%lc` and
`%ls` retain their different destination increments; output `%ls` uses the new
wide16 string chain.

Floating input separates whitespace/sign, decimal significand/fraction/exponent,
hexadecimal phases, special-word matching and termination. Templates, mapping,
classification, decimal separator, constants and descriptor callbacks remain
guest/live. Original frame, consumed count, flags, lookahead and FPSCR effects
are preserved. Decimal packed scanning uses one delimiter between values;
infinity's original lookahead consumption requires its historical double delimiter.

## Decimal input bridges

`dec2num` marshals the immutable 42-byte Apple decimal record, including signed16
big-endian exponent, sign and length, and returns f1. Apple native record
conversion uses the guest rounding mode. Native exceptions are discarded and
the caller's native floating environment is restored; the entire guest FPSCR
is preserved. Undefined length above 36 and non-IEEE arithmetic are rejected.
`num2dec` was not changed.

`nan` reads a complete guest NUL-terminated tag and constructs the PPC quiet NaN
bits directly. The decimal code is bounded to 0..255 and placed at fraction bits
44..37; the quiet bit is 51. Negative codes become zero. Host std::nan is not used.
Independent controls check exact result bits, records/tags and guards, signed
zero, rounding, overflow/underflow, specials, FPSCR/native environment, fault
prefixes, and actual generated CFM glues with import accounting.

Primary contracts: local Apple SDK fp.h/math.h and
[Inside Macintosh: PowerPC Numerics](https://www.vintageapple.org/inside_r/pdf/PPC_Numerics_1994.pdf),
chapter 7's NaN code definition, chapter 9 decimal conversion and Tables E-9/E-19
(no numeric exceptions for these conversions). ABI compatibility alone was not
used as proof of PPC payloads or exceptions.

## Focused acceptance

Full cloned-state comparisons include all architectural and temporary bytes,
PC/LR/CTR/TOC/CR/XER/FPSCR, mapped memory, ordered scalar/bulk writes, callback
arguments and prefixes at faults. Memory bulk reads retain the existing runtime
observer behavior; minimal bridge mappings explicitly include their full records.
Only instruction accounting and process-local diagnostic selection/counters are
excluded. ABI differences are not normalized.

| Chain | Comparisons | Expected faults | Own negatives | Child negative | Generated indirect entries | Mixed modes |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Output | 1,395 | 1,028 | 15 | 1 | 15 | 16 |
| Characters | 335 | 49 | 6 | 1 | 6 | 8 |
| Floating input | 156 | 89 | 1 | 1 | 1 | 2 |
| Scanner regression | 940 | 641 | 7 | 1 | 7 | 12 |

All 22 new public entries have nonempty successful semantic witnesses, with zero
original selection and zero normal escape. Output includes two shared reset
transitions and three separately identified unknown-destination diagnostics;
the latter are excluded from normal witnesses. Controls cover live tables,
classification, descriptor mutation/clobbers/failure, prefix/template changes,
TOC relocation, representative aliases and minimal mappings with removed bytes.
Independent examples include 7.25, ties, signed zero, exponential/general/hex
output, truncated buffers, wide16 bytes, packed lanes, negative dynamic fields,
stream bytes/cursor, and successful `%f` / `%lf` / `%Lf`, `%n`, suppression,
packed decimal and NaN input. Independent resets preserve other modules.

The final normal-witness counters below are from the focused harnesses,
including successful child calls. Every row also has its own rejection control
and a generated indirect transition; diagnostic/mixed runs are separate.

| Entry | Normal semantic calls | Original calls | Normal escapes |
| --- | ---: | ---: | ---: |
| `0x10778db0` | 142 | 0 | 0 |
| `0x10779620` | 119 | 0 | 0 |
| `0x10779880` | 45 | 0 | 0 |
| `0x10779b70` | 36 | 0 | 0 |
| `0x10779ed0` | 53 | 0 | 0 |
| `0x10779ff0` | 48 | 0 | 0 |
| `0x1077a000` | 70 | 0 | 0 |
| `0x1077a690` | 119 | 0 | 0 |
| `0x1077b4c0` | 18 | 0 | 0 |
| `0x1077b520` | 151 | 0 | 0 |
| `0x1077b590` | 6 | 0 | 0 |
| `0x1077b610` | 6 | 0 | 0 |
| `0x1077b680` | 94 | 0 | 0 |
| `0x1077b6f0` | 28 | 0 | 0 |
| `0x1077b720` | 24 | 0 | 0 |
| `0x10777f60` | 191 | 0 | 0 |
| `0x10778050` | 154 | 0 | 0 |
| `0x10778170` | 77 | 0 | 0 |
| `0x10778190` | 51 | 0 | 0 |
| `0x10778240` | 37 | 0 | 0 |
| `0x10778260` | 21 | 0 | 0 |
| `0x1077de10` | 66 | 0 | 0 |

Final regeneration: 1,953,157 instructions / 1,922 pages, zero missing/pending
static targets. Release ARM64 build is consistent with the final State layout.
Unchanged generated source timestamps were retained only after byte-for-byte
hash comparison; every unit had already been rebuilt with that State layout.
Seven focused CTests passed in 26.44 seconds: output, characters, floating input,
scanner, decimal, string and memory. The nine Python audit scripts passed 46
tests. Focused UBSAN instruments the nine new adapter/bridge sources and all
new/modified harnesses, linked against the final consistent libraries. All five
binaries passed with empty stderr.

The bridge suite adds 101 independent input/CFM checks and two independent
negative controls to the retained 1,064 num2dec checks and 14 original formatting
examples. Historical reports have not been rewritten.

## Delivery and limits

The installed baseline was preserved with forks, xattrs and metadata in a new
private reference. Protection includes 809 Game files, 66 Runtime files, 52 Runs
files, presentation preferences, original payloads, the LFS save snapshot, all 22
handoffs (17 private), and 117 historical bundles. Private receipts, indexes,
listings, logs, fixtures, payloads, saves and bundles are excluded from publication.
The protected installer ran once on October 9 at 21:12:21 São Paulo time.
Internal executable identities are distinct from bundle modification dates:

| Artifact | Architecture | Executable SHA-256 |
| --- | --- | --- |
| Linker output, retained privately | ARM64 | `f7ec2c8aa65c59a5de8290a606a60c881ae552d75715ad6b7182cc652bce18b6` |
| Signed source / candidate / installed | ARM64 | `0725840a0907bc6bd866d47632fbe75606defaa98d320ecb53b50c7348fed749` |
| Preserved installed baseline reference | ARM64 | `2e88cca135176aed98fe9771c06a269463e7fd16ebc14161fd66bd9c4492437e` |

Original payloads, production plists and strict deep signatures passed. No
validation fixtures/configuration were included in the bundle. Immediately
after installation and before UI, protected content and metadata were identical.

The installed app at `/Users/victordmor/Applications/CM 01-02 Native Prototype.app`
rendered Prepare Game. Command-O opened the Load chooser; Command-[ cancelled
without loading. Command-Q and Return requested normal exit. The running process
was verified against the installed path and hash above. Its log recorded normal
exit and no fatal diagnostics. A bound-app observation after quit reopened a
second exclusive test session; that session was also quit normally. Final
process verification found no remaining installed-app process.

The first smoke log recorded semantic output calls at format analysis (770),
word32 formatting (4), descriptor formatting (835), bounded callback (791),
bounded vector facade (835), unbounded vector facade (72), and variadic buffer
facade (763), with zero original selection/escapes. Characters and floating input
were not called by this smoke. These observations do not replace the per-entry
harness evidence above. After smoke, Game content/metadata, LFS snapshot,
handoffs, references and all historical bundles remained unchanged. Runtime/Runs
and preferences may change during the intentional UI session; byte identity of
those artifacts is not claimed.

The original `0x1p0` floating exponent phase and its semantic counterpart each
exhaust a small 1,500-instruction diagnostic budget. Neither is counted as a
passing conversion. Executable hexadecimal paths are compared separately.
No legacy correction or extended investigation was introduced.

Save/Load is **not retested**. No career, day, match, persistence campaign,
benchmark or full test suite was run. Date's ascending-year leap behavior is
unchanged. Locale, num2dec, fwrite/flush/refill and the remaining file/stream
runtime remain dependencies, with no ownership claim from this reconstruction.
