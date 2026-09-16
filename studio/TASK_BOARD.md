# GAMEDEV TASK BOARD

## METADATA

- Last updated: 2026-09-16
- Focus (current experiment): deterministic Actor-like scene hierarchy
- Active Critical Path: prove hierarchy at the sample boundary

## KANBAN

### BACKLOG

- `M5-COMPONENTS-001` — design explicit component ownership after hierarchy review.
- `M2-ASSETS-001` — define a minimal deterministic asset handle contract.

### READY

### IN PROGRESS

### REVIEW / TEST

- `M5-HIERARCHY-001` — type: capability slice; owner: codex; priority: P0; criticality: P1; effort: S; epic: Unreal-like runtime foundation.
  - Context: `m5::Scene` is a flat entity list.
  - Goal: add stable parent IDs, JSON round-trip support, and world transform resolution.
  - Dependencies: none.
  - Blocks: component ownership and prefab composition.
  - DoD: self-test covers round-trip, rotation/scale composition, missing parent, duplicate ID, and cycle errors; focused Windows suite passes.
  - Next Action: add a headless sample assertion that consumes resolved hierarchy data.
  - Implementation notes: keep root output backward compatible; `z` remains additive height; resolved 2D basis is affine to preserve shear.
  - Test/acceptance notes: affine regression and invalid-parent JSON tests pass; focused suite 4/4 passes; full suite has five unrelated Windows harness failures.
- `M8-HIERARCHY-SAMPLE-001` — type: proof slice; owner: codex; priority: P0; criticality: P1; effort: S; epic: Unreal-like runtime foundation.
  - Context: m5 hierarchy was only proven by a module self-test.
  - Goal: demonstrate parent-linked transforms through a real headless sample using dump, see, and drive.
  - Dependencies: merged m5 hierarchy contract.
  - Blocks: component/prefab work until the runtime contract is visible at sample level.
  - DoD: `hierarchy` builds; dump contains `10 -> 11 -> 12`; repeated replay produces byte-identical JSON/PNG; undriven and driven outputs differ.
  - Next Action: open the m8 sample PR and pass CI.
  - Implementation notes: no new engine API; `ResolveWorldTransforms` is consumed directly by the sample.
  - Test/acceptance notes: Windows focused sample test 1/1 passes; full Windows CTest is now 18/18.
- `M8-WINDOWS-TESTS-001` — type: infrastructure; owner: codex; priority: P1; criticality: P1; effort: S; epic: agent observability.
  - Context: five existing CTest scripts assumed Linux executable names or direct script execution.
  - Goal: make the full CTest suite runnable on Windows without weakening Linux coverage.
  - Dependencies: none.
  - Blocks: clean Windows acceptance signal for future slices.
  - DoD: full CTest passes on Windows; scripts resolve `.exe` names and invoke claim tools through Python when needed.
  - Next Action: merge after the existing m8 PR queue is resolved.
  - Implementation notes: preserve intentional Unix-only Bash exclusions; use the platform temp directory for core screenshots.
  - Test/acceptance notes: full Windows CTest is 18/18; Linux behavior is unchanged by the portable path/name fixes.

### BLOCKED

### DONE

- `M5-HIERARCHY-001` — merged as PR #108; affine transform regression and JSON validation are covered.

## ARCHIVE
