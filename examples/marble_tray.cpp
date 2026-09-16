#include "m4/physics3d.hpp"
#include "showcase.hpp"

namespace physics = clank::m4;
struct MarbleTray {
  physics::World* world = physics::CreateWorld({0, -10, 0});
  struct Item {
    physics::Body* body;
    physics::BodyDef def;
  };
  std::vector<Item> items;
  explicit MarbleTray(int seed) {
    auto box = [&](physics::Vec3 p, float hx, float hy, float hz) {
      physics::BodyDef d;
      d.position = p;
      d.shape = physics::ShapeKind::Box;
      d.box_hx = hx;
      d.box_hy = hy;
      d.box_hz = hz;
      items.push_back({physics::CreateBody(world, d), d});
    };
    box({0, -.25f, 0}, 7, .25f, 5);
    box({-7, 1, 0}, .2f, 1, 5);
    box({7, 1, 0}, .2f, 1, 5);
    box({0, 1, -5}, 7, 1, .2f);
    box({0, 1, 5}, 7, 1, .2f);
    for (int x = -3; x <= 3; x += 3) box({static_cast<float>(x), .6f, 0}, .6f, .6f, .6f);
    clank::m0::Rng rng;
    clank::m0::Seed(rng, seed);
    for (int i = 0; i < 30; ++i) {
      const float x = (i % 6 - 2.5f) * 1.5f;
      const float z = (i / 6 - 2.0f) * 1.5f;
      Drop(x, z, 3 + static_cast<float>(clank::m0::NextFloat01(rng) * 4));
    }
  }
  ~MarbleTray() { physics::DestroyWorld(world); }
  void Drop(float x, float z = 0, float y = 7) {
    if (items.size() >= 128) return;
    physics::BodyDef d;
    d.position = {x, y, z};
    d.type = physics::BodyType::Dynamic;
    d.sphere_r = .45f;
    d.linear_vel = {.7f, 0, .4f};
    items.push_back({physics::CreateBody(world, d), d});
  }
  void Step(float dt) { physics::Step(world, dt); }
  void Draw(float emitter) const {
    Camera3D camera{{18, 17, 22}, {0, 1, 0}, {0, 1, 0}, 42, CAMERA_PERSPECTIVE};
    BeginMode3D(camera);
    for (size_t i = 0; i < items.size(); ++i) {
      const auto& [body, d] = items[i];
      const auto p = physics::GetPosition(body);
      Vector3 v{p.x, p.y, p.z};
      if (d.shape == physics::ShapeKind::Sphere) {
        DrawCylinder({p.x, .01f, p.z}, .47f, .47f, .01f, 24, {20, 30, 40, 255});
        DrawSphere(v, d.sphere_r, showcase::colors[i % 3]);
        DrawSphereWires(v, d.sphere_r + .005f, 4, 8, Fade(showcase::ink, .18f));
      } else {
        DrawCube(v, d.box_hx * 2, d.box_hy * 2, d.box_hz * 2, {38, 57, 74, 255});
        DrawCubeWires(v, d.box_hx * 2, d.box_hy * 2, d.box_hz * 2, {83, 114, 138, 255});
      }
    }
    DrawSphereWires({emitter, 7, 0}, .5f, 8, 12, showcase::colors[0]);
    EndMode3D();
    DrawText("02 / DEPTH", 44, 146, 20, showcase::ink);
    DrawText("30 seeded spheres\nStatic box tray\n3-axis collision", 44, 184, 18, showcase::muted);
    DrawText("BOX3D ALPHA", 1000, 146, 20, showcase::colors[1]);
    DrawText("Physics: m4 facade\nView: raylib 3D", 1000, 184, 18, showcase::muted);
  }
  clank::m5::Scene Scene(int seed, float emitter) const {
    clank::m5::Scene scene{seed, {}};
    for (const auto& [body, d] : items) {
      const auto p = physics::GetPosition(body);
      const bool ball = d.shape == physics::ShapeKind::Sphere;
      const std::string name = ball ? "sphere" : "obstacle";
      const float sx = 2 * (ball ? d.sphere_r : d.box_hx);
      // WHY two projections: the current scene contract has no Z field.
      scene.entities.push_back({static_cast<int>(scene.entities.size()), name + ".xy", p.x, p.y,
                                physics::GetAngle(body), sx, 2 * (ball ? d.sphere_r : d.box_hy)});
      scene.entities.push_back({static_cast<int>(scene.entities.size()), name + ".xz", p.x, p.z,
                                physics::GetAngle(body), sx, 2 * (ball ? d.sphere_r : d.box_hz)});
    }
    scene.entities.push_back({static_cast<int>(scene.entities.size()), "emitter", emitter, 7});
    return scene;
  }
};
int main(int argc, char** argv) {
  return showcase::Run<MarbleTray>(argc, argv, "marble_tray", "Marble tray / 3D",
                                   "Sphere contacts in three dimensions / fixed camera");
}
