# Complete C++23 port: progress and acceptance boundaries

The active objective is to finish the modern C++23 port and the game's native
runtimes. A completed subsystem advances this objective; it does not close it.
The production ARM64 application already runs the statically recompiled Mac
engine. Most of that engine still consists of generated routines rather than
named semantic C++23 operations. The native application and a completely
reconstructed engine are separate claims.

## Current evidence

The preceding accepted baseline is commit `7826fad7667398eeaaa404ca68dc707672c75c77`:
127 semantic entry replacements and the installed allocator acceptance described
in [the allocator report](native-allocator-pools-reconstruction.md).
The private indexed original contains 1,953,157 instructions; the semantic entry
count is not a percentage of engine behavior, functions or instructions.
The earlier 5,833 analyzed functions are an analysis result, not a completeness
certificate.

The current accepted manifest contains **149 semantic replacements**. The exact
prior 127 gained thirteen storage/lifecycle entries covering 541 indexed
instructions, followed by nine byte/item-I/O entries covering 682 instructions.
The [storage report](native-stream-buffer-reconstruction.md) records 234
comparisons and installed menu/chooser/normal-exit proof, with persistence not
retested for that bounded block.

The [I/O report](native-stream-io-reconstruction.md) records 226 comparisons,
27 expected faults, 13 rejected negatives, six accepted focused CTests, seven
new audits plus 60 tests in affected prior audits, and three focused UBSAN passes.
A private signed copy using identical compiled code loaded Reading, saved through
the original in-game menu, exited, reopened and loaded the new streamio save.
It retained the observed club/manager/date/squad and preserved the original save.
This isolated runtime witness used the existing diagnostic event-coordinate click
mode because CUA background input carried Command/live-cursor differences. It is
not installed-production Save/Load or complete serialization evidence.

File seek/tell, open/mode parsing and remaining file services are next. They remain
generated productive dependencies even though the isolated career used them
successfully. Native input validation limitations are also retained as an evidence
boundary rather than being hidden by the test mode.

## Work that still prevents completion

| Area | Current authoritative boundary | Evidence required for completion |
| --- | --- | --- |
| C/C++ runtime | Allocator and text/number blocks are semantic; many stream, file, sorting, lifecycle and numeric routines remain generated | Named implementations, pinned Mac contracts, ABI/effect/fault comparisons, discriminating negatives and production dispatch evidence for each migrated entry |
| Simulation engine | Management, leagues, competition, database, transfers and match orchestration largely remain statically recompiled | Complete coherent semantic subsystems and original-compatible composition; component checks alone do not establish game behavior |
| Native platform | `native/platform.cpp` explicitly returns Open Transport unsupported initialization; print-job and less-used service coverage is incomplete | Real native services with original Mac ABI/error/lifetime behavior and exercising their actual product flows |
| Whole-game compatibility | Historical live evidence covers core single-player actions and a post-match save; it is not current whole-release coverage | Current installed-artifact identity plus appropriate management, simulation, competition, persistence and supported multiplayer evidence after affected subsystems migrate |
| Generated engine retirement | Generated bodies remain the productive implementation of unmigrated routines and the diagnostic oracle | All required productive engine paths have semantic owners before generated bodies can be removed from production |

This is an evidence ledger, not a reduced definition of completion. Unlisted
services remain subject to the full objective. Native import string presence,
an entry manifest, a source inventory or a green focused suite cannot prove that
the whole game is complete. Newly exposed dependencies are added to the work
as their actual callers and contracts are characterized.

## Execution discipline

Work proceeds through coherent subsystems with shared final acceptance, while
retaining individual ABI, ordered guest effects, negative controls and normal
execution witnesses. Preserve the original Date leap asymmetry and the productive
allocator progress guard. Preserve user saves, preferences, original payloads,
private handoffs and earlier reference artifacts. Implementation artifacts and
technical reports are in English; discussion remains in Portuguese.

Use focused checks during migration and broaden validation when a changed
dependency or an actual divergence warrants it. Earlier bounded Save/Load waivers
remain `not retested`; they are not passes and do not prevent the broader objective
from requiring persistence evidence when the migrated subsystem affects it.
Install only technically accepted artifacts, with a protected reference and
verified internal executable identity. Commit and push reviewed product changes,
excluding original listings, generated sources, payloads, runtime artifacts and
private prompts. Keep the overall goal active until current evidence proves
the complete requested end state.
