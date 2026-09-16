#pragma once
// WHY: single contract for dump/see/drive; shape must match spec/scene.schema.json.
#include <expected>
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
struct Scene {
  int seed = 42;
  std::vector<Entity> entities;
};
struct WorldTransform {
  float x = 0, y = 0, angle = 0, sx = 1, sy = 1, z = 0;
};
std::string DumpJson(const Scene& scene);
std::expected<Scene, std::string> LoadJson(const std::string& text);
// WHY return the same order: callers can join resolved transforms to their input entities without
// a second id map. x/y/angle/scale compose in 2D; z is an additive height channel.
std::expected<std::vector<WorldTransform>, std::string> ResolveWorldTransforms(const Scene& scene);
}  // namespace clank::m5
