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
};
struct Scene {
  int seed = 42;
  std::vector<Entity> entities;
};
std::string DumpJson(const Scene& scene);
std::expected<Scene, std::string> LoadJson(const std::string& text);
}  // namespace clank::m5
