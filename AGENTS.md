# CM 01/02 Codex instructions

Read `CLAUDE.md` for the project's reconstruction, guest-state fidelity,
verification and delivery conventions. Preserve existing unrelated changes.

## Model routing for C++ reconstruction

Victor authorized this workflow on 2026-10-26. For reconstruction work, this
policy specializes the delegation guidance in `CLAUDE.md`.

- Use GPT-6.1 Sol (`gpt-6.1-sol`) as the default coordinator and implementer.
  Keep the session's selected reasoning effort. The coordinator owns scope,
  integration, focused verification and delivery.
- Do not delegate routine module conversions or require an Astra review of
  every module. Routine compiler errors, missing dependencies and large
  function counts alone are not reasons to escalate.
- Automatically delegate the smallest difficult part to GPT-6 Astra
  (`gpt-6-astra`, reasoning effort `high`) when either:
  1. Targeted inspection of the original code leaves competing interpretations
     of behavior that matter to correctness, including stack/ABI, memory,
     floating point, RNG ordering or save-format behavior; or
  2. The same observable original-versus-rewrite divergence remains after two
     distinct, evidence-based correction attempts.
- Escalation does not need another permission request. Briefly tell Victor
  what remains uncertain and why Astra is being used. Do not make speculative
  edits merely to reach the two-attempt threshold.
- Use the project custom agent `recomp_astra` when the client exposes custom
  agent selection. With a spawn tool that exposes model overrides instead,
  explicitly select `model="gpt-6-astra"`, `reasoning_effort="high"` and a
  fresh context (`fork_turns="none"` when supported). Pass the instructions
  from `.codex/agents/recomp_astra.toml` in that agent's task message too.
- The handoff must include the objective, affected original entries and source
  files, expected versus observed behavior, relevant logs, attempted fixes,
  a focused verification command, permitted edit scope, and completion
  criteria. Keep game-derived listings and captures private under `.local/`.
- Run at most one Astra escalation at a time. Give it exclusive ownership of
  any files it may edit; the coordinator must not edit those files concurrently.
  Keep unrelated work and broad builds out of the escalation.
- The specialist returns its diagnosis, evidence, changes, checks and remaining
  uncertainty. Sol checks that evidence, integrates the result and resumes
  ordinary implementation and delivery. A specialist's confidence alone is
  not proof of equivalence.
- Do not recursively spawn specialists. If the current coordinator already
  uses Astra, handle the difficult reasoning there rather than spawning a
  duplicate. This policy does not itself switch the active chat's model.
- If the requested model or delegation capability is unavailable, report that
  limitation and continue only work supported by the available evidence. Do
  not silently claim that an Astra review occurred or install a workaround.

These instructions authorize task-scoped delegation, not background schedules,
new user-owned chats, a broader game-testing campaign, or changes to global
Codex settings. Explicit instructions from Victor about a task take precedence.
