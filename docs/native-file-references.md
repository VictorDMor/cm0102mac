# Native Mac file references and Unicode conversion

This increment supplies thirteen missing native services used by the original
Mac file controllers. Production `Gestalt('fs  ')` now returns `0x1302`, allowing
the existing controllers to choose their FSRef and signed-64-bit fork path.
The previous [fork transport](native-fork-io-bridge.md) remains its real storage
implementation. The ordered semantic manifest is still **149** entries; these
native imports are not semantic replacements of the original controllers.

## Services and execution evidence

All imports use the `CarbonLib::` namespace. Each service has an actual positive
execution and a discriminating rejection in `file_reference_checks.cpp`.
The generated original chain also invokes every service below through the normal
production dispatcher, including the Classic metadata setter in a mixed-path
fixture. Native errors are retained rather than replaced with canned successes.

| Import | Implementation and focused evidence |
| --- | --- |
| `FSpMakeFSRef` | Resolve the existing guest FSSpec namespace into a native opaque identity; missing file rejects without publishing one |
| `FSMakeFSRefUnicode` | Resolve a Unicode child through the merged namespace and existing canonical/case comparison; missing child leaves output untouched |
| `FSGetCatalogInfo` | Marshal selected catalog fields and optional name/FSSpec/parent outputs; sparse sizes, dates, permission widths, missing fields and wrapping extents are checked |
| `FSCreateFileUnicode` | Create in private storage using Unicode units and selected metadata; accented and supplementary names succeed, collisions and empty names reject |
| `FSOpenFork` | Open real data or named resource forks owned by `NativeDataForks`; invalid/outside identities reject and data/resource payloads remain distinct |
| `FSCompareFSRefs` | Compare effective native identities; equal identities, distinct files and malformed references discriminate results |
| `FSRenameUnicode` | Preserve native identity and guest directory/file IDs using the existing rename implementation; collision rejects and the old identity follows a successful rename |
| `FSDeleteObject` | Remove an actual private object through the existing namespace policy; repeated deletion, malformed reference and outside-root reference reject |
| `PBSetCatInfoSync` | Set Classic Finder/extended Finder metadata, flags and dates through native catalog services; Classic/FSRef composition and unknown-directory `ioResult` are checked |
| `UpgradeScriptInfoToTextEncoding` | Preserve the native signed script/language/region arguments and optional Pascal font; system encoding and invalid-script results are compared |
| `CreateTextToUnicodeInfoByEncoding` | Own native contexts behind word32 tickets; unsupported/invalid encoding retains the native error and does not publish a ticket |
| `ConvertFromTextToUnicode` | Convert through the actual native context with counts and offset maps; MacRoman, multibyte Shift-JIS, incomplete input, short odd capacity, zero length and extent failures are covered |
| `DisposeTextToUnicodeInfo` | Dispose the owned context and clear its ticket; stale/double disposal rejects |

Direct dispatcher fixtures assert that registers and temporary storage remain
unchanged except for the documented result register and return PC. Files and
directories are real, disposable native objects with independent POSIX/xattr
checks. Conversion results, byte counts, offsets, dates and permission mode are
compared to direct CoreServices calls, not to the bridge's own helpers.

## PowerPC layouts and lifetime

FSRef is an opaque **80-byte** native File Manager identity. Its bytes are copied
without treating it as a pointer-bearing SDK structure. It tracks native object
identity across rename. Resolving an injected reference still checks the two
configured roots before any operation. It cannot grant access to arbitrary host
files.

Catalog information is different: the original two-byte-aligned record is
**144 bytes**, and the LP64 SDK permission pointer changes its host layout.
Numeric fields are marshalled explicitly in big endian order. Only requested
fields are read or published; a minimally mapped data-size destination works
without the other 128 bytes. Arithmetic never wraps a field into low guest memory.

| Guest offset | Width | Field |
| --- | ---: | --- |
| 0 | 2 | Node flags |
| 2 | 2 | Virtual signed volume reference |
| 4, 8 | 4 each | Guest parent and node IDs |
| 12, 13 | 1 each | Sharing flags and user privileges |
| 16, 24, 32, 40, 48 | 8 each | Creation, content modification, attribute modification, access and backup UTC dates |
| 56, 60 | 4 each | Native user and group IDs |
| 64, 65, 66 | 1, 1, 2 | Reserved permission byte, user access and mode |
| 68 | 4 | No host `FSFileSecurityRef` pointer is published |
| 72, 88 | 16 each | Finder and extended Finder records |
| 104, 112, 120, 128 | 8 each | Data/resource logical and physical sizes |
| 136, 140 | 4 each | Valence and text encoding hint |

Each UTC date has high-seconds word, low-seconds long and fractional word.
File and folder Finder records have different first fields and require different
endian conversion. Extended records mix words and longs; raw host byte copies
would be wrong. Setting existing Finder metadata uses the actual native object
kind for that conversion. Classic parameter blocks retain the existing guest
FSSpec/IOPB offsets and receive a signed `ioResult` at offset 16.

