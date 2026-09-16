// WHY: smallest dump/load round trip, now on the m1 fixed-step loop instead of inline code.
// m0/m6 have no role here (no flags, no input, no RNG); the seed stays a plain constant.
#include <cstdio>

#include "m1/loop.hpp"
#include "m5/scene.hpp"
int main() {
  clank::m5::Scene scene;
  scene.seed = 42;
  scene.entities.push_back({1, "player one", 0.0f, 0.0f, 0.0f, 1.0f, 1.0f});
  constexpr int kFrames = 120;
  constexpr float kDt = 1.0f / 60.0f;
  auto update = [&](clank::m1::Frame, double) { scene.entities[0].x += kDt; };
  clank::m1::Run({1.0 / 60.0, kFrames}, update);
  std::string json = clank::m5::DumpJson(scene);
  std::printf("%s\n", json.c_str());
  auto back = clank::m5::LoadJson(json);
  if (!back) {
    std::fprintf(stderr, "reload failed: %s\n", back.error().c_str());
    return 1;
  }
  std::fprintf(stderr, "minimal: frames=%d entities=%zu x=%f\n", kFrames, back->entities.size(),
               static_cast<double>(back->entities[0].x));
  return 0;
}
