# GAMEDEV TASK BOARD

## METADATA

- Last updated: 2026-09-17
- Focus (current experiment): deterministic Actor-like runtime foundations
- Active Critical Path: resolve the competing m8 PR queue and merge the verified sample

## KANBAN

### BACKLOG

- `M3-CONTACT-FILTERS-001` — define deterministic contact/query filtering without a physics manager.

### READY

### IN PROGRESS

### REVIEW / TEST

- `M8-HIERARCHY-SAMPLE-001` — type: proof slice; owner: codex; priority: P0; criticality: P1; effort: S; epic: Unreal-like runtime foundation.
  - Context: the m8 guard has one older competing open PR (#107).
  - Goal: merge the already verified hierarchy sample and Windows portability proof after the queue is resolved.
  - DoD: PR #109 guard, build, lint, and LOC all green; local Windows CTest is 18/18.
  - Next Action: resolve #107 ownership/queue; do not bypass the one-PR-per-module guard.

### DONE

- `M3-QUERY-AABB-001` — deterministic Box2D AABB body query; merged PR #114; m3 selftest and CI green.
- `M4-SET-VELOCITY-001` — null/static-safe Box3D velocity control with deterministic scripted selftest; merged PR #115.
- `M1-LIVE-INPUT-001` — headless-safe current keyboard snapshot with windowed raylib polling; merged PR #116.
- `M2-ASSETS-001` — checked file-backed model/texture errors with lazy headless-safe loading; merged PR #117.
- `M5-ACTOR-LIFECYCLE-001` — validated spawn and leaf destroy with component cleanup; merged PR #118.
- `M5-COMPONENTS-001` — owner-validated ComponentStore and deterministic lookup; merged PRs #110 and #113.
- `M6-ACTION-MAP-001` — named action bindings over `.clk` replay; merged PR #111.
- `M7-WINDOWS-SHOT-001` — stdlib-only portable shot runner and tests; merged PR #112.
- `M5-HIERARCHY-001` — stable parent IDs and affine world transforms; merged PR #108.
- `M8-WINDOWS-TESTS-001` — portable executable/script/temp-path tests; verified in PR #109.
- `M8-HIERARCHY-SAMPLE-001` — headless hierarchy dump/see/drive proof; verified in PR #109.

### BLOCKED

- `M8-HIERARCHY-SAMPLE-001` — merge queue blocked by older open PR #107; local implementation and tests are complete.

## ARCHIVE
