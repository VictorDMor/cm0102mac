# Native Mac fork I/O transport

This increment implements seventeen previously missing File Manager imports in
the production ARM64 dispatcher. It is a prerequisite for the ongoing stream
file-controller migration, not completion of that migration or of the whole port.
The ordered semantic manifest remains exactly **149** entries; native imports
are not counted as reconstructed original C/C++ routines.

## Services and ownership

| Imports | Native behavior |
| --- | --- |
| `PBReadForkSync`, `PBReadForkAsync` | Read an owned fork using a signed 64-bit position and unsigned 32-bit count; preserve partial data/count on EOF |
| `PBWriteForkSync`, `PBWriteForkAsync` | Write an owned fork at a signed 64-bit position; publish the actual count and native error |
| `PBGetForkSizeSync`, `PBGetForkSizeAsync` | Return the full 64-bit logical EOF |
| `PBSetForkSizeSync`, `PBSetForkSizeAsync` | Resize with the original position mode and signed 64-bit offset |
| `PBSetForkPositionSync`, `PBSetForkPositionAsync` | Set the native mark without truncating the high word |
| `PBReadAsync`, `PBWriteAsync` | Complete the Classic signed-count/word-offset parameter calls, including their mark output |
| `PBGetEOFAsync`, `PBSetEOFAsync`, `PBSetFPosAsync` | Complete the existing Classic size/position contracts |
| `FSSetForkSize` | Marshal the PowerPC `r5:r6` signed 64-bit argument following `r3` reference and `r4` mode |
| `FSCloseFork` | Close only a reference owned by this native File Manager instance |

All names use the `CarbonLib::` import namespace. `NativeDataForks` retains its
ownership set and real CoreServices fork handles. It still closes owned handles
on destruction and rejects unknown/closed references. Its seek/read/write/resize
interfaces now accept 64-bit offsets. The existing Classic synchronous bridge
continues to sign-extend its original 32-bit values before entering this storage.
No host container replaces guest stream descriptors, file-record lists or C++
allocator metadata.

`fork_io_bridge.cpp` shares the existing bounded transfer algorithm with Classic
sync I/O: at most 1 MiB of temporary storage, ordered bulk guest copies, an initial
position followed by native-mark continuation, and explicit failure if a native
operation makes no progress without an error. Requests crossing the guest 32-bit
address boundary fail before file I/O. No scalar observer requirement is added to
bulk memory copies.

## PowerPC parameter ABI

The original pointer-bearing `FSForkIOParam` is two-byte aligned. Host SDK structs
must never be overlaid on guest memory. Fields are explicitly marshalled in big
endian order:

| Offset | Width | Field |
| --- | ---: | --- |
| 12 | 4 | CFM completion transition vector |
| 16 | 2 | signed `ioResult` |
| 24 | 2 | signed fork reference |
| 32 | 4 | guest buffer address |
| 36 | 4 | unsigned fork request count; signed for Classic I/O |
| 40 | 4 | actual byte count |
| 44 | 2 | position mode |
| 46 | 8 | signed fork position offset; Classic uses a four-byte offset here |

Classic EOF is a signed word at offset 28. Fork EOF is returned at offset 46.
Fork read/write/set-position retain their input position field; the native mark
is maintained by CoreServices. Classic read/write/set-position retain the existing
bridge's updated word-mark output. Failed size queries do not manufacture an EOF.
Read EOF retains the bytes actually obtained; the generated stream callback maps
partial EOF to success and empty EOF to its original stream status 2.

