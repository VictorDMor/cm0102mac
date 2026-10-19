# Generated-source storage and cycle retention

The native-port goal cleans reproducible build outputs after each accepted
cycle. Keep the latest accepted consistent build and the active phase. Preserve
application bundles, original/private inputs, references, receipts, handoffs,
saves and user settings. Build only the explicitly required targets.

Include standalone check executables and their debug-symbol bundles in each
cycle's inventory. Earlier checks also live directly in phase directories or
custom sanitizer folders, so scanning only CMake/build directories misses them.
Use an exact reviewed removal list, verify regular paths and idle processes,
and retain their source, original references, validation logs and build recipes.
Do not classify archived baseline/reference programs as disposable solely by
their size or executable format.

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
