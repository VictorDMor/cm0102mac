# CM 01/02 for Apple Silicon

A native ARM64 macOS port of **Championship Manager 01/02**, built through static
recompilation of the original PowerPC Mac executable and ongoing C++23
reconstruction of its routines.

**Status: playable development build, actively being developed.** Core
single-player gameplay has been verified in local use: creating and loading a
career, managing a club, advancing dates, completing matches, returning to
management, saving and reloading. Multiplayer and some less-used services remain
unfinished. There is no public binary release yet.

## How it works

- A custom Ghidra-based pipeline translates the original 32-bit PowerPC program
  into C++, which Apple Clang compiles ahead of time for ARM64.
- Native macOS bridges provide windows, input, drawing, audio and file services.
- Reconstructed C++23 subsystems progressively replace generated routines while
  preserving calling conventions, data layouts and observable effects.
- The runtime contains no CPU interpreter or JIT and runs without Wine or a
  legacy Mac OS virtual machine. It retains a checked, big-endian 32-bit data
  address space for the original game's pointers and structures.

The latest accepted production manifest contains **685 reconstructed entry points**.
This is an integration count, not a percentage of the game recovered. Much of the
simulation still uses generated code. See the
[C++23 progress report](docs/native-cpp23-port-progress.md) and
[current status and limitations](docs/native-port-status.md).

## What is included

| Area | Contents |
| --- | --- |
| Native game | ARM64 runtime, macOS bridges and application shell |
| C++23 reconstruction | Runtime, calendar, storage and simulation subsystems with focused equivalence checks |
| Match Debugger | Native match inspection, deterministic scenarios and history controls |
| Recompilation tools | Ghidra exporters, indexed code generation and reference harness generators |
| Asset inspector | Optional Swift tools for examining locally supplied game assets |

This repository contains project source and documentation. Original game files,
generated program payloads, saves, application bundles and private validation
artifacts are excluded. **You must supply your own original Mac release.** The
Windows executable is not supported by this pipeline. Original game content
remains the property of its respective owners.

## Supported original release

The current pipeline supports the **original PowerPC Mac version 3.9.62**,
published on Macintosh by Feral Interactive. Both `vers` resources in the audited
executable identify version 3.9.62. The installer checks the executable and its
resource fork before copying game data.

| Original executable | Expected value |
| --- | --- |
| File | `Championship Manager/CM 01-02` |
| Data fork size | 9,394,972 bytes |
| Data fork SHA-1 | `a767c5b8edc139598c28afb16e404435d4877024` |
| Data fork SHA-256 | `91692f95df17cf851e7c1d6dbdecd0bd0a5809ce1937133e209aee859a6b516b` |
| Resource fork SHA-256 | `1f4ae4f499d43f163b9153026939b19a2ad022a8b4181690ba7b1b49cf5e551c` |

These hashes identify the executable's forks, not a disc image or a native build.
The game is catalogued as
[Championship Manager: Season 01/02 (Wikidata Q1037535)](https://www.wikidata.org/wiki/Q1037535).
Other original releases, Windows executables and community patches have not
been accepted as inputs to this pipeline.

## Features and current limits

| Area | Verified behavior and limits |
| --- | --- |
| Career play | Create/load a career, manage a club, inspect players and competitions, advance dates, play complete matches and return to management |
| Persistence | Normal save, exit, reopen and reload have passed in local use; individual later subsystem reports state when persistence was not retested |
| macOS presentation | Native text/control drawing, 100–200% interface scaling and Fit to Screen, keyboard commands and audio |
| Match Debugger | Separate native application with seeded matches, player/event inspection, substitutions and rewind/redo; its full product contract remains incomplete |
| Reconstruction | 685 accepted C++23 entry points coexist with statically generated original routines; this count is not a completion percentage |
| Availability | Developer source workflow for Apple Silicon / macOS 13+; your original Mac files and local compilation are required |
| Remaining work | Multiplayer and some services remain unsupported; the whole game port is unfinished and no public binary release is available |

## Developer build

The current workflow targets **Apple Silicon and macOS 13 or later**. It requires
Apple Command Line Tools with C++23 support, CMake 3.28+, Python 3, and Ghidra with
a compatible JDK. Ghidra 12.0 with its native ARM64 decompiler was used for the
recorded builds. Swift 6 is needed only for the optional asset inspector.

With your original Mac media mounted, analyze it and build locally:

```sh
git clone https://github.com/VictorDMor/cm0102mac.git
cd cm0102mac

JAVA_HOME='/path/to/jdk/Contents/Home' bash tools/analyze_mac.sh \
  '/path/to/ghidra' '/Volumes/CM 01-02'
JAVA_HOME='/path/to/jdk/Contents/Home' bash tools/build_aot.sh \
  '/path/to/ghidra'
```

The build script exports and indexes the analyzed program, generates C++, builds
the native targets and runs the configured checks. Generated inputs and builds
remain under `.local/`. Initial compilation covers roughly 1,922 translation
units. This is a development workflow; unsupported original addresses or native
services stop with explicit diagnostics.

After building, double-click `Install Native Prototype.command` in Finder with
the original media still mounted for the first installation. The installer
verifies the supported executable and resource fork, copies the required data,
and installs `~/Applications/CM 01-02 Native Prototype.app`. Game data and saves
use `~/Library/Application Support/CM 01-02 Native/`. Later installations verify
the existing data and preserve saves.

The installer requires your local build; cloning this repository alone does not
provide a prebuilt application. See the
[recompilation pipeline](docs/aot-port.md) for technical details and
[Match Debugger documentation](docs/native-match-debugger-application.md) for its
separate development workflow.

## Validation and contributions

Reconstruction work is accepted in bounded subsystems. Reports distinguish
component comparisons against original routines, production integration, installed
application checks and real-match evidence. Passing an isolated comparison does
not mean that a subsystem is installed or that the whole port is complete.

Useful contributions include reconstructing remaining simulation routines,
characterizing original behavior, improving macOS compatibility and reproducing
gameplay regressions. Open an issue describing the behavior, macOS version and
source revision. Keep original game content, saves and private runtime files out
of issues and pull requests.

For the optional asset inspector, independent checks can run without game data:

```sh
swift run cm-assets-checks
```

The root CMake project is an older, optional comparison against an external ReCM
checkout; the active game build uses `native/` through `tools/build_aot.sh`.

## Project reports

- [Native port status](docs/native-port-status.md)
- [C++23 reconstruction progress](docs/native-cpp23-port-progress.md)
- [Static recompilation architecture](docs/aot-port.md)
- [Match Debugger](docs/native-match-debugger-application.md)
- [Native presentation and commands](docs/native-presentation-and-direct-commands.md)
- [Latest accepted ball-action reconstruction](docs/native-ball-action-reconstruction.md)

Individual reports retain the evidence and limitations of their delivery dates.
The root [`.recomp.json`](.recomp.json) supplies the project's listing metadata
using the [recomp.board specification](https://recomp.fyi/spec).
