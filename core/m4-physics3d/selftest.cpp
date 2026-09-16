#include "m4/physics3d.hpp"

#include <cstdio>

// WHY: regression test for gravity integration; static bodies must never move.
int main() {
  clank::m4::World* w = clank::m4::CreateWorld({0.0f, -10.0f, 0.0f});
  if (w == nullptr) return 1;
  clank::m4::BodyDef fall{};
  fall.type = clank::m4::BodyType::Dynamic;
  fall.position = {0.0f, 10.0f, 0.0f};
  clank::m4::Body* dyn = clank::m4::CreateBody(w, fall);
  clank::m4::BodyDef still{};
  still.position = {3.0f, 5.0f, -2.0f};
  clank::m4::Body* stat = clank::m4::CreateBody(w, still);
  if (dyn == nullptr || stat == nullptr) return 1;
  constexpr float kDt = 1.0f / 60.0f;
  for (int i = 0; i < 60; ++i) clank::m4::Step(w, kDt);
  float y = clank::m4::GetPosition(dyn).y;
  float vy = clank::m4::GetVelocity(dyn).y;
  clank::m4::Vec3 sp = clank::m4::GetPosition(stat);
  bool ok = (y < 10.0f) && (vy < 0.0f) && (sp.x == 3.0f) && (sp.y == 5.0f) &&
            (sp.z == -2.0f);
  std::printf("m4 falling y=%.3f vy=%.3f static=(%.1f,%.1f,%.1f) %s\n", y, vy, sp.x, sp.y,
              sp.z, ok ? "PASS" : "FAIL");
  clank::m4::DestroyBody(dyn);
  clank::m4::DestroyBody(stat);
  clank::m4::DestroyWorld(w);
  return ok ? 0 : 1;
}