Native conversion objects are held by `NativeUnicode`; process-local tickets
start at `0x5b000000`. Native pointers do not enter guest context slots. Disposal
removes ownership, clears the guest ticket and leaves stale tickets invalid.
Destruction releases outstanding contexts; a fault publishing a newly created
ticket releases the unpublished native context.

The twelve-argument converter receives arguments 1--8 in `r3`--`r10`, then output
capacity, source-count pointer, Unicode-byte-count pointer and Unicode buffer at
stack offsets **56, 60, 64 and 68**. Offsets/counts are word32, output characters
are big-endian UTF16 units, and output length is a byte count. Completed prefixes
and native error codes are retained. Native encoding acceptance is not widened:
the acceptance host rejected a UTF8 TextToUnicode context with `-8738`, while
MacRoman and Shift-JIS contexts worked. The check also handles a future native
SDK accepting that encoding by comparing a real conversion.

## Namespace and original-media preservation

The bridge preserves the existing two virtual volumes, guest IDs, case/canonical
filename comparison and merged original/private directory listings. It maps
catalog parent/node/volume fields back to that view rather than exposing host
volume IDs. Virtual-root parent output stays within the configured namespace.

Write access to original media still creates a private copy with data, resource
fork and Finder metadata. An identity acquired before that copy resolves to the
current visible replacement for later reads or metadata queries. The real-file
fixture writes `port` through such an identity, closes/reopens it, then reads the
private bytes while the original continues to contain `original`.

Unicode creation and rename use the original supplied units/encoding information
where applicable. FSSpec outputs retain the existing MacRoman/63-byte leaf limit;
Unicode-only names are exercised without requesting a legacy FSSpec. The existing
original-media remove/rename policy still requires persistent tombstones and
remains an explicit unsupported case. This increment does not remove that limit
or replace the existing alias-resolution policy.

## Focused acceptance

- `native_file_references`: **401 assertions**, **17 generated routine calls**,
  and **28 distinct real imported services** in the original-chain transcript.
- Original `fopen` discovers the production capability itself. It opens/reads/
  closes an existing file, creates/writes/closes a new file, truncates write mode,
  rejects missing read mode, creates an accented MacRoman filename through the
  real Unicode conversion chain, renames/removes a private file and maps a failed
  removal to the original facade's `-1` result. A final forced Classic fixture
  verifies creation/Finder metadata/close interoperability.
- Six focused CTests pass: native bridges, filesystem, file references, fork I/O,
  stream I/O equivalence and stream buffer equivalence (**17.94 s** aggregate).
- Three UBSAN binaries pass with empty stderr: file references, fork I/O and
  filesystem. The five changed native implementation units and these three
  harnesses were instrumented and linked to the final consistent Release build.
- Release ARM64 build passes. There is no State-layout change or new generation.
  Manifest order/content remains the prior 149-entry array with SHA-256
  `6a9f68dbb88b2fd876157371d39098f2e9746aff781342ddfb5442139b98f9cb`;
  the generated compilation still has 1,953,157 instructions, 1,922 pages and
  zero missing/pending targets.

## Protected installation and remaining work

The prior installed executable is protected with forks/xattrs/metadata, SHA-256
`b773d4ab9281a23b673243b384e935b0eec215728bf01e772d32e7a6531113c5`.
Before installation, protection covered 809 Game records, five Runtime records,
63 prior run-log records, the workspace save snapshot, 23 handoffs, original
program payloads and 136 earlier private bundles.

The linker-output executable SHA-256 before final bundle signing is
`9b920614a64bc4b20d7ed165f0ddec46a95d05f0641ac2f4b8bdad9fa3811ee0`.
The signed source/candidate/installed executable is ARM64, SHA-256
`b3c376d2cb730bef9173890da3f5dc5d73d996107539269fc379611984103c88`.
Strict signature, normal production plist and all four original bundled payloads
were checked independently. No validation settings are packaged.

One protected installation delivered that candidate. The installed app rendered
Prepare Game, opened the original Load Game chooser showing Reading, cancelled
back to Prepare Game and exited through the original confirmation. Its running
executable matched the installed hash. The startup log records a normal exit
after 11,847,460,081 statically recompiled instructions, with no fatal diagnostics;
the process was confirmed closed. No career was loaded. This menu sequence is
not per-import coverage; the generated-chain fixtures provide that evidence.

Final protection passed for Game content and metadata, the workspace save
snapshot, all 23 handoffs, original payloads, the protected reference and 136
earlier private bundles. Runtime/Runs and presentation preferences may change
from the intentional smoke; no byte-identity claim is made for those afterward.

Save/Load is **not retested** for this increment. No performance benchmark,
career, simulation-day or match acceptance is claimed. Named semantic C++23
open/mode/seek/tell/file-controller bodies and the rest of the engine/runtime
remain required work under the active complete-port goal.
