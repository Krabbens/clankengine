#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>

#include "sample_loop.hpp"

namespace hierarchy {
struct Game {
  int seed = 42;
  float root_x = 0;
  float root_angle = 0;
  float arm_angle = 0.65f;
  mutable std::vector<clank::m5::WorldTransform> world;

  explicit Game(int seed_value) : seed(seed_value) {}

  clank::m5::Scene Scene(int seed) const {
    clank::m5::Scene scene{seed, {}};
    scene.entities.push_back({10, "root", root_x, 0, root_angle, 1, 1});
    scene.entities.push_back({11, "arm", 1.8f, 0, arm_angle, 1.8f, 0.45f, 0, 10});
    scene.entities.push_back({12, "tip", 1.4f, 0, 0.25f, 0.7f, 0.7f, 0, 11});
    return scene;
  }

  void Update(const sample_loop::Context& ctx, int frame, double dt) {
    const bool left = ctx.isDown(frame, clank::m6::Key::Left, KEY_LEFT) ||
                      ctx.isDown(frame, clank::m6::Key::Left, KEY_A);
    const bool right = ctx.isDown(frame, clank::m6::Key::Right, KEY_RIGHT) ||
                       ctx.isDown(frame, clank::m6::Key::Right, KEY_D);
    root_x += static_cast<float>(right - left) * static_cast<float>(dt) * 2.0f;
    root_angle = 0.18f * std::sin(static_cast<float>(frame) * 0.045f);
    arm_angle = 0.65f + 0.35f * std::sin(static_cast<float>(frame) * 0.06f);
  }

  const std::vector<clank::m5::WorldTransform>& Resolve() const {
    auto resolved = clank::m5::ResolveWorldTransforms(Scene(seed));
    if (!resolved) {
      std::fprintf(stderr, "hierarchy: %s\n", resolved.error().c_str());
      std::exit(1);
    }
    world = std::move(*resolved);
    return world;
  }

  static float ScreenX(float x) { return 640.0f + x * 95.0f; }
  static float ScreenY(float y) { return 390.0f - y * 95.0f; }

  static void Link(clank::m2::Renderer& renderer, const clank::m5::WorldTransform& a,
                   const clank::m5::WorldTransform& b, clank::m2::Color color) {
    const float ax = ScreenX(a.x);
    const float ay = ScreenY(a.y);
    const float bx = ScreenX(b.x);
    const float by = ScreenY(b.y);
    const float dx = bx - ax;
    const float dy = by - ay;
    const float length = std::sqrt(dx * dx + dy * dy);
    const int steps = std::max(1, static_cast<int>(length / 8.0f));
    for (int i = 1; i < steps; ++i) {
      const float t = static_cast<float>(i) / static_cast<float>(steps);
      clank::m2::DrawCircle(renderer, ax + dx * t, ay + dy * t, 3, color);
    }
  }

  static void Arrow(clank::m2::Renderer& renderer, const clank::m5::WorldTransform& t,
                    clank::m2::Color color) {
    const float length = std::sqrt(t.m00 * t.m00 + t.m10 * t.m10);
    const float dx = length > 0 ? t.m00 / length : 1;
    const float dy = length > 0 ? t.m10 / length : 0;
    const float px = -dy;
    const float py = dx;
    const float x = ScreenX(t.x);
    const float y = ScreenY(t.y);
    clank::m2::DrawTriangle(renderer, x + dx * 28, y - dy * 28, x - dx * 12 + px * 9,
                            y + dy * 12 + py * 9, x - dx * 12 - px * 9, y + dy * 12 - py * 9,
                            color);
  }

  void Draw(clank::m2::Renderer& renderer) const {
    const auto& t = Resolve();
    Link(renderer, t[0], t[1], {70, 218, 195, 255});
    Link(renderer, t[1], t[2], {255, 186, 99, 255});
    clank::m2::DrawCircle(renderer, ScreenX(t[0].x), ScreenY(t[0].y), 16, {70, 218, 195, 255});
    clank::m2::DrawCircle(renderer, ScreenX(t[1].x), ScreenY(t[1].y), 13, {255, 186, 99, 255});
    clank::m2::DrawCircle(renderer, ScreenX(t[2].x), ScreenY(t[2].y), 10, {139, 151, 255, 255});
    Arrow(renderer, t[1], {255, 240, 180, 255});
    clank::m2::DrawText(renderer, "ROOT 10", ScreenX(t[0].x) - 30, ScreenY(t[0].y) + 22, 16,
                        {226, 235, 244, 255});
    clank::m2::DrawText(renderer, "ARM 11 -> 10", ScreenX(t[1].x) - 38, ScreenY(t[1].y) + 20, 16,
                        {226, 235, 244, 255});
    clank::m2::DrawText(renderer, "TIP 12 -> 11", ScreenX(t[2].x) - 35, ScreenY(t[2].y) + 18, 16,
                        {226, 235, 244, 255});
  }
};
}  // namespace hierarchy

int main(int argc, char** argv) {
  return sample_loop::Run<hierarchy::Game>(
      argc, argv,
      {"hierarchy",
       "04 / SCENE GRAPH",
       "Parent-linked transforms",
       "A deterministic Actor-like hierarchy: root -> arm -> tip",
       "Arrows/A,D move root | P pause | N step | Esc exit\n",
       "ARROWS/A,D move root  P pause  N step",
       {70, 218, 195, 255},
       {226, 235, 244, 255},
       {135, 158, 181, 255}},
      [](int seed) { return std::make_unique<hierarchy::Game>(seed); },
      [](hierarchy::Game& game, const sample_loop::Context& ctx, int frame, double dt) {
        game.Update(ctx, frame, dt);
      },
      {}, [](const hierarchy::Game& game, clank::m2::Renderer& renderer) { game.Draw(renderer); },
      [](const hierarchy::Game& game, int seed) { return game.Scene(seed); });
}
