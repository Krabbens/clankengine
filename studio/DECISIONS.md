# Decisions

## D-001 — Start with parent-linked scene transforms

- Status: accepted for the current sprint.
- Options: add an editor, add an asset system, add scene hierarchy.
- Choice: scene hierarchy.
- Trade-off: it improves the runtime foundation without proving visual tooling yet.
- Rationale: Actor-like composition is a prerequisite for components, prefabs, and streaming.
- Hypothesis: stable IDs plus deterministic transform resolution will let samples express composition without custom parent math.
- Test: round-trip a child through JSON; resolve a rotated/scaled three-level tree; reject missing parents, duplicate IDs, and cycles.
