---
name: cm0102-recompile-module
description: Reconstruct a whole CM 01/02 module (X.CPP) as handwritten C++23 in cm0102mac, including closed inventory, production routing, differential checks and delivery. Use for a named module, unfinished verification or an authorized next/random module, including Portuguese requests such as "recompila", "reconstrói" and "próximo módulo". Exclude isolated bug fixes, gameplay features and performance tuning.
---

# CM 01/02 complete module reconstruction

Deliver ordinary C++23 that preserves everything the game can observe. Locate
the active checkout, usually `/Users/victordmor/Projects/cm0102mac`. Read its
current `AGENTS.md` and `CLAUDE.md`: they own routing, verification and delivery
policy; current user instructions take precedence. Read `docs/cpp-rewrites.md`
through "Checking one" and the relevant subsystem sections. Older reports are
background, not acceptance rules. Communicate in Victor's language; code,
comments, technical docs and commits stay in English.

## Target, ownership and completion

Use the named module. When Victor authorizes choosing the next or a random one,
read the existing progress sheet linked in `CLAUDE.md` through connected
Drive/Sheets tools and the relevant skill. For a random choice, filter out
`Concluído` rows and modules with ongoing work, then draw using `random.choice`;
do not rank by size or profile candidates first. Check native, tools, shared
integration diffs and `TASKS.md` before taking ownership. If no target or choice
authorization can be inferred, ask once and continue useful read-only discovery.
Keep the current module sequential unless Victor redirects it.

State the module and finish line in one sentence. Record `git status` before
editing; preserve unrelated work even inside shared files. Keep the active work
list in `TASKS.md`, with detailed evidence private under `.local/rewrites/<module>/`.

Report four dimensions separately, with exact denominators only after inventory
closure; an estimate or address range is not a denominator:

- **Inventory:** justified, closed set of original entries.
- **Implementation:** complete handwritten C++23 for every entry, including
  error, legacy, callback and resume paths.
- **Production:** normal-app routing for every entry and owned continuation,
  without generated-body or instruction-builder fallback.
- **Verification:** valid comparisons for each entry and reachable behavior,
  combined-module and necessary integration checks, with no unresolved divergence.

Say **100% reconstructed** only when all four close. State build, installation,
tracker and push status separately; pending delivery is not completed delivery.
Honor user restrictions and report the actual scope without lowering the checks
or implying a proof over all possible game executions.

Victor owns career, visual/gameplay QA, manual save/reload and new gameplay
captures. Do not launch or drive these without his explicit request on this
task. Existing captures and controlled fixtures supply agent evidence. His QA
is outside module completion gates; mention `not retested` once where relevant,
without presenting it as unfinished agent work.

## 1. Close the inventory

Reconcile source/string-pool evidence, TOC, neighbors, calls, transition vectors,
callbacks, tail calls and reachable interior entries. Separate functions from
jump-table labels and shared code. A dispatch census or the sheet's heuristic
range alone proves no boundary or unreachability.

Create or extend `tools/<module>_audit.py` and its focused tests: pin entries,
ends, uniqueness, reference digests and production ownership; resolve actual
gaps and overlaps. A private-export skip is not a passing audit. Reuse small
versioned metadata, keeping original listings and captures private.

For integration, inspect `native/siprint*`, `tools/siprint_audit.py`,
`tools/test_siprint_audit.py`, `tools/make_siprint_reference.py`, its CMake block
and generator hook. For checker strength, inspect `native/media_checks.cpp`
and `tools/call_arguments.py`; adapt their contracts, not module addresses or
unchecked assumptions. Do not copy an older builder module's architecture.

## 2. Pilot the checker, then implement functional blocks

Read [architecture-and-fidelity.md](references/architecture-and-fidelity.md)
before the first block and
[verification-and-delivery.md](references/verification-and-delivery.md) before
preparing checks. Characterize inputs, results, records, ABI, calls and unusual
paths from the reference. Pilot one or two representative routines and verify
the checker and original bypass before scaling the implementation.

