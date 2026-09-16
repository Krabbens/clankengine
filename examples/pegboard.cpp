#include "m3/physics2d.hpp"
#include "showcase.hpp"

namespace physics = clank::m3;
struct Pegboard {
  physics::World* world = physics::CreateWorld({0, -10});
  struct Item {
    physics::Body* body;
    physics::BodyDef def;
  };
  std::vector<Item> items;
  explicit Pegboard(int seed) {
    auto box = [&](float x, float y, float hx, float hy) {
      physics::BodyDef d;
      d.position = {x, y};
      d.shape = physics::ShapeKind::Box;
      d.box_hx = hx;
      d.box_hy = hy;
      items.push_back({physics::CreateBody(world, d), d});
    };
    box(0, 0, 7, .2f);
    box(-7, 6, .2f, 6);
    box(7, 6, .2f, 6);
    for (int x = -4; x <= 4; x += 2) box(static_cast<float>(x), 1, .08f, 1);
    for (int row = 0; row < 4; ++row)
      for (int col = 0; col < 6; ++col) {
        physics::BodyDef d;
        d.position = {-5.5f + 2 * col + (row % 2) * .6f, 4.0f + row * 1.5f};
        d.circle_r = .22f;
        items.push_back({physics::CreateBody(world, d), d});
      }
    clank::m0::Rng rng;
    clank::m0::Seed(rng, seed);
    for (int i = 0; i < 24; ++i)
      Drop(static_cast<float>(clank::m0::NextFloat01(rng) * 10 - 5), 11 + i * .65f);
  }
  ~Pegboard() { physics::DestroyWorld(world); }
  void Drop(float x, float y = 11) {
    if (items.size() >= 128) return;
    physics::BodyDef d;
    d.position = {x, y};
    d.type = physics::BodyType::Dynamic;
    d.circle_r = .28f;
    d.friction = .15f;
    items.push_back({physics::CreateBody(world, d), d});
  }
  void Step(float dt) { physics::Step(world, dt); }
  void Draw(float emitter, clank::m2::Renderer& renderer) const {
    namespace m2 = clank::m2;
    // WHY local palette: m2::Color is stdlib-only, so raylib showcase colors convert at the call.
    const m2::Color pal[3] = {{70, 218, 195, 255}, {255, 186, 99, 255}, {139, 151, 255, 255}};
    const m2::Color ink{226, 235, 244, 255};
    const m2::Color muted{135, 158, 181, 255};
    m2::DrawRect(renderer, 318, 116, 644, 536, {18, 30, 44, 255});
    for (size_t i = 0; i < items.size(); ++i) {
      const auto& [body, d] = items[i];
      const auto p = physics::GetPosition(body);
      const float x = 640 + p.x * 42, y = 638 - p.y * 42;
      if (y < 115) continue;
      const m2::Color color =
          d.type == physics::BodyType::Dynamic ? pal[i % 3] : m2::Color{78, 103, 128, 255};
      if (d.shape == physics::ShapeKind::Circle) {
        m2::DrawCircle(renderer, x, y, d.circle_r * 42, color);
        if (d.type == physics::BodyType::Dynamic) m2::DrawCircle(renderer, x - 3, y - 3, 3, ink);
      } else
        m2::DrawRect(renderer, x - d.box_hx * 42, y - d.box_hy * 42, d.box_hx * 84, d.box_hy * 84,
                     color);
    }
    m2::DrawTriangle(renderer, 640 + emitter * 42, 152, 650 + emitter * 42, 132, 630 + emitter * 42,
                     132, pal[0]);
    m2::DrawText(renderer, "01 / GRAVITY", 44, 180, 20, ink);
    m2::DrawText(renderer, "24 seeded balls\nStatic circle pegs\nBox dividers\nContact + friction",
                 44, 218, 18, muted);
    m2::DrawText(renderer, "BOX2D 3.1", 1000, 180, 20, pal[1]);
    m2::DrawText(renderer, "Same seed.\nSame input.\nSame simulation.", 1000, 218, 18, muted);
  }
  clank::m5::Scene Scene(int seed, float emitter) const {
    clank::m5::Scene scene{seed, {}};
    for (const auto& [body, d] : items) {
      auto p = physics::GetPosition(body);
      scene.entities.push_back(
          {static_cast<int>(scene.entities.size()),
           d.type == physics::BodyType::Dynamic ? "ball" : "obstacle", p.x, p.y,
           physics::GetAngle(body),
           d.shape == physics::ShapeKind::Circle ? d.circle_r * 2 : d.box_hx * 2,
           d.shape == physics::ShapeKind::Circle ? d.circle_r * 2 : d.box_hy * 2});
    }
    scene.entities.push_back({static_cast<int>(scene.entities.size()), "emitter", emitter, 11});
    return scene;
  }
};
int main(int argc, char** argv) {
  return showcase::Run<Pegboard>(argc, argv, "pegboard", "Pegboard / 2D",
                                 "Rigid bodies, seeded rain and replayable input");
}
