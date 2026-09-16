# Decisions

## D-001 — Start with parent-linked scene transforms

- Status: accepted for the current sprint.
- Options: add an editor, add an asset system, add scene hierarchy.
- Choice: scene hierarchy.
- Trade-off: it improves the runtime foundation without proving visual tooling yet.
- Rationale: Actor-like composition is a prerequisite for components, prefabs, and streaming.
- Hypothesis: stable IDs plus deterministic transform resolution will let samples express composition without custom parent math.
- Test: round-trip a child through JSON; resolve a rotated/scaled three-level tree; reject missing parents, duplicate IDs, and cycles.

## D-002 — Keep affine basis in resolved transforms

- Status: accepted after clanker review.
- Problem: adding parent angles and scales loses shear when a non-uniform parent scale combines with a rotated child.
- Choice: return a 2D affine basis plus translation and additive height, while keeping local entity data as compact TRS.
- Trade-off: consumers do not get a derived world angle/scale for free; they get exact composition instead.
- Test: a non-uniformly scaled, rotated three-level tree resolves the grandchild position without TRS approximation; invalid parent IDs fail at resolve and JSON load.
