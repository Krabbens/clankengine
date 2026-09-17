#pragma once
// WHY: single contract for dump/see/drive; shape must match spec/scene.schema.json.
#include <expected>
#include <span>
#include <string>
#include <vector>
namespace clank::m5 {
struct Entity {
  int id = 0;
  std::string name;
  // WHY z last: positional initializers in examples keep meaning, z defaults 0.
  float x = 0, y = 0, angle = 0, sx = 1, sy = 1, z = 0;
  // WHY id, not an index: scene order may change without changing hierarchy links.
  int parent = -1;
};
struct Component {
  int id = 0;
  // WHY an entity ID: component ownership must survive scene-vector reordering.
  int owner = 0;
  std::string type;
};
struct ComponentStore {
  // WHY own components here: IDs must not depend on a caller's temporary span or vector index.
  std::vector<Component> components;
};
struct Scene {
  int seed = 42;
  std::vector<Entity> entities;
};
struct WorldTransform {
  float x = 0, y = 0, z = 0;
  // 2x2 basis stores exact rotation/scale composition, including shear.
  float m00 = 1, m01 = 0, m10 = 0, m11 = 1;
};
// WHY append-only: callers can retain order while IDs remain stable across removal.
std::expected<int, std::string> SpawnEntity(Scene& scene, Entity entity);
// WHY require the store: removing owned components prevents dangling ownership after destroy.
std::expected<Entity, std::string> DestroyEntity(Scene& scene, ComponentStore& store, int id);
std::string DumpJson(const Scene& scene);
std::expected<Scene, std::string> LoadJson(const std::string& text);
// WHY return the same order: callers can join resolved transforms to their input entities without
// a second id map. The basis is a 2D affine transform; z is an additive height channel.
std::expected<std::vector<WorldTransform>, std::string> ResolveWorldTransforms(const Scene& scene);
// WHY return IDs, not references: callers can keep components in their own storage and reorder it.
// The result preserves the input span order. The span is the complete component registry: every
// entry must have a valid owner, non-empty type, and globally unique ID, even when not selected.
// Components are runtime data in this slice; scene JSON remains backward-compatible.
std::expected<std::vector<int>, std::string> ResolveComponents(
    const Scene& scene, std::span<const Component> components, int owner);
// WHY return the explicit ID: callers can retain it while the store grows or is reordered.
std::expected<int, std::string> AttachComponent(ComponentStore& store, const Scene& scene,
                                                Component component);
std::expected<Component, std::string> LookupComponent(const ComponentStore& store, int id);
// WHY sort IDs: lookup order is stable even if the owning storage is reordered.
std::expected<std::vector<int>, std::string> LookupComponents(const ComponentStore& store,
                                                              int owner);
}  // namespace clank::m5
