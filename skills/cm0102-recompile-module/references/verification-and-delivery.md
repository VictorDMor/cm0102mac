# Verification and delivery

Inspect the current checker options and test registrations before adapting
examples. Use `.local/current/{export,generated,build}` and `tools/dev.sh`;
avoid new build trees or unrelated large checker binaries.

## Establish a trustworthy checker early

Use SIPRINT for audit/reference integration and MEDIA for richer checking.
Before scaling beyond the pilot, establish:

- Original bypass includes nested owned calls; neither side substitutes an owned
  routine with a fixture answer or routes both sides to the C++ implementation.
- Parent checker selection and excluded entries reach the intended dispatcher;
  direct internal calls do not silently evade per-entry checks. Preserve normal
  direct calls outside checker mode.
- Identical input state includes memory, heap, registers, RNG, imports and
  required low memory. Fixture preconditions come from the reference; arbitrary
  zero/null pointers are not valid error-path evidence.
- Comparisons cover observable memory/stack, heap effects, results, later-read
  registers, FP status and ordered call/import arguments. Scripted dependencies
  preserve specified side effects and reject wrong arguments/order, rather than
  returning canned answers that hide a broken ABI.
- Negative controls actually reach the target and detect a wrong result and
  relevant trace defects. Check their diagnostic and convention for expected
  success/failure; a crash, timeout or zero compared calls is not detection.

`tools/call_arguments.py START END --export EXPORT` derives r3-r10 read masks.
It is an aid, not proof of all ABI dependencies: inspect unknown/indirect/import
targets, FPRs, varargs and spilled registers. An absent mask is unknown, not an
empty argument list. Compare call targets, counts, order, LR/call sites and
observable registers, not merely return state. MEDIA's dispatch trace misses
some direct/same-page calls and pairs only shared sites; audit those omissions
or extend tracing. Zero findings alone cannot establish trace completeness.

## Cases, mutations and evidence classification

By final module closure, compare every entry and run all rewritten entries
together, without `--only`. This is a closure gate, not a per-routine cadence.
Use existing captures for normal state and controlled fixtures for uncovered
branches: empty/single/full collections, limits, aliases, sentinels, numeric
edges, callbacks, resumes, legacy versions, partial IO and allocation/assertion
paths where applicable. Synthetic calls can reuse real capture state; see
`--synthetic`, `m32=` and `expect32=` in `docs/cpp-rewrites.md`.

Mutate initialized scalar inputs and relevant records with a recorded seed and
case IDs. Preserve pointer relationships and preconditions unless the reference
explicitly handles invalid values. Count completed valid mutations separately
from faults, timeouts and partial stops. Random volume does not replace targeted
cases or behavioral coverage; a reproducible failing seed is a regression case.

Classify results explicitly:

| Result | What it establishes |
| --- | --- |
| Returned comparison | Both sides completed and required state/effects matched for that case. |
| Uncaptured import stop | Compared prefix only; suffix stays open until a completed or scripted case covers it. |
| Internal fault | Failed or invalid case to diagnose, even when exception text matches. |
| Expected original error/exit | Valid only with its boundary, effects and termination established and checked on both sides. |
| End-state replay | Integration evidence, not every entry or branch. |
| Skipped/unavailable/not retested | No passing result for that check. |

Never refresh `expected.snapshot` from the rewrite. For a proven reference
correction, retain provenance and establish the replacement expectation from
the corrected original route under the project's simulation/RNG approval rules.

## Production routing and existing replays

Inspect generator ownership, adapter defaults, draft/disabled lists, owned
direct calls and transfer paths. Check normal routing with checker/probe hooks
disabled and observers enabled as applicable. SIPRINT's `production_mode` and
`ownership_mode` illustrate exhausted-instruction-budget controls: C++ must
enter through the normal route, while its forced-original control must stop.
A stop at a legitimate external generated dependency is not owned fallback;
prove the owned path through that boundary separately. A prologue write or a
fault on artificial inputs alone proves neither full execution nor equivalence.

Use the sweep instead of writing another replay loop:

```sh
python3 tools/<module>_audit.py --entries > .local/rewrites/<module>/entries.txt
python3 tools/replay_sweep.py census .local/rewrites/<module>/entries.txt
python3 tools/replay_sweep.py pair CM_<MODULE>_ORIGINAL --captures CAPTURE...
```

