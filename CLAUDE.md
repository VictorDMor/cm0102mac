# CM 01/02 native port: how we work

Championship Manager 01/02 for Apple Silicon. The original 32-bit PowerPC Mac
executable is statically translated to C++ (Ghidra export → `tools/compile_recomp.py`
→ Clang), runs on native macOS bridges (`native/platform.cpp` and friends), and
hand-written C++23 replacements gradually take over generated routines. README.md
has the overview; `docs/aot-port.md` explains the pipeline.

## Working style

When a step doesn't need my input, keep going. Put status notes in the same message as your next action.
Stop and ask only when you can't continue without me, or before anything destructive: deleting data, force-pushing, or changing anything outside this repository. Installing the app with `tools/dev.sh install` is routine and doesn't need asking.

- If a task has no finish line, state the one you'll use in one line, then start.
- On a run with several steps, keep the work list in `TASKS.md`: tick items when done, add what you find, and move decisions you need from me to its top section.
- Split audits and broad reviews across subagents, and check each subagent's evidence before you accept it.
- Before committing a change to the generator, the runtime, a semantic replacement, or file/save handling, run a review pass on the diff (the code-review skill) and fix what it finds.
- Mark anything you couldn't confirm, and say where you looked.
- Talk to me in the language I write in. Code, comments, docs and commit messages are in English.
- End every run with three headings: **Blocked on me**, **Changed**, **Found**. Under Changed, say how each change was verified and what wasn't.

## Rules that don't bend

- The game must behave as the original: game-visible memory, RNG draw order, save format and UI. For reconstruction work the generated code is the reference. Where the reference itself differs from PowerPC hardware (listed in `TASKS.md`), don't change it silently.
- Ask before a change that alters simulation results, RNG order or instruction accounting relative to the reference. Keep working on other items while you wait.
- Fix the generator or runtime; never hand-edit generated pages, which are regenerated.
- Don't touch original game files, the reference save in `saves/` (Git LFS; test on copies), or `~/Library/Application Support/CM 01-02 Native` except through the installer and normal play.
- Never commit original game content, exports, generated C++, binaries, saves other than the LFS reference, logs or captures. They live in `.local/`.
- No `-ffast-math` or anything else that relaxes floating point.

## Build, test, install

Everything uses `.local/current/{export,generated,build}`; machine settings (Ghidra, JDK) are in `.local/current/env`.

| Command | When |
| --- | --- |
| `tools/dev.sh export` | After editing `tools/discovery_roots.txt` or `tools/ExportRecomp.java`. A few minutes. |
| `tools/dev.sh generate` | After changing the generator or its audits. Rewrites only changed pages and lists them; check the list matches what you meant to change. |
| `tools/dev.sh build [targets]` | Incremental build of the app and every test binary already built. Name a target (`cm-platform-checks`) to build one more. |
| `tools/dev.sh test [ctest args]` | No arguments: every test that can run here (38 of 288 today). Use `-R` while iterating. |
| `tools/dev.sh install` | Installs the built app; refuses while the game runs; saves and game data are preserved. |

Editing `native/aot_runtime.hpp` or another header the generated pages include recompiles ~1,900 pages. Don't build every test target: each test binary is ~170 MB and the disk is nearly full.

## How much verification

| Change | Verify with |
| --- | --- |
| Docs, scripts, tooling | Run the script, or its tests if it has them |
| macOS bridges, installer, UI host code | Build; the area's tests; add a regression test when the bug can be reproduced without the game |
| Generator, runtime, semantic replacements | Regenerate if the generator changed; build; the area's equivalence/profile tests; `-R match_engine` when the engine or a runtime hot path changed |
| Performance | All of the above, plus alternating paired timing on the same captured input (at least 5 pairs). Keep only measured wins; put the numbers in the commit message |
| File or save handling | All of the above, plus one save and reload on a copy of a career. If you can't drive the UI, put it under Blocked on me |

Before pushing, run `tools/dev.sh test`. A new failure blocks the push; known failures are listed in `TASKS.md`. Check the real game only for paths tests can't reach, and use a copy of a career.

## Recording work

- The commit message is the record: what changed, why, how it was verified, and what wasn't verified. Don't write separate delivery reports.
- Commit each verified change to `main`. Push normally once the suite passes. Never force-push or rewrite history.
- Update a doc only when the behaviour it describes changes, or when you learn something non-obvious (a quirk of the original, a contract). A few lines in that subsystem's doc. Update README.md when user-visible status changes.
- Skip the old rituals: hash inventories of protected files, per-delivery install scripts, Mach-O section comparisons, handoff prompt files, and repeating the same status paragraph across several docs.

## Older material

`docs/features/*-prompt.md` and the long `docs/native-*.md` reports come from the earlier process. Read them for background on a subsystem; their acceptance rules no longer apply.
