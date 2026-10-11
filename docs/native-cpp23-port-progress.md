# Complete C++23 port: progress and acceptance boundaries

The active objective is to finish the modern C++23 port and the game's native
runtimes. A completed subsystem advances this objective; it does not close it.
The production ARM64 application already runs the statically recompiled Mac
engine. Most of that engine still consists of generated routines rather than
named semantic C++23 operations. The native application and a completely
reconstructed engine are separate claims.

## Current evidence

The accepted production manifest contains **234 semantic entry replacements**.
The [compiler arithmetic report](native-compiler-arithmetic-reconstruction.md)
adds eight complete entries / 412 original instructions, preserving the first
226 exactly. Generation has 1,953,334 instructions, 1,922 pages and zero
missing/pending targets. A fresh full Release ARM64 build and all affected checks
use the expanded State layout consistently.

Arithmetic component acceptance has 13,199 comparisons, 77 mapping-fault prefixes,
two enabled traps and eight rejected negatives. Production adds 7,422 comparisons
and 4,895 exact budget stops, normal direct/generated/CFM witnesses for every
entry, independent selection/reset and a separate 16-case original-PC profiling
check. Ten focused CTests, 20 Python suites / 127 tests with zero skips, and
fresh ten-unit UBSAN pass. The protected installed menu/chooser/cancel/normal-exit
smoke succeeds. Unsigned quotient records 12,125,476 modern calls/returns without
faults or original selections; the other seven UI counters are zero. The harness
supplies individual evidence for all eight entries. Save/Load is not retested
for this bounded block.
The installed internal ARM64 executable SHA-256 is
`6814c65faad2615a547a5d3ce97a3b6f840ec20c58bd09c7cac8b7a76ae3beb3`.

The prior [exception report](native-exception-runtime-reconstruction.md) remains
the acceptance record for its 21 complete entries / 1,657 instructions. Its
read-only recovery added 177 previously unindexed instructions while preserving
all earlier records and payloads. Earlier allocator, stream, file/path/controller,
fragment metadata, lifecycle and object-storage acceptances likewise remain
in their subsystem reports. Their historical manifest totals describe those
deliveries rather than the current 234-entry baseline.

The semantic entry count is not a percentage of behavior, functions or
instructions. The earlier 5,833 analyzed functions are an analyzer result rather
than a completeness certificate. Native file/fork/Unicode services are separate
bridge evidence and do not increase the semantic entry manifest. The complete
port objective remains active.

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