Confirm the original override is read by the adapter and unset diagnostic
overrides on the normal side. Inspect module-specific evidence that the capture
actually exercises the rewrite. `MATCH` checks recorded end states, including
the production `--timing` route; instruction-count changes are only a proxy for
execution. `SAME` does not establish rewrite execution, `STALE` fails on the
original, and `DIFFERS`/`TIMEOUT` require diagnosis; none is an equivalence pass.

A census misses direct/same-page calls and cannot see inside a failing capture.
Before concluding no existing capture reaches the module, try likely captures
with routine checking:

```sh
.local/current/build/cm-world-replay CAPTURE 1 \
  --check-routines 200 500 --only 0xENTRY --compare-f1
```

This samples the first 200 calls and every 500th afterward; report the sampling.
Use exhaustive focused comparisons where feasible and the final combined run.
Run a reached original/C++ pair with matching final state on a usable capture.
If none reaches the module, construct a controlled replay fixture with valid
state and an expectation established from the original, then run it as an
original/C++ paired replay. Execute the combined owned call graph on both routes
from identical state and compare the complete final observable state, not just
independent entry returns. Record the existing-capture replay as unavailable
and the controlled replay's exact scope. If no valid paired replay can establish
integration, verification stays open. Do not require Victor's new gameplay
captures or claim that an unreached pair checked the module.

## Coverage, provenance and disk use

Instrument only the module's checker objects, using `CM_SIPRINT_COVERAGE` as a
current CMake example. Normal builds must not depend on coverage symbols.
Forked children must flush their profiles; absent, incomplete or mismatched
profiles mean unknown coverage. Profiles must use the same code/configuration.

Map uncovered source branches to original behaviors. Each reachable outcome
needs a valid check or a specific proof (dominating condition, immutable data,
gate). Compiler-generated branches need a mapping. One call per entry and 100%
line coverage do not close behavioral paths. Prioritize simulation/RNG/save
gaps; batching coverage is a cost saving, never permission to leave gaps open.

Check available disk before a large run. Once a successful merge is verified,
remove only its reproducible raw profiles; keep the merged profile until the
relevant closure/review is done. Do not erase another active run's evidence.

Keep a compact `.local/rewrites/<module>/ledger.json`: revision and relevant
dirty source/configuration identity, per-entry implementation/production/check
status, exact commands/seeds and concise results, open paths/proofs and external
dependencies. Preserve counts/proofs and rerun recipes after deleting bulky
outputs. Old results are reusable only when their affected inputs/code remain
valid; do not copy historical logs into a new passing total.

## Select checks from the diff

At block/module closure, run the module's applicable audit and equivalence
checks. While implementing a routine, select its focused cases and affected
callers; do not automatically repeat every completed routine. Add only affected
checks:

| Changed area | Additional verification |
| --- | --- |
| Generator output or exact-PC observers | `generated_block_accounting`; regenerate after exact-PC changes. |
| Direct calls across owners | `generated_direct_owner_calls` and affected routing checks. |
| Another module | Its affected focused checks. |
| Engine or runtime hot path | `-R match_engine`. |
| File/save handling | Exact bytes/counts, legacy and partial/error paths; controlled persistence checks as necessary. |
| Performance claim | At least five alternating paired runs, identical inputs/final states and final build configuration. |

Use an explicit CTest selection, for example:

```sh
tools/dev.sh test -R '^(siprint_equivalence|siprint_audit|generated_block_accounting)$'
```

Inspect output to confirm selected tests exist and ran; zero tests or skips do
not pass. Do not run the full suite or build unrelated checkers for routine
delivery. Reconcile a failure with the exact known issue in current `TASKS.md`;
new failures block the push. Rerun after changes that invalidate the evidence,
not merely to accumulate repeated green runs.

## Review and deliver

1. **Normal configuration.** Turn temporary coverage/diagnostic defaults OFF,
   build the normal ARM64 app and rerun selected tests. Do not ship a coverage
   build or impose manual career/visual/save-reload QA as an agent gate.
2. **Scoped review.** Review owned code and checker against the generated
   reference for fidelity, architecture, routing and valid evidence. Use the
   required code-review skill if available; otherwise disclose the limitation
   and perform an explicit scoped diff review. Fix findings and repeat affected
   checks/review as warranted. Batch related stable blocks; no automatic Astra
   review of every module and no review of unrelated dirty work.