Synchronous fork parameter calls clear a stale completion pointer. Asynchronous
services write a pending value, perform the real native operation, publish the
result/count, and invoke a supplied original CFM completion once. Fork async
functions are void and retain `r3`. Classic async functions return `noErr` for
the accepted request; the completed operation may still fail in `ioResult`.
Completion runs through the
existing saved-register native dispatcher, with the parameter block as its sole
argument. Local operations complete immediately; no worker or pending request
outlives `State`. This adds completion semantics, not parallel disk throughput or
a promise that an original yield loop executes on this backend. Apple describes
completion and polling in its archived
[File Manager documentation](https://leopard-adc.pepas.com/documentation/mac/Files/Files-73.html).
Field directions and widths were checked against the installed SDK `Files.h` and
the archived [File Manager reference](https://leopard-adc.pepas.com/documentation/Carbon/Reference/File_Manager/Reference/reference.html).

## Focused acceptance

`native_fork_io` passes **377 independent assertions**, including positive calls
to all seventeen imports and closed/unknown-reference rejection for all fifteen
parameter services. Checks use actual private native files, not canned import
responses. They cover:

- A sparse file above 4 GiB, high-word read/write/EOF, signed relative offsets,
  absolute growth, EOF-relative shrink and rejected negative sizes. Its physical
  allocation remains below 16 MiB.
- Partial and empty EOF, unsigned fork counts with the high bit set, zero-length
  transfers with an unmapped buffer, bounded multi-chunk continuation, byte/field
  guards and rejected guest address overflow without file mutation.
- Three real generated CFM completion executions, including failed I/O, with
  pending/result ordering, one invocation, correct argument, architectural and
  temporary restoration; sync calls suppress a stale completion.
- Seven executions of the existing generated Mac read/write/seek callback
  bodies over the new services. They include async selection, partial/empty EOF,
  the original rejected seek origin, 64-bit EOF-relative carry, native-mark
  preservation and the original low-word callback output.
- Native `wrPermErr` (-61) for a read-only fork, ordinary close, double-close and
  private-overlay writes preserving original media.

The read-only fixture initially expected `permErr` (-54). The actual File Manager
returned `wrPermErr`; the fixture was corrected to the documented fork permission
error. The implementation did not normalize the native result. Failed attempts
and final receipts remain private.

Five focused CTests pass: `native_fork_io`, `native_filesystem`,
`modern_stream_io_equivalence`, `modern_stream_buffer_equivalence` and
`native_bridges` (16.14 seconds together). Three UBSAN binaries pass with empty
stderr: the new fork harness, the real filesystem harness and the stream I/O
equivalence harness. `fork_io_bridge.cpp`, `data_forks.cpp`, `platform.cpp` and each
harness were instrumented and linked against the consistent final ARM64 libraries.
No generated source or `State` layout changed, and regeneration was unnecessary.
The exact prior manifest SHA-256 remains
`6a9f68dbb88b2fd876157371d39098f2e9746aff781342ddfb5442139b98f9cb`.

## Delivery and remaining boundary

The signed source/candidate/installed ARM64 executable SHA-256 is
`b773d4ab9281a23b673243b384e935b0eec215728bf01e772d32e7a6531113c5`.
The linker artifact before final bundle signing is separately retained with SHA-256
`a03dbc0a64b85761fcf85222080e77e434cd231baf789188efce845f1789b6f2`.
The protected prior installed reference is
`5e219ce7b0c4e35a0761f0ba4ff52a326873bfe1faf9f7e99dd60172ec6c373a`.
Strict signatures, production launch metadata and all four original-program
payload hashes pass. A preliminary locally validated snapshot was installed;
final SDK review then corrected the Classic async queue-result ABI. That version
was preserved privately, followed by a rebuilt/retested final protected update.
Two installations occurred. Before the final update and its UI smoke, full
content/metadata protection covered 809 Game files, five Runtime files,
62 existing Runs files, workspace saves, the new reference and 131 earlier bundles;
all 23 handoff contents and original payload hashes were retained.

The installed app rendered Prepare Game, opened the original chooser through
Game -> Load Game (Reading visible), returned through Game -> Back or Cancel,
and exited through the original quit confirmation. The running executable
matched the installed hash, no fatal diagnostic appeared and no game process
remained. No career was loaded. Final protection again passed for Game and
installed user save content/metadata, workspace saves, payload/handoff contents,
the new reference and all 131 earlier bundles. Runtime/Runs/presentation files
may change from intentional UI; no identity claim is made for them after smoke.
Save/Load is **not retested** for this transport increment; the earlier isolated
stream I/O persistence witness is not promoted to current acceptance.

Production `Gestalt('fs  ')` deliberately remains on its existing Classic
capability response. The generated file callbacks use the new fork services in
focused fixtures, but ordinary menu smoke cannot establish their execution.
FSRef lookup/open/create/catalog/rename/delete, text-to-Unicode lifetime/marshalling
and the semantic open/mode/position/file-controller bodies still require their
own implementations and acceptance before selecting the full FSRef chain by
default. The existing Classic large-EOF diagnostic and original-media tombstone
limitations remain. These are outstanding work in the active whole-port goal,
not reasons to mark the goal complete.
