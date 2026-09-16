# Clankengine Studio

## Current direction

Build a tiny, agent-first game engine that approaches Unreal-like capability
through explicit, deterministic slices rather than a large editor framework.

## Current experiment

Build a thin Unreal-like runtime foundation from deterministic slices: m3 AABB
queries, m5 scene/components, m6 named actions, m7 Windows observability, and
the m8 hierarchy sample.

## Last decision

The runtime stays explicit and manager-free: stable entity/component IDs,
deterministic input actions, physics queries, and headless dump/see/drive are
the useful foundation before any editor or large object framework.

## Open

The next decision is how to resolve the two open m8 PRs while keeping one
claim per module. Component ownership and action mapping are now implemented;
asset handles and richer Actor lifecycle remain future slices.

## Evidence so far

The integrated Windows build passes full CTest `18/18`. m3 QueryAabb,
m5 ComponentStore, m6 named actions, m7 `shot.py`, and the hierarchy sample
all pass focused tests; the m7 smoke collects deterministic JSON and PNG
artifacts on Windows.

## Next action

Keep PR #109 ready and resolve the older open m8 PR #107 before merging the
sample. No code or test failure remains in the current local integration.
