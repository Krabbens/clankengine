# Reviews

## Review: m5 scene hierarchy sprint

- Date: 2026-09-16
- Claim: stable parent-linked scene entities can be resolved deterministically and survive JSON dump/load.
- Audience: engine contributors and agents building samples without an editor.
- Evidence: Windows CMake/Ninja build; `m5-selftest`; focused CTest 4/4; schema accepts optional `parent`; `git diff --check` clean.
- Gate: focused suite passes, no P0, and the API stays within the m5 boundary.
- Verdict: IMPROVE
- Weighted score: 3.7/5
- P0 veto: None

### Panel

| Reviewer | Weight | Score 1-5 | Evidence | Main objection | Next action |
|---|---:|---:|---|---|---|
| Gameplay Programmer | 1 | 4 | Small public API, stable IDs, deterministic resolver, no dependency | No runtime sample consumes the resolver yet | Wire one sample to resolved transforms |
| Technical Designer | 1 | 3 | Parent, rotation, scale, and additive height compose in a three-level test | `z` is intentionally not a full 3D scale channel | Keep scope explicit; revisit only with a 3D scene contract |
| QA / Slice Guardian | 1 | 4 | Round-trip, missing-parent, duplicate-ID, and cycle cases are covered | Full Windows CTest remains red for pre-existing harness assumptions | Create a separate m8 Windows harness sprint |

### Clanker pass

This was an internal adversarial pass, not an independent reviewer.

- Strongest claim: the new behavior is bounded, deterministic, and backward-compatible for root dumps.
- Strongest objection: the engine API can pass its self-test while no real sample uses it.
- Missing evidence: a sample-level dump or screenshot driven by resolved hierarchy data.
- Adversarial test: reorder entities, rotate/scale a three-level tree, then feed missing, duplicate, and cyclic parent IDs.
- Score: 3/5
- P0: None

### Synthesis

- What convinced the panel: the resolver is a thin value-level addition and all focused Windows checks pass.
- What did not convince the panel: the feature is not yet visible at the sample/agent workflow boundary.
- Decision: keep the direction and improve the proof before adding components.
- Next sprint: use the hierarchy in one headless sample; repair the unrelated Windows CTest harness on m8.

## Follow-up: clanker correction

- Date: 2026-09-16
- Trigger: the adversarial subagent found that TRS-only world composition lost shear for non-uniform parent scale plus rotated child.
- Correction: `WorldTransform` now stores an exact 2D affine basis; JSON load and resolve reject invalid negative parents and invalid scene IDs/links.
- Evidence: Windows rebuild, `m5-selftest`, focused CTest 4/4, and diff check pass.
- Result: P1 transform bug closed; sample-level proof and full Windows CTest portability remain open.

## Review: hierarchy sample proof

- Date: 2026-09-16
- Claim: an agent can observe a real parent-linked scene through dump, pixels, and replay.
- Audience: engine contributors validating runtime capabilities without an editor.
- Evidence: `hierarchy-sample` CTest 1/1; 1280x720 headless PNG inspected; dump contains `10 -> 11 -> 12`; repeated replay is byte-identical.
- Gate: sample uses only the frozen m5 API, supports all standard flags, and has no P0.
- Verdict: IMPROVE
- Weighted score: 4.3/5
- P0 veto: None

### Panel

| Reviewer | Weight | Score 1-5 | Evidence | Main objection | Next action |
|---|---:|---:|---|---|---|
| Sample/Runtime Reviewer | 1 | 5 | Three-node arm visibly follows resolved parent transforms; dump/PNG/replay checks pass | No component ownership yet | Keep hierarchy stable and move to components only after the Windows gate is green |
| QA / Slice Guardian | 1 | 4 | Same replay is byte-identical; driven and plain runs diverge | Full Windows suite still has five harness failures | Run the Windows portability sprint |
| Technical Designer | 1 | 4 | The sample communicates root -> arm -> tip with minimal code | Labels are absent in headless pixels because text is draw-log-only | Keep node colors/links as the headless proof |

### Clanker pass

This was an internal adversarial pass, not an independent reviewer.

- Strongest claim: the hierarchy is now proven at the same CLI boundary agents use.
- Strongest objection: the sample proves transforms, not Actor lifecycle or components.
- Missing evidence: full Windows CTest and a live-window screenshot with text.
- Adversarial test: run the same replay twice, compare bytes, and compare against an undriven run.
- Score: 4/5
- P0: None

### Synthesis

- Decision: accept the proof direction; improve the platform gate before adding components.
- Next sprint: repair Windows CTest portability in a separate m8 PR.

## Follow-up: Windows portability proof

- Date: 2026-09-16
- Claim: the standard agent-facing CTest suite is runnable on Windows without weakening Unix-only exclusions.
- Evidence: `.exe`-safe test names, Python-launched extensionless claim tool, portable core screenshot path, and full Windows CTest 18/18.
- Verdict: ACCEPT locally; merge remains gated by the pre-existing open m8 PR #107.
- Missing evidence: CI rerun after the duplicate-module queue is resolved.
