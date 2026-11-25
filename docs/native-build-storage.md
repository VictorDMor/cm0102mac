# Generated-source storage and cycle retention

The native-port goal cleans reproducible build outputs after each cycle. Keep
the latest consistent build, the active phase, installed applications and
necessary references. Preserve original/private inputs, receipts, handoffs,
saves and user settings. Build only the explicitly required targets. Retire an
older generated validation application only after verifying retained source,
original inputs and a build recipe; its age alone is insufficient.

Include standalone check executables and their debug-symbol bundles in each
cycle's inventory. Earlier checks also live directly in phase directories or
custom sanitizer folders, so scanning only CMake/build directories misses them.
Use an exact reviewed removal list, verify regular paths and idle processes,
and retain their source, original references, validation logs and build recipes.
Do not classify archived baseline/reference programs as disposable solely by
their size or executable format.

## Replay capture files

The native capture writer stores each new `.snapshot` as a level-1 Zstandard
frame with a checksum. The payload is the original snapshot format. Match and
world replay readers detect the format from the file header and also accept
existing uncompressed captures. To inspect the original bytes of a compressed
capture, use `zstd -dc initial.snapshot`. The game's career saves use their own
format.

## Cycle storage control on 2026-10-19

The user reaffirmed removal of old artifacts that can be recreated and storage
control after every reconstruction cycle. The accepted 958-owner production
build remains reusable. Older linked checks can be removed from that shared
build while retaining their objects, libraries and exact CMake link recipes.
Current focused actor-action, target-acceptance and range-sampling checks remain.

The reviewed cleanup removes **410 reproducible outputs** in two exact batches.
Transparent macOS compression preserves the original paths and ordinary-read
SHA-256 of **193 snapshots**. All 385 reviewed snapshots are hashed; the other
192 retain their original files because compression copies change their system
provenance attribute. No snapshot is renamed to an archive extension. Existing
readers continue to use the same paths without a decompression step.

Available space is observed at **8,856,231,936 bytes before cleanup** and
**47,479,332,864 bytes afterward**, an increase of approximately **38.6 GB**.
This is an available-space observation, not an exclusive APFS extent calculation.
All 856 protected file hashes and metadata for 2,052 application files are
unchanged. Original inputs, saves, application bundles, reference sources,
validation logs, build recipes and the current production build are retained.

Each following cycle reuses the consistent production build, creates only its
required fixtures, removes reviewed obsolete check outputs and compacts newly
retained snapshots when their content and metadata can be preserved. Inventory
and preservation receipts stay private under the cycle's local storage records.

A subsequent controlled pass compacts the 192 deferred snapshots and fifteen
new factory captures. Exact ordinary-read content, paths, permissions, ownership,
timestamps and all original non-provenance attributes are verified. macOS may
regenerate `com.apple.provenance` on a generated snapshot copy; this is recorded
separately and is not represented as unchanged metadata. Inputs, saves and
application bundles are excluded. All 207 files are compressed, and the actual
C++ loader passes fifteen profile comparisons through the original compressed
paths with output identical to the earlier uncompressed run.

Across these passes, **400 snapshots are newly compressed** and **426
reproducible outputs are removed**. After the 963 technical build, its protected
installation and this cycle's cleanup, available space is observed at
**59,144,859,648 bytes**. Necessary original references, current production and
sanitizer executables, source, logs and recipes remain available. The archived
failing divide-regression executable is retained. Subsequent cycles review
older seeded factory captures for retirement when their producer, parameters,
source revision and original inputs are retained; ordinary full-state copies
must not accumulate indefinitely.

## Independent generated-source reuse

`tools/compile_recomp.py` accepts `--reuse-generated PREVIOUS_GENERATED_DIR`
when its destination is empty. On macOS, `generation_storage.py` uses
`clonefile` to create independent copy-on-write C++ files. APFS can share their
data extents until a destination changes. These are separate inodes, not hard
links; writing a newly generated page leaves the old reference intact.

Every indexed instruction and every generated page still goes through the
existing generator and contract audits. The renderer compares the complete
fresh content with each seeded file and rewrites changed units. This option
reuses storage only; it does not skip generation, auditing or compilation.
Native builds continue to use a fresh consistent set of objects when needed.

Example, with private data kept outside Git:

```sh
python3 tools/compile_recomp.py \
  .local/compiler-arithmetic-reconstruction/input \
  .local/aggregate-object-reconstruction/generated \
  --root 0x10773710 --all-analyzed \
  --reuse-generated .local/tree-mutation-reconstruction/generated
```

The seed validates the completed source manifest, every listed regular C++ file
and the empty destination before copying. It rejects directory/file symlinks,
parent traversal, overlapping directories, duplicate/unexpected filenames,
missing files and incomplete manifests. It copies only listed C++ units and
never copies `compilation.json`. Completion is published by the generator only
after all output succeeds; a failed seed or generation has no completion claim.

Systems without `clonefile`, and filesystems reporting an unsupported clone,
use exclusive independent regular copies and report that mode honestly.
Permission, disk-full and existing-destination errors propagate; they do not
trigger a fallback that could hide the failure or overwrite an existing file.

## Verification on 2026-10-11

