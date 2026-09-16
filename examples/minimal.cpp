// WHY: shows dump/see/drive shape before m0/m1/m6 land.
// TODO(m0/m1/m6): replace inline loop/seed with foundation/loop/input facades.
#include "m5/scene.hpp"
#include <cstdio>
int main() {
  clank::m5::Scene scene;
  scene.seed = 42;
  scene.entities.push_back({1, "player one", 0.0f, 0.0f, 0.0f, 1.0f, 1.0f});
  const int frames = 120;
  const float dt = 1.0f / 60.0f;
  for (int f = 0; f < frames; ++f) scene.entities[0].x += dt;
  std::string json = clank::m5::DumpJson(scene);
  std::printf("%s\n", json.c_str());
  auto back = clank::m5::LoadJson(json);
  if (!back) {
    std::fprintf(stderr, "reload failed: %s\n", back.error().c_str());
    return 1;
  }
  std::fprintf(stderr, "minimal: frames=%d entities=%zu x=%f\n", frames, back->entities.size(),
               static_cast<double>(back->entities[0].x));
  return 0;
}