3. **Guarded install.** Use `tools/dev.sh install`; its signing/verification is
   authoritative. Do not repeat protected-file hashes, Mach-O comparisons or
   custom install/signature rituals. If the running-game guard refuses, leave
   installation pending without terminating Victor's session.
4. **Progress sheet.** Use connected Drive/Sheets tools and their skill. Locate
   the existing module row, read its current values/formulas, update verified
   counts/status/reliability and a concise Portuguese note, then read back the
   changed cells, neighbors, formulas/errors and affected summary totals.
   Derive summary changes from the previous row; do not assume +1 if it was
   already complete. Unavailable access leaves the tracker pending, not passed.
5. **Records and commit.** Update docs only for a useful contract/quirk and
   README for changed user-visible status. The commit records what/why/checks/
   limits. Stage only owned hunks; use `git apply --cached` or reconstruct a
   shared file's intended index content from `HEAD` when needed. Preserve
   unrelated working-tree edits. Commit verified blocks to `main`, push normally
   after required checks and verify the remote revision. Never amend others'
   work, force-push or commit private game artifacts. Remove only your completed
   `TASKS.md` items; keep pending installation/tracker steps tracked after a
   verified commit/push, preserving others' entries.
6. **Cleanup.** Once required checks, review and commit/push are complete,
   preserve whatever is needed for pending installation/tracker work. Then
   inspect the repo helper and preview
   `python3 .claude/skills/recompile-module/scripts/clean_evidence.py MODULE`.
   Use its module-scoped `--delete` only for reviewed reproducible outputs
   belonging to this completed run. Never use `--all` as routine delivery or
   infer inactivity from clean sources alone. Preserve original inputs,
   captures, saves, compact proofs/ledger and evidence still needed by others;
   if the helper cannot separate them, leave those paths and report why.

## Keep usage proportional

Before adding or repeating a costly test layer, check for overtesting: what
specific behavior or integration risk does it cover, is that already established
by valid evidence, and what changed to require another run? Keep this reasoning
brief in the existing work notes; do not create another report or approval gate.

- Prefer behavior partitions and targeted edge/callback/RNG cases over large
  Cartesian products of equivalent inputs. More cases or a coverage percentage
  alone is not additional proof. Retain required behavioral coverage and useful
  seeded mutations without inventing a quota per routine.
- Preserve valid evidence across iterations. A change to one routine invalidates
  its affected checks, not automatically the entire module or engine suite.
  Rebuilds, documentation edits and unrelated changes alone do not justify
  replaying the same full corpus.
- At stable block closure, combine necessary comparisons, mutations, traces and
  coverage where possible. Do not run dedicated, prefix and combined suites
  repeatedly when they exercise the same contract. Keep a separate test only
  when it detects a distinct failure that the combined check cannot.
- Verify the final normal configuration with relevant routing/integration
  checks. Turning coverage off alone does not require rerunning every synthetic
  case on both routes; reuse instrumented semantic evidence when the underlying
  implementation and inputs are unchanged. Still prove normal production routing.
- Migrate legacy tests only where ownership changes affect a live supported
  route. Preserve their meaningful contracts; do not restore obsolete instruction
  counts, translated temporaries or disabled-hook assumptions just to make them
  pass. Avoid expanding a focused rewrite into a broad legacy test renovation.
- Once required focused and closure checks pass, proceed to delivery. Add a new
  round only for a new change, failure or concrete unresolved concern. Maintain
  the final whole-module gate and required relevant integration checks; reducing
  duplication must not hide differences, missing paths or invalid fixtures.

Pilot the checker before a large conversion. Read relevant originals in bounded
chunks and retain concise contracts to avoid repeated discovery. Reuse domain
helpers and local scripts for repetitive extraction, summaries and long runs.
A script may draft repetitive code, but the
landed algorithms still require readable C++ and reference-based verification.
Avoid repeated full exports/generation, broad scans and redundant reviews.

Batch the expensive all-entry/mutation/trace/coverage run on stable code, then
fix the concrete gaps. Do not import Claude token estimates or Sonnet/Opus
routing into Codex; estimate only from relevant measured evidence. Ordinary
modules stay with Sol under `AGENTS.md`; delegation never substitutes for local
checks. If Victor limits usage, deliver the verified subset and record the
remainder without calling the module complete.
