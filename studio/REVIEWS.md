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
