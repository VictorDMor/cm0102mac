# Complete C++23 port: progress and acceptance boundaries

The active objective is to finish the modern C++23 port and the game's native
runtimes. A completed subsystem advances this objective; it does not close it.
The production ARM64 application already runs the statically recompiled Mac
engine. Most of that engine still consists of generated routines rather than
named semantic C++23 operations. The native application and a completely
reconstructed engine are separate claims.

## Current evidence

The accepted production manifest contains **598 semantic entry replacements**.
The latest [record-history report](native-record-history-reconstruction.md)
adds two complete entries / 393 indexed instructions, preserving the accepted
596-entry prefix. Nine focused CTests, 250 generator audits, thirteen storage
regressions and seven affected UBSAN units pass. Individual actual production
checks cover both entries; both also execute in the owned installed menu smoke.
The signed installed ARM64 executable SHA-256 is
`96d8399080b94158afe410834f39b5b7d19e48d815c61e91b9ba2d2a97682723`.
Save/Load is **not retested** and performance is **not benchmarked** in this cycle.

## Historical register-context acceptance

The [register-context report](native-register-context-reconstruction.md) adds all
thirty floating suffix/nonlocal context entries over four shared bodies / 91
unique original instructions, preserving the first 234 replacements exactly.
Generation retains 1,953,334 instructions / 1,922 pages with zero missing/pending
targets. Full Release ARM64 build and checks use the expanded State consistently.

Acceptance includes 2,883 component comparisons, 756 mapping-fault prefixes and
thirty individual negatives; production adds 1,169 comparisons, 598 exact budget
stops, sixty mapping faults, thirty negatives and direct/generated/CFM evidence
for every entry. Eighty nested transfers check abandoned and surviving C++ calls,
recursive/shared frames, copied contexts, peer State and owner expiry. Ten focused
CTests, fifteen affected Python suites / 101 tests with zero skips and fresh
eleven-unit UBSAN pass. All six sanitizer outputs match Release.

At that delivery, the protected installed ARM64 executable SHA-256 was
`05f50e3350ac9244a0796093ddf186e7171f3c5a7e34c954e4d7741e162a42f1`.
Installed menu/chooser/cancel/normal exit records 337 modern captures and one
modern restore/transfer, with zero faults, original selections, escapes or unknown
transfers. The 28 floating suffix UI counters are zero; the harness establishes
individual coverage for them. Save/Load is **not retested** for this block.

The prior arithmetic, exception, allocator, stream, file/path/controller,
fragment, lifecycle and object-storage acceptances remain in their subsystem
reports. Historical manifest totals describe those deliveries. The semantic entry
count is not a percentage of behavior or functions, and the 5,833 analyzed
functions are not a completeness certificate. Whole-port completion remains open.

## Register-context ownership contract

The entire architectural and control-flow block is accepted in production.
Raw guest storage and scoped host routing remain separate. A nonlocal restore
abandons interrupted semantic callers; it does not return through a stale
C++ epilogue. The shared exception continuation regressions also pass. Unknown
or manufactured contexts use a separately counted fallback, while all live
captured-owner transfer acceptance and installed restoration use no fallback.

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
