# Mission (standing, decided 2026-09-16)

> Standing agent notes, tracked in repo. Rides along with a module PR, never
> alone: `claim-guard` ignores non-module paths for module mapping and
> `loc-guard` does not count `.agents/`, so bundling is CI-safe.

## 1. Agent-first includes GitHub

"No monitor, no mouse, no editor" (AGENTS.md §1) also means no human clicker.
The agent drives the full loop autonomously via `gh` + `tools/clank-claim`:

- open PRs, read full diffs, post review findings as PR comments,
- poll `gh pr checks` between tasks (never `sleep`-wait on CI),
- merge with `gh pr merge --merge`, delete the branch, `release` the claim,
- heartbeat every ~15min so claims never go stale mid-CI.

A wave that stalls waiting for a human to press merge has failed §1.

## 2. Self-improvement mandate

The standing task is self-improvement of this engine: every slice must leave
the engine more capable of agent-driven development than it found it.
Acceptance criterion for any change, beyond green CI:

- Does a headless agent build → run → see → drive → verify faster or with
  fewer lines than before? (bench delta or it didn't happen.)
- Is the new behavior reachable headless with machine-readable stdout and a
  deterministic seed? Editor-only / human-only = reject.
- Did `core/` total stay flat or shrink? Deleting code counts as a feature.

Wave backlog (in order): observability (m2 compare done PR #13, m5 schema v1,
m7 dump/diff, m8 goldens) → physics events/queries (m3/m4) → live input +
replay wiring (m6) → render unification through m2 DrawLog.

## 3. Golden discipline (I approve, so I hold the line)

Goldens turn into fiction the moment approvals go blind. Binding rules:

- Regen only with a pixel-level note of what changed and why it is correct.
  "Update golden to make CI pass" is never a reason.
- Every image diff is actually looked at before merge. `fraction <= 2%`
  green is not an alibi.
- Goldens come from fixed `--seed` + fixed timestep, headless. Unseeded or
  variable-timestep goldens are rejected (AGENTS.md §10).
- Fewer, smaller goldens beat many big ones. Each file in `test/golden/`
  must be traceable to the exact command that produced it.
