#include "m3/physics2d.hpp"
#include "showcase.hpp"

namespace physics = clank::m3;
namespace render = clank::m2;
struct Breakout {
  physics::World* world = physics::CreateWorld({0, 0});
  struct Item {
    physics::Body* body;
    physics::BodyDef def;
    bool brick = false;
  };
  std::vector<Item> items;
  physics::Body* paddle = nullptr;
  float paddle_x = 0;
  explicit Breakout(int seed) {
    (void)seed;
    auto wall = [&](float x, float y, float hx, float hy) {
      physics::BodyDef d;
      d.position = {x, y};
      d.shape = physics::ShapeKind::Box;
      d.box_hx = hx;
      d.box_hy = hy;
      d.friction = 0;
      d.restitution = 1.0f;
      items.push_back({physics::CreateBody(world, d), d, false});
    };
    wall(-7, 6, .2f, 6);
    wall(7, 6, .2f, 6);
    wall(0, 12, 7, .2f);
    MovePaddle(0);
    for (int row = 0; row < 3; ++row)
      for (int col = 0; col < 8; ++col) {
        physics::BodyDef d;
        d.position = {-5.25f + 1.5f * col, 6.5f + row};
        d.shape = physics::ShapeKind::Box;
        d.box_hx = .65f;
        d.box_hy = .35f;
        d.friction = 0;
        d.restitution = 1.0f;
        items.push_back({physics::CreateBody(world, d), d, true});
      }
  }
  ~Breakout() {
    if (paddle != nullptr) physics::DestroyBody(paddle);
    physics::DestroyWorld(world);
  }
  void MovePaddle(float x) {
    // WHY destroy/create: the facade has no teleport; one static body per step is cheapest.
    if (paddle != nullptr) physics::DestroyBody(paddle);
    physics::BodyDef d;
    d.position = {x, 1.0f};
    d.shape = physics::ShapeKind::Box;
    d.box_hx = 1.2f;
    d.box_hy = .15f;
    d.friction = 0;
    d.restitution = 1.0f;
    paddle = physics::CreateBody(world, d);
  }
  const Item* Find(const physics::Body* body) const {
    for (const auto& item : items)
      if (item.body == body) return &item;
    return nullptr;
  }
  static bool IsBall(const Item& item) { return item.def.type == physics::BodyType::Dynamic; }
  void Drop(float x, float y = 2.2f) {
    if (items.size() >= 128) return;
    paddle_x = x;
    physics::BodyDef d;
    d.position = {x, y};
    d.type = physics::BodyType::Dynamic;
    d.circle_r = .28f;
    d.friction = 0;
    d.restitution = 1.0f;
    d.linear_vel = {1.0f, 6.0f};
    items.push_back({physics::CreateBody(world, d), d, false});
  }
  void Step(float dt) {
    MovePaddle(paddle_x);
    physics::Step(world, dt);
    for (const auto& t : physics::GetTouches(world)) {
      if (!t.began) continue;
      const Item* at = Find(t.a);
      const Item* bt = Find(t.b);
      const physics::Body* hit = nullptr;
      if (at != nullptr && bt != nullptr) {
        if (at->brick && IsBall(*bt)) hit = at->body;
        if (bt->brick && IsBall(*at)) hit = bt->body;
      }
      if (hit == nullptr) continue;
      for (auto it = items.begin(); it != items.end(); ++it)
        if (it->body == hit) {
          physics::DestroyBody(it->body);
          items.erase(it);
          break;
        }
    }
    for (size_t i = 0; i < items.size();) {
      if (IsBall(items[i]) && physics::GetPosition(items[i].body).y < -2) {
        physics::DestroyBody(items[i].body);
        items.erase(items.begin() + static_cast<ptrdiff_t>(i));
      } else {
        ++i;
      }
    }
  }
  void Draw(float emitter, render::Renderer& renderer) const {
    const render::Color ink{226, 235, 244, 255};
    const render::Color muted{135, 158, 181, 255};
    const render::Color pal[3] = {{70, 218, 195, 255}, {255, 186, 99, 255}, {139, 151, 255, 255}};
    render::DrawRect(renderer, 318, 116, 644, 536, {18, 30, 44, 255});
    auto dot = [&](float x, float y, float r, render::Color c) {
      render::DrawCircle(renderer, 640 + x * 42, 638 - y * 42, r * 42, c);
    };
    auto bar = [&](float x, float y, float hx, float hy, render::Color c) {
      render::DrawRect(renderer, 640 + (x - hx) * 42, 638 - (y + hy) * 42, hx * 84, hy * 84, c);
    };
    for (size_t i = 0; i < items.size(); ++i) {
      const auto& [body, d, brick] = items[i];
      const auto p = physics::GetPosition(body);
      const float y = 638 - p.y * 42;
      if (y < 115) continue;
      if (d.shape == physics::ShapeKind::Circle)
        dot(p.x, p.y, d.circle_r, pal[i % 3]);
      else if (brick)
        bar(p.x, p.y, d.box_hx, d.box_hy, pal[(i / 8) % 3]);
      else
        bar(p.x, p.y, d.box_hx, d.box_hy, render::Color{78, 103, 128, 255});
    }
    const auto pp = physics::GetPosition(paddle);
    bar(pp.x, pp.y, 1.2f, .15f, pal[0]);
    render::DrawTriangle(renderer, 640 + emitter * 42, 152, 650 + emitter * 42, 132,
                         630 + emitter * 42, 132, pal[0]);
    render::DrawText(renderer, "04 / BREAKOUT", 44, 180, 20, ink);
    render::DrawText(renderer, "Restitution rallies\nContacts break bricks", 44, 218, 18, muted);
    render::DrawText(renderer, "BOX2D 3.1", 1000, 180, 20, pal[1]);
    render::DrawText(renderer, "Same seed.\nSame input.\nSame simulation.", 1000, 218, 18, muted);
  }
  clank::m5::Scene Scene(int seed, float emitter) const {
    clank::m5::Scene scene{seed, {}};
    for (const auto& [body, d, brick] : items) {
      auto p = physics::GetPosition(body);
      scene.entities.push_back(
          {static_cast<int>(scene.entities.size()),
           d.type == physics::BodyType::Dynamic ? "ball"
           : brick                              ? "brick"
                                                : "obstacle",
           p.x, p.y, physics::GetAngle(body),
           d.shape == physics::ShapeKind::Circle ? d.circle_r * 2 : d.box_hx * 2,
           d.shape == physics::ShapeKind::Circle ? d.circle_r * 2 : d.box_hy * 2});
    }
    const auto pp = physics::GetPosition(paddle);
    scene.entities.push_back(
        {static_cast<int>(scene.entities.size()), "obstacle", pp.x, pp.y, 0, 2.4f, .3f});
    scene.entities.push_back({static_cast<int>(scene.entities.size()), "emitter", emitter, 11});
    return scene;
  }
};
int main(int argc, char** argv) {
  return showcase::Run<Breakout>(argc, argv, "breakout", "Breakout / 2D",
                                 "Restitution rallies, contacts break bricks");
}
