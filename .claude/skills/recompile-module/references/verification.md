# Verification

## The checker

```sh
tools/dev.sh build cm-world-replay
.local/current/build/cm-world-replay CAPTURE 1 \
  --check-routines 200 500 --only 0xENTRY --compare-f1
```

This samples the first 200 calls and every 500th after, so it doesn't check every
call. Check exhaustively on small focused cases, and state the sampling for large
replays. `--only` isolates entries, so also run the final combined configuration.
`--census FILE` is a separate run. Same-page and direct calls can escape it, so
zero hits don't prove a routine unreachable.

Every module also needs its own checker with a true original bypass, nested owned
calls included. Comparing two routes to the same C++ body verifies nothing.

Compare observable memory, heap effects, results, later-observed registers, FP
status and call or import arguments and order. Know what the checker compares:
tell an unobservable scratch difference from a real dependency with evidence, not
a blanket ignore list.

## Reaching paths no capture reaches

No new gameplay captures unless Victor asks. Use:

- **Synthetic calls on an existing capture:** `--synthetic FILE` (syntax in
  `docs/cpp-rewrites.md`) runs a target in real state, with `m32=` overrides and
  `expect32=` final-memory requirements.
- **Fixtures in the module's checker:** empty, single and full collections, loop
  limits, sentinels, aliases, integer and FP edges, callbacks, resumes, legacy
  versions, short or partial IO, allocation failures and assertions, as they apply.

Derive pointers and preconditions from real state and the reference. A fault
caused by a fabricated invalid pointer is not an error-path test.

## How to count evidence

Report these separately, because they prove different things:

- **Returned comparison:** both sides finished and matched. Only this closes a path.
- **Stopped at uncaptured import:** proves the prefix only. The rest stays open
  until a completed case or a scripted result covers it.
- **Internal fault:** a case to diagnose, even if both messages match.
- **End-state replay:** integration evidence for the parent, not coverage of every
  entry below it.
- **Skipped or unavailable:** no result.

Show the oracle can fail: `--check-routines-corrupted`, or a deliberate wrong
result in the fixture, after confirming the target really was checked. Revert it
before normal checks. Never refresh `expected.snapshot` from the rewrite under
test. If a capture predates a proven reference correction, establish the new
expectation from the corrected original path and follow the rules on simulation
and RNG changes.

## Coverage

Call counts don't close branches, and neither does 100% line coverage. Use coverage
to find holes, then map them back to original behavior.

Copy the `CM_SIPRINT_COVERAGE` block in `native/CMakeLists.txt` so that only this
module is instrumented, then configure with it ON in a checker-only build. Forked
children must flush their profiles. An absent or incomplete profile means unknown
coverage, not covered, and profiles from different code or configurations don't
merge.

Raw profiles reach 1.5 GB each. Merge them with `llvm-profdata` as soon as a run
ends and delete the raw files, keeping just the merged result for that code state.
Check `df -h .` before a coverage run, and clean first if under 10 GB is free.

Each reachable outcome gets a check or a specific proof that the original can't
reach it (dominating test, immutable table, gate). Compiler-only branches need the
mapping stated, not an invented test.

## Ledger

Keep `.local/rewrites/<module>/ledger.json` small. Results belong in the commit
message and contracts in the audit, so the ledger only holds what those can't:

- entries not yet compared, and why;
- each uncovered outcome, with its next check or its unreachability proof;
- external dependencies, and the revision the evidence applies to.

Its job is to let a later session resume and to keep the proofs. Keep game-derived
listings and captures under `.local/`, and never commit them.
