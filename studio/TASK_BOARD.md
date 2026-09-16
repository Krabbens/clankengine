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

- `M8-WINDOWS-TESTS-001` — type: infrastructure; owner: next sprint; priority: P1; criticality: P1; effort: S; epic: agent observability.
  - Context: five existing CTest scripts assume Linux executable names or direct script execution.
  - Goal: make the full CTest suite runnable on Windows without weakening Linux coverage.
  - Dependencies: none.
  - Blocks: clean Windows acceptance signal for future slices.
  - DoD: full CTest passes on Windows; scripts resolve `.exe` names and invoke claim tools through Python when needed.
  - Next Action: claim m8 and reproduce each failure on a clean branch.

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

### BLOCKED

### DONE

## ARCHIVE