Thirteen focused tests pass. They check independent inodes and source immutability
after destination writes, ordinary-copy fallback, unsupported-clone fallback,
failure propagation, preflight rejection, changed manifests, interrupted seeds
and actual CLI regeneration. A deliberately poisoned seed produces the same
bytes as fresh generation; an unsupported operation fails after seeding without
publishing a completion manifest.

Full private production generation cloned all **1,923 C++ files**, with zero
ordinary copies. It regenerated **1,953,334 indexed instructions on 1,922 pages**.
All C++ hashes and the completion manifest exactly match the previous accepted
generation: **573 semantic entries**, the same roots and five source fixups,
zero missing/pending targets. Every cloned source has a separate inode, and every
prior source hash, size, inode and modification timestamp remains unchanged.

The C++ files contain **752,660,018 logical bytes**. APFS shares those extents;
`st_blocks` sums count shared blocks again and must not be described as newly
allocated space or measured savings. Available space showed no decrease during
the run (55.90 GB before, 55.92 GB afterward); other filesystem activity can
affect that observation. No game executable change or runtime-speed gain is
claimed by this build-tool change.

The private active-phase records are `generation-storage-tests.log`,
`generation-storage-full-generation.log`, `generation-storage-source-before.json`
and `generation-storage-receipt.json`. At this storage-only delivery, the installed
signed ARM64 app remained the accepted tree-mutation build. The subsequent
[aggregate-object delivery](native-aggregate-object-reconstruction.md) reuses
that seed for a fresh consistent production build. Save/Load is **not retested**
and performance is **not benchmarked**. The full C++23 port remains active.

## Profile-guided optimisation

`tools/dev.sh pgo` refreshes a Clang profile that `tools/dev.sh build` then uses for the game libraries (`cm_aot`
and what it links), `cm-world-replay` and the app. Everything lives in `.local/current/pgo/` (never committed):
`cm.profdata` and `cm.profdata.ok`, whose first line is the compiler the profile was written for and whose second
says when, at which commit and on which runs.

- `tools/dev.sh pgo` runs the steps `select`, `instrument`, `train`, `merge` and a normal build (about 45 minutes in
  all: the two full builds and the game runs); any step can be run alone and an interrupted `instrument` resumes, as
  it is an incremental build in the same build directory. Since 2026-11-25 the training input is the game itself,
  not replays: replays could not run the parallel modes, the screens or the host UI, and their captures were lost
  with `.local` on 2026-11-24.
  - The workload is `tools/pgo_train.py`: isolated driven copies of the app (`tools/game_driver.py`), each with one
    benchmark save of `.local/benchmarks/saves`. `holiday` is the multi-league career on holiday to day 244 with the
    matches on 8 workers (the daily world, background matches, the shortlist scans and their workers, the holiday
    screens); `season` continues the multi-league career of 14 July 2001 to day 212, playing the human's matches
    (best eleven, Z) and answering the screens on the way. `heldout` (another career, January 2002) is never trained
    on: it is the timing input that shows a profile is not fitted to its training. The same script times a build
    (`--app`, `--repeat`, one line per scenario). `CM_PGO_SCENARIOS` picks the training scenarios.
  - `select` runs the workload on the normal app under macOS `sample` and lists the generated pages whose own code
    takes the most samples on any thread (`tools/pgo_pages.py`, 200 pages or 98%; 200 pages gave 96.7% on
    2026-11-25: most of the time is now in handwritten C++, always instrumented). Instrumenting all 1,922 pages
    doubles their code and the executable no longer links: arm64 branches reach 128 MB, the linker adds one island
    hop, so about 250 MB of code is the limit. The other pages keep their normal code in the training build and get
    none from the profile.
  - `instrument` reconfigures with `CM_PGO_GENERATE=ON` (IR-level `-fprofile-generate`, chosen over front-end
    `-fprofile-instr-generate` because it counts fewer places and optimises better) and builds the app and
    `cm-world-replay`.
  - `train` runs the workload on the instrumented app with `LLVM_PROFILE_FILE=<raw>/<scenario>-%c%p.profraw`: `%c`
    is LLVM's continuous mode, the counters mapped onto the file, so the driver's SIGTERM loses nothing and every
    thread's counts are there (the counters are not atomic; races lose an occasional count).
  - `merge` runs `llvm-profdata merge -sparse` and deletes the raw files.
- `tools/dev.sh build` passes `-DCM_PGO_PROFILE=<cm.profdata>` while the marker names the compiler in use and
  switches `CM_PGO_GENERATE` off whatever the tree held, so an instrumented tree never survives a build;
  `tools/dev.sh install` refuses one. `CM_PGO=off tools/dev.sh build` ignores the profile;
  `tools/dev.sh pgo clear` deletes it.
- The profile is not tied to the sources. Clang drops the profile of a function whose control flow changed (the
  warnings about unprofiled, missing and out-of-date functions are off, and so is -Wbackend-plugin, which an inline
  function instantiated in both Objective-C++ and C++ files raises; the build stays quiet) and optimises it
  as before; refresh the profile after larger changes. A new profile of the same path rebuilds every object.
- Results do not change: the options add no floating-point flag (`-ffp-contract=off` stays on the libraries,
  nothing like `-ffast-math`); check a new profile with a strict replay (same state digest and instruction count)
  and alternating pairs of `tools/pgo_train.py holiday heldout --app ...` against the previous app.