Use named record views, typed functions, ordinary control flow, direct calls
between rewritten routines and a narrow ABI adapter. Preserve memory/stack,
call arguments and order, RNG, floating point, errors and serialized bytes.
Keep an explicit list of external dependencies. Owned generated calls, builder
transcriptions and algorithms living in a register array remain incomplete.

Syntax-check owned translation units before the serialized build. During a
routine's implementation, test that routine and affected callers/dependencies.
Batch combined-module checks at coherent block closure and final module closure,
not after every routine, helper or mechanical edit. Before an expensive rerun,
identify the unresolved risk and which change invalidated existing evidence;
reuse still-valid results. Avoid repeating prefix suites once complete returned
comparisons cover their contracts, unless they retain a distinct regression.
Compare each block from identical initial states. Required evidence includes comparisons of
every entry, reproducible input mutations, call traces with arguments, deliberate
wrong-result/trace controls, and one run with all entries active together.
Coverage locates missing behaviors; close them with valid cases or specific
reference-based reachability proofs. Record any genuinely inapplicable check
and its reason; zero calls or faulty fixtures cannot make it inapplicable.

On a difference, find the earliest observable mismatch and inspect the reference,
rewrite and fixture preconditions before editing. Never weaken the oracle,
discard valid cases or refresh an expected snapshot from the rewrite. If the
generated reference differs from PowerPC hardware, establish the discrepancy
and fix generator and rewrite together under current project policy. Obtain the
required authorization before changing simulation results or RNG order.

## 3. Escalate only the bounded difficult part

Follow `AGENTS.md`: GPT-6.1 Sol coordinates and implements at the session's
reasoning effort. Use one `recomp_astra` specialist at a time when targeted
original inspection leaves competing correctness interpretations, or when the
same observable divergence survives two distinct evidence-based fixes. Size,
compiler errors and missing dependencies alone are not escalation triggers.

Explain the uncertainty to Victor. Prefer the custom agent; otherwise select
`gpt-6-astra`, `high`, fresh context. Pass the current instructions from
`.codex/agents/recomp_astra.toml`, objective, entries/files, expected/observed
behavior, private evidence, attempted fixes, focused command, exclusive edit
scope and completion criteria. Check returned evidence before integration.
Do not recursively escalate from Astra, manufacture failed attempts, import
Claude model names or change global model settings. Report unavailable routing
and continue only evidence-supported work.

## 4. Integrate and verify the final configuration

Hook every entry in the generator, regenerate and inspect the changed-page list.
Never hand-edit generated pages. Remove superseded owned builder production
paths while retaining a true original route for checks.

Prove default production and checker routing separately, including direct and
same-page calls, callbacks, interior resumes and observer-enabled configurations.
A checker-selected route or `--only` run can conceal a production gap.

Run the final combined comparisons, mutations, argument traces and coverage on
the instrumented checker, then the relevant existing replay pairs and selected
tests described in the reference. Batch expensive runs on stable code; rerun
the evidence invalidated by subsequent edits. If no usable capture exists, use
controlled original/C++ paired replay fixtures with matching final observable
state and record the replay limit. Unproven integration remains open; never
start a new gameplay campaign to chase it.

## 5. Deliver with focused checks

Restore coverage and diagnostic defaults OFF, build the normal app and rerun
selected checks. Follow the reference for review, guarded installation, tracker
readback, owned commits/push and evidence cleanup. Review before committing;
report an unavailable review skill and perform an explicit scoped diff review.
Do not claim installation, a pushed revision or a speed gain without evidence.

Keep completed verified blocks deliverable even if another gate is pending;
continue authorized independent work. Use the commit message as the record and
update subsystem docs only for a changed contract or a useful quirk. Do not
create separate delivery reports or repeat the same status across docs.

End with the project's **Blocked on me**, **Changed**, **Found** headings. Give
implementation/production counts, checks actually run, limits, the four closure
states and delivery status. Do not report historical logs as current passes.
