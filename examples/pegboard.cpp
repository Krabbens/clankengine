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
  void Draw(float emitter) const {
    DrawRectangle(318, 116, 644, 536, {18, 30, 44, 255});
    for (size_t i = 0; i < items.size(); ++i) {
      const auto& [body, d] = items[i];
      const auto p = physics::GetPosition(body);
      const float x = 640 + p.x * 42, y = 638 - p.y * 42;
      if (y < 115) continue;
      Color color =
          d.type == physics::BodyType::Dynamic ? showcase::colors[i % 3] : Color{78, 103, 128, 255};
      if (d.shape == physics::ShapeKind::Circle) {
        DrawCircleV({x, y}, d.circle_r * 42, color);
        if (d.type == physics::BodyType::Dynamic) DrawCircleV({x - 3, y - 3}, 3, showcase::ink);
      } else
        DrawRectangleRec({x - d.box_hx * 42, y - d.box_hy * 42, d.box_hx * 84, d.box_hy * 84},
                         color);
    }
    DrawTriangle({640 + emitter * 42, 152}, {650 + emitter * 42, 132}, {630 + emitter * 42, 132},
                 showcase::colors[0]);
    DrawText("01 / GRAVITY", 44, 180, 20, showcase::ink);
    DrawText("24 seeded balls\nStatic circle pegs\nBox dividers\nContact + friction", 44, 218, 18,
             showcase::muted);
    DrawText("BOX2D 3.1", 1000, 180, 20, showcase::colors[1]);
    DrawText("Same seed.\nSame input.\nSame simulation.", 1000, 218, 18, showcase::muted);
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
