#include <cstdio>

#include "m3/physics2d.hpp"

// WHY: regression test for gravity integration; static bodies must never move.
int main() {
  clank::m3::World* w = clank::m3::CreateWorld({0.0f, -10.0f});
  if (w == nullptr) return 1;
  clank::m3::BodyDef fall{};
  fall.type = clank::m3::BodyType::Dynamic;
  fall.position = {0.0f, 10.0f};
  clank::m3::Body* dyn = clank::m3::CreateBody(w, fall);
  clank::m3::BodyDef still{};
  still.position = {3.0f, 5.0f};
  clank::m3::Body* stat = clank::m3::CreateBody(w, still);
  if (dyn == nullptr || stat == nullptr) return 1;
  constexpr float kDt = 1.0f / 60.0f;
  for (int i = 0; i < 60; ++i) clank::m3::Step(w, kDt);
  float y = clank::m3::GetPosition(dyn).y;
  float vy = clank::m3::GetVelocity(dyn).y;
  clank::m3::Vec2 sp = clank::m3::GetPosition(stat);
  bool ok = (y < 10.0f) && (vy < 0.0f) && (sp.x == 3.0f) && (sp.y == 5.0f);
  std::printf("m3 falling y=%.3f vy=%.3f static=(%.1f,%.1f) %s\n", y, vy, sp.x, sp.y,
              ok ? "PASS" : "FAIL");
  clank::m3::DestroyBody(dyn);
  clank::m3::DestroyBody(stat);
  clank::m3::DestroyWorld(w);
  if (!ok) return 1;
  // WHY contacts: a ball falls onto a floor; a begin event must appear exactly at first touch.
  clank::m3::World* cw = clank::m3::CreateWorld({0.0f, -10.0f});
  if (cw == nullptr) return 1;
  clank::m3::BodyDef floor{};
  floor.shape = clank::m3::ShapeKind::Box;
  floor.box_hx = 5.0f;
  floor.box_hy = 0.5f;
  floor.position = {0.0f, 0.0f};
  clank::m3::Body* fl = clank::m3::CreateBody(cw, floor);
  clank::m3::BodyDef ball{};
  ball.type = clank::m3::BodyType::Dynamic;
  ball.position = {0.0f, 5.0f};
  clank::m3::Body* bl = clank::m3::CreateBody(cw, ball);
  if (fl == nullptr || bl == nullptr) return 1;
  for (int i = 0; i < 10; ++i) clank::m3::Step(cw, kDt);
  if (!clank::m3::GetTouches(cw).empty()) return 1;
  bool touched = false;
  for (int i = 0; i < 200 && !touched; ++i) {
    clank::m3::Step(cw, kDt);
    for (const auto& t : clank::m3::GetTouches(cw)) {
      if (!t.began) return 1;
      if ((t.a == bl && t.b == fl) || (t.a == fl && t.b == bl)) touched = true;
    }
  }
  std::printf("m3 contacts begin=%s\n", touched ? "PASS" : "FAIL");
  clank::m3::DestroyBody(bl);
  clank::m3::DestroyBody(fl);
  clank::m3::DestroyWorld(cw);
  if (!touched) return 1;
  // WHY sensors: overlap without collision response; begin on entry, end on exit, ball falls
  // through.
  clank::m3::World* sw = clank::m3::CreateWorld({0.0f, -10.0f});
  if (sw == nullptr) return 1;
  clank::m3::BodyDef zone{};
  zone.shape = clank::m3::ShapeKind::Box;
  zone.box_hx = 5.0f;
  zone.box_hy = 0.5f;
  zone.position = {0.0f, 0.0f};
  zone.sensor = true;
  clank::m3::Body* zn = clank::m3::CreateBody(sw, zone);
  clank::m3::BodyDef drop{};
  drop.type = clank::m3::BodyType::Dynamic;
  drop.position = {0.0f, 5.0f};
  clank::m3::Body* dr = clank::m3::CreateBody(sw, drop);
  if (zn == nullptr || dr == nullptr) return 1;
  for (int i = 0; i < 10; ++i) clank::m3::Step(sw, kDt);
  if (!clank::m3::GetOverlaps(sw).empty()) return 1;
  bool entered = false;
  bool exited = false;
  for (int i = 0; i < 300 && !exited; ++i) {
    clank::m3::Step(sw, kDt);
    for (const auto& t : clank::m3::GetOverlaps(sw)) {
      if (t.began && t.sensor == zn && t.visitor == dr) entered = true;
      if (!t.began && t.sensor == zn && t.visitor == dr) exited = true;
    }
  }
  // WHY -0.5 not -2: the loop stops at the first end event, when the ball just cleared the zone.
  if (clank::m3::GetPosition(dr).y > -0.5f) return 1;
  if (!clank::m3::GetTouches(sw).empty()) return 1;
  std::printf("m3 overlap begin+end=%s\n", entered && exited ? "PASS" : "FAIL");
  clank::m3::DestroyBody(dr);
  clank::m3::DestroyBody(zn);
  clank::m3::DestroyWorld(sw);
  return entered && exited ? 0 : 1;
}
