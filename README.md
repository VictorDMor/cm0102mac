# Championship Manager 01/02 — Apple Silicon native port research

**Status: native development build; core single-player gameplay verified.**
The game runs as ARM64 with no CPU emulator, Wine, or legacy Mac OS virtual
machine. Live checks cover creating/loading a managed game, advancing dates,
completing a match, returning to the club, saving, and normal exit. Multiplayer
and some less-used services remain unfinished. The engine uses original routines
statically recompiled to ARM64, with native macOS integration.

The CD contains a PowerPC PEF executable and data, with no original source files.
The existing Windows ReCM decompilation/reimplementation discovered locally is a
potential engine source. Its full engine does not currently compile for ARM64.
See [the current investigation](docs/native-port-status.md).

The active implementation also explores recompiling the original Mac executable
ahead of time, preserving its simulation code and data layouts. See the
[native recompilation pipeline and its limits](docs/aot-port.md).

## Run the current prototype

Open `CM-01-02.cdr` once, then double-click `Install Native Prototype.command` in
Finder. The installer uses the existing ARM64 build, verifies the supplied Mac
release, and copies 809 files (the game folder plus the CD's `Extras` folder),
preserving resource forks and Finder metadata.
It installs `~/Applications/CM 01-02 Native Prototype.app` and stores game data in
`~/Library/Application Support/CM 01-02 Native/Game`. The original program segments
are bundled with the app. No compilation is required to install the existing build.

After installation, eject the CD image and open the installed application. Its
original opening and main menu have been observed on screen with the CD unmounted.
The original CD check also passed with the image unmounted after installing its
required `Extras/qpr for the cup` file. Database and league initialization completed,
and the user selected Cruzeiro and entered its original squad screen. Opening a
player then exposed the missing `num2dec` service, now implemented and covered by
the original number-formatting routines; the user has now opened a player in the
reloaded game. Save testing exposed two further boundaries: canonically equivalent accented
filenames and final save-file renaming. Both are implemented; the recovered
save also exposed an English club-name ordering mismatch in the supplied data.
A verified private language correction is now installed for new games, and
`Mac-Teste-Recuperado.sav` successfully loaded the managed Cruzeiro squad.
Opening its competition exposed another null background lookup; the installed
correction passes original-routine tests and the user confirmed the competition opens.
The user then saved normally as `Mac-Teste-Validado` and loaded that new file
successfully without repair. A correction to the arithmetic translation allowed
`Continue Game` to advance from 21 December 2001 into January 2002. The user
confirmed advancement. After a second arithmetic correction, a friendly against
Anapolina completed, ending 1–0 at minute 93. Continuing from full time exposed
an audio callback running during sound-data cleanup. The corrected build has
now passed that transition, advanced further, saved normally as
`Mac-Teste-Validado-Pos-Jogo`, and exited normally. The user subsequently
confirmed that this save loads and preserves the Anapolina result in Last Game.
Existing prototype
preferences are copied on first installation. Later installation runs verify
installed data, update the native app, and preserve preferences and saves; they
do not require the CD again. Changed installed assets stop verification rather
than being overwritten.

This remains a development build. Opening takes about 30 seconds on the current
machine. The installed app has no diagnostic instruction limit and records each
run in `~/Library/Application Support/CM 01-02 Native/Runs`; private preferences
and game-created files use the adjacent `Runtime` directory. An unfinished native
service stops execution with a diagnostic message. Day advancement and normal
save/reload, match simulation, and returning from full time are verified.
Multiplayer remains unavailable; its initialization
now returns an explicit unsupported error to the original game.

## Saved game snapshot

`saves/Mac-Teste-Validado-Pos-Jogo.sav` is a Git LFS snapshot of the latest
verified post-match save. It was copied from the installed game's
`Runtime/Files/games` directory; the installed save remains in place. After
cloning, run `git lfs pull` if Git has not downloaded the save automatically.
Other local saves stay outside the repository.

`Run Native Prototype.command` remains a diagnostic alternative using the local
build and installed game data. It limits execution to 10 billion original
instructions by default, writes `.local/prototype-runs/run.XXXXXX`, and leaves
Terminal open after stopping. `CM_AOT_INSTRUCTION_BUDGET=unlimited` removes that
limit. Retained images are application buffers, not physical-screen captures.
Audio transitions are logged; detailed OS-call tracing is opt-in with
`CM_AOT_TRACE_IMPORTS=1` because the original waits make millions of native calls.

## Native asset checks

Requires macOS 13+ on Apple Silicon and Swift 6 Command Line Tools. The executable
checks do not require XCTest, an Xcode application, or a system configuration change.

```sh
swift run cm-assets-checks
swift build -c release
.build/release/cm-assets '/Volumes/CM 01-02/Championship Manager/Data' reports/generated/assets
file .build/release/cm-assets
```

The inspector reads original data and writes its JSON report and PNG previews only
to the output directory. It reports unsupported image variants explicitly. The
images are previews of disc assets; they are not screenshots of a running game.

## Disc audit

With the supplied image already mounted:

```sh
python3 tools/audit_media.py '/Volumes/CM 01-02'
```

This records PEF sections, imports, executable and resource-fork checksums, file
inventory, and source filename hints. Hints embedded in compiled code are not
source files. Generated reports and all proprietary media stay untracked.

## Existing Windows source: native compatibility check

Requires CMake 3.28+ and Apple Clang with C++23. Set the source path explicitly:

```sh
cmake -S . -B .local/native-checks -DRECM_SOURCE_DIR='/path/to/recm-0102'
cmake --build .local/native-checks
ctest --test-dir .local/native-checks --output-on-failure
```

This compiles the existing record-layout tests into ARM64 without changing the
source project. It does not compile or execute the whole game. Passing these
tests does not establish compatibility with Mac `.dam` tables.

## Mac binary analysis

```sh
JAVA_HOME='/path/to/jdk/Contents/Home' bash tools/analyze_mac.sh '/path/to/ghidra' '/Volumes/CM 01-02'
```

Ghidra 12.0 with its ARM64 decompiler was used locally. The wrapper selects the
correct PowerPC language explicitly and preserves the project under `.local`.
The analysis exports function metadata and entry-point pseudocode. It produces
analysis material, not reconstructed, buildable game source.
