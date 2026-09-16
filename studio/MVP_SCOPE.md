# MVP Scope

## MUST HAVE

- Fixed-step deterministic runtime.
- `dump / see / drive` observability.
- Thin rendering and physics facades.
- Scene entities with stable IDs and serializable transforms.
- Parent-child transform resolution with cycle and missing-parent errors.

## SHOULD HAVE

- Actor-like components with explicit ownership.
- Asset handles and deterministic load failures.
- World queries, event routing, and prefab instantiation.
- Headless sample coverage for every new runtime capability.

## CUTTABLE

- Editor UI, visual scripting, post-processing, and broad platform abstraction.

## OUT OF SCOPE (NOW)

- Full Unreal parity, networking, large-world streaming, shader graphs, and a general-purpose scripting runtime.
