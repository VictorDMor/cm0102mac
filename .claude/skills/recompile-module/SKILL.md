---
name: recompile-module
description: Reconstruct a whole original CM 01/02 module (X.CPP) as handwritten C++23 in this repo, from a closed routine inventory through production routing, original-versus-rewrite checks, delivery and evidence cleanup. Use whenever Victor asks to recompile, rewrite, reconstruct, port, finish or resume a module, names a .CPP module from the progress sheet, or asks to pick the next or a random module, including in Portuguese ("recompila o NEWS.CPP", "reconstrói esse módulo", "pega um módulo", "próximo módulo"). Not for one-off bug fixes, isolated performance tuning or gameplay features.
---

# Reconstructing a module

Deliver the module, not a plan or a set of hooks: ordinary C++23 that leaves
everything the game can observe exactly as the generated reference does.

`CLAUDE.md` owns project policy and overrides this workflow. `docs/cpp-rewrites.md`
is the technical contract: read it through "Checking one" before the first block,
plus the sections of any subsystem this module touches.

## Pick the target

Victor chooses modules, currently at random (decided 2026-10-30). Use the module
he names. If he asks you to pick, or names none, read the
[progress sheet](https://docs.google.com/spreadsheets/d/1nAl1ojgAi6lJlIAllCFNSPQ-I-HfGgG155MeNJ85Fd4/edit)
with the Google Drive connector and draw with `python3 -c "import random; print(random.choice([...]))"`
from the `Módulos C++` rows whose status is not `Concluído`. Skip modules with
uncommitted work (`git status --short -- 'native/<module>*'`) and say which one
you drew. Don't rank by cost or profile candidates first; that isn't the criterion now.

State the module and finish line in one sentence, then start.

## What Victor owns

Career testing, visual and gameplay checks, manual save/reload and new gameplay
captures are his (confirmed 2026-10-30). Don't launch or drive a career, inspect
game screens or record new gameplay captures unless he asks for it on this task.
These are outside your completion gates, so their absence never blocks "complete".
Mention them once in the commit message as not retested, and not as open work.

Your evidence is existing captures plus controlled fixtures and synthetic calls.

## What "complete" means

Report these four separately, with exact counts once the inventory is closed:

- **Inventory:** closed set of original entries with justified module bounds.
- **Implementation:** every entry is complete handwritten C++23, including error,
  legacy, callback and resume paths.
- **Production:** every entry routes to it in the normal app; no owned path falls
  back to a generated body or instruction builder.
- **Verification:** each entry and its reachable behaviors have valid evidence.

Say "100% complete" only when all four close. An unchecked reachable path or a
failing focused check prevents it. Completion describes this audited scope and
evidence, not a proof over every possible execution.

Before you touch anything, note the current `git status`. The tree often holds
someone else's unfinished work, and it must survive your commits untouched.

## 1. Close the inventory

Prove the bounds before quoting any percentage. Reconcile source and string-pool
evidence, TOC, neighboring modules, direct calls, transition vectors, callbacks,
tail calls and reachable interior entries. Separate jump-table labels and shared
code from functions. An address interval or the sheet's estimate alone proves
nothing: its ranges are heuristics, and it says so.

Write the module's audit (`tools/<module>_audit.py` and its test) pinning entries,
ends, digests and production ownership. An audit that skips without its private
export is not a pass.

The closest template is SIPRINT: `native/siprint*`, `tools/siprint_audit.py`,
`tools/test_siprint_audit.py`, `tools/make_siprint_reference.py`, the `CM_SIPRINT`
block in `native/CMakeLists.txt` and its hook in `tools/compile_recomp.py`. Copy
its integration shape. Don't copy an older module's architecture merely because
it is hooked, since builder code is what this work replaces.

## 2. Implement in functional blocks

Read [fidelity.md](references/fidelity.md) before the first block. Group routines
that call each other and share records; keep per-entry characterization inside
the block, and the work list in `TASKS.md`.

Use named record views, typed functions, plain control flow and direct calls
between rewritten routines, with a narrow ABI adapter at the edge. A builder
transcription or a register-array algorithm does not count, and neither does a
call to a generated routine this module owns. Calls to other modules' generated
routines are fine; keep them on an explicit dependency list.

## 3. Check each block against the original

Read [verification.md](references/verification.md) when preparing the first
checks. Run original and C++ from identical states, including at least one
deliberate wrong result the checker must catch. Once a block's comparisons pass,
run a coverage build to find the behaviors they missed, and close those before
calling the block verified.

When a check differs, reduce it to the earliest observable difference and read
the reference, the implementation and the fixture's preconditions before editing.
Never weaken the checker, drop a valid case or refresh an expected snapshot from
the rewrite. If the reference itself differs from PowerPC hardware, fix generator
and rewrites together as `CLAUDE.md` describes. Anything that would change
simulation results or RNG order goes to Victor first; keep working on the rest.

## 4. Integrate

Hook every entry through the generator, regenerate, and check that the changed
page list is what you meant. Never edit generated pages. Remove superseded owned
builder paths from normal routing; reference paths stay for the checks.

Prove normal-app routing, not just checker routing: callbacks, same-page and
direct calls, interior resumes and observer-enabled paths. `--only` and diagnostic
hooks can hide a production gap. `production_mode` and `ownership_mode` in
`native/siprint_checks.cpp` show the pattern: exhaust the instruction budget so
that any generated execution fails.

Then run one original/C++ replay pair (`CM_<MODULE>_ORIGINAL=1` for the original
side) from the same existing capture, and require identical final states. If no
existing capture reaches the module, say so and rely on the fixtures. A capture
that fails even with the module forced to original is stale: don't count it.

## 5. Test only what changed

Run the module's audit and equivalence tests plus whatever your diff can affect:

| The diff touched | Add |
| --- | --- |
| Generated pages, or code watching an exact PC | `generated_block_accounting` |
| Direct calls between owners | `generated_direct_owner_calls` |
| Another module's files | that module's tests |
| The engine or a runtime hot path | `-R match_engine` |

```sh
tools/dev.sh test -R '^(siprint_equivalence|siprint_audit|generated_block_accounting)$'
```

Don't run the full suite (`tools/dev.sh test` with no arguments) and don't build
other modules' checkers: neither says anything about this diff, and each checker
binary costs about 170 MB on a nearly full disk. A failure in a selected test
blocks the push unless `TASKS.md` lists it as known; anything unlisted counts as new.

## 6. Deliver

In this order:

1. **Normal build.** Reconfigure with the coverage option you added OFF, then build
   and rerun the selected tests. A leftover coverage build must not reach production.
2. **Review.** Run the `code-review` skill at `high` on the owned paths, not the
   whole tree, and ask it to compare with the generated reference. Fix what it
   finds and rerun what those fixes affect. Past module reviews caught real ABI
   and return-state defects, so this is worth its cost.
3. **Install.** `tools/dev.sh install`. The installer signs and verifies the
   bundle, so don't add your own signature checks. If the game is running it
   refuses: leave that under **Blocked on me** and never stop his session.
4. **Progress sheet.** Load `anthropic-skills:google-workspace` before editing the
   [sheet](https://docs.google.com/spreadsheets/d/1nAl1ojgAi6lJlIAllCFNSPQ-I-HfGgG155MeNJ85Fd4/edit).
   In `Módulos C++`, fill the module's row as the `CASH.CPP` row is filled: status
   `Concluído`, new routines, audited total, reliability and a short note in
   Portuguese. In `Resumo`, update the date and note. Read it back: formulas and
   neighboring rows unchanged, completed modules up by one, routines up by this
   module's count. If you can't write to it, put that under **Blocked on me** and
   finish the rest.
5. **Docs.** A few lines in `docs/cpp-rewrites.md` for each quirk or contract
   worth knowing; `README.md` only if user-visible status changes.
6. **Commit and push.** Stage owned changes only. Where a shared file also holds
   someone else's hunks, stage yours with `git apply --cached <patch>`, since
   interactive staging isn't available. The commit message is the record: what
   changed, why, how it was verified and what wasn't. Remove your ticked items
   from `TASKS.md` in that same commit, and don't copy evidence into it.
7. **Clean up.** This is part of delivery, because the disk is nearly full and
   evidence piles up fast (9 GB by 2026-10-30, almost all raw coverage profiles):

   ```sh
   python3 .claude/skills/recompile-module/scripts/clean_evidence.py MODULE          # shows what would go
   python3 .claude/skills/recompile-module/scripts/clean_evidence.py MODULE --delete
   ```

   It removes profiles, logs, baselines, the coverage output and the module's
   checker binary, which `tools/dev.sh build cm-<module>-checks` recreates. It
   keeps the ledger and small notes, and refuses a module with uncommitted work.

If a module takes several blocks, each block that passes its checks lands as its
own reviewed commit, even while another gate is still open. Don't mark the module
complete early.

## Using subagents

Small modules don't need any. For larger ones, split independent blocks and match
the model to the work: `haiku` for searches and reading tool output, `sonnet` for
well-specified blocks, `opus` for hard equivalence reasoning. Give each block
exclusive ownership of its files, and check returned evidence yourself: a
subagent's confidence is not proof of equivalence.

Get a fresh `opus` read when targeted inspection leaves competing interpretations
of stack, ABI, memory, floating point, RNG or save behavior, or when the same
divergence survives two distinct evidence-based fixes. Size or compiler errors
are not reasons to escalate.

## Ending the run

Finish with **Blocked on me**, **Changed** and **Found**. Under Changed give the
exact implementation and production counts, which tests passed, what wasn't
verified and whether all four dimensions closed. Don't write separate reports.
