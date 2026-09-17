#include <cmath>
#include <cstdio>

#include "m4/physics3d.hpp"

namespace {

constexpr float kDt = 1.0f / 60.0f;

bool SameVec(clank::m4::Vec3 a, clank::m4::Vec3 b) {
  return a.x == b.x && a.y == b.y && a.z == b.z;
}

bool NearVec(clank::m4::Vec3 actual, clank::m4::Vec3 expected) {
  return std::fabs(actual.x - expected.x) < 1.0e-5f && std::fabs(actual.y - expected.y) < 1.0e-5f &&
         std::fabs(actual.z - expected.z) < 1.0e-5f;
}

struct Snapshot {
  clank::m4::Vec3 position{};
  clank::m4::Vec3 velocity{};
};

Snapshot RunScripted() {
  clank::m4::World* world = clank::m4::CreateWorld({0.0f, -10.0f, 0.0f});
  if (world == nullptr) return {};
  clank::m4::BodyDef def{};
  def.type = clank::m4::BodyType::Dynamic;
  def.position = {1.0f, 4.0f, -2.0f};
  clank::m4::Body* actor = clank::m4::CreateBody(world, def);
  if (actor == nullptr) {
    clank::m4::DestroyWorld(world);
    return {};
  }
  for (int frame = 0; frame < 60; ++frame) {
    if (frame == 0) clank::m4::SetVelocity(actor, {2.0f, 0.0f, -1.0f});
    if (frame == 20) clank::m4::SetVelocity(actor, {1.0f, 0.5f, -2.0f});
    if (frame == 40) clank::m4::SetVelocity(actor, {0.0f, 1.0f, 0.0f});
    clank::m4::Step(world, kDt);
  }
  const Snapshot snapshot{clank::m4::GetPosition(actor), clank::m4::GetVelocity(actor)};
  clank::m4::DestroyBody(actor);
  clank::m4::DestroyWorld(world);
  return snapshot;
}

}  // namespace

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
  clank::m4::SetVelocity(nullptr, {8.0f, 9.0f, 10.0f});
  clank::m4::SetVelocity(stat, {8.0f, 9.0f, 10.0f});
  const clank::m4::Vec3 commanded{2.0f, 3.0f, -1.0f};
  clank::m4::SetVelocity(dyn, commanded);
  for (int i = 0; i < 60; ++i) clank::m4::Step(w, kDt);
  float y = clank::m4::GetPosition(dyn).y;
  clank::m4::Vec3 velocity = clank::m4::GetVelocity(dyn);
  clank::m4::Vec3 sp = clank::m4::GetPosition(stat);
  const clank::m4::Vec3 expected_velocity{commanded.x, commanded.y - 10.0f, commanded.z};
  bool ok = (y < 10.0f) && NearVec(velocity, expected_velocity) && (sp.x == 3.0f) &&
            (sp.y == 5.0f) && (sp.z == -2.0f) && SameVec(clank::m4::GetVelocity(stat), {});
  std::printf("m4 falling y=%.3f velocity=(%.1f,%.1f,%.1f) static=(%.1f,%.1f,%.1f) %s\n", y,
              velocity.x, velocity.y, velocity.z, sp.x, sp.y, sp.z, ok ? "PASS" : "FAIL");
  clank::m4::DestroyBody(dyn);
  clank::m4::DestroyBody(stat);
  clank::m4::DestroyWorld(w);
  if (!ok) return 1;
  const Snapshot first = RunScripted();
  const Snapshot second = RunScripted();
  const bool deterministic =
      SameVec(first.position, second.position) && SameVec(first.velocity, second.velocity);
  std::printf("m4 scripted 60-step determinism=%s\n", deterministic ? "PASS" : "FAIL");
  if (!deterministic) return 1;
  // WHY contacts: a ball falls onto a floor; a begin event must appear exactly at first touch.
  clank::m4::World* cw = clank::m4::CreateWorld({0.0f, -10.0f, 0.0f});
  if (cw == nullptr) return 1;
  clank::m4::BodyDef floor{};
  floor.shape = clank::m4::ShapeKind::Box;
  floor.box_hx = 5.0f;
  floor.box_hy = 0.5f;
  floor.box_hz = 5.0f;
  floor.position = {0.0f, 0.0f, 0.0f};
  clank::m4::Body* fl = clank::m4::CreateBody(cw, floor);
  clank::m4::BodyDef ball{};
  ball.type = clank::m4::BodyType::Dynamic;
  ball.position = {0.0f, 5.0f, 0.0f};
  clank::m4::Body* bl = clank::m4::CreateBody(cw, ball);
  if (fl == nullptr || bl == nullptr) return 1;
  for (int i = 0; i < 10; ++i) clank::m4::Step(cw, kDt);
  if (!clank::m4::GetTouches(cw).empty()) return 1;
  bool touched = false;
  for (int i = 0; i < 200 && !touched; ++i) {
    clank::m4::Step(cw, kDt);
    for (const auto& t : clank::m4::GetTouches(cw)) {
      if (!t.began) return 1;
      if ((t.a == bl && t.b == fl) || (t.a == fl && t.b == bl)) touched = true;
    }
  }
  std::printf("m4 contacts begin=%s\n", touched ? "PASS" : "FAIL");
  clank::m4::DestroyBody(bl);
  clank::m4::DestroyBody(fl);
  clank::m4::DestroyWorld(cw);
  if (!touched) return 1;
  // WHY sensors: overlap without collision response; begin on entry, end on exit, ball falls
  // through.
  clank::m4::World* sw = clank::m4::CreateWorld({0.0f, -10.0f, 0.0f});
  if (sw == nullptr) return 1;
  clank::m4::BodyDef zone{};
  zone.shape = clank::m4::ShapeKind::Box;
  zone.box_hx = 5.0f;
  zone.box_hy = 0.5f;
  zone.box_hz = 5.0f;
  zone.position = {0.0f, 0.0f, 0.0f};
  zone.sensor = true;
  clank::m4::Body* zn = clank::m4::CreateBody(sw, zone);
  clank::m4::BodyDef drop{};
  drop.type = clank::m4::BodyType::Dynamic;
  drop.position = {0.0f, 5.0f, 0.0f};
  clank::m4::Body* dr = clank::m4::CreateBody(sw, drop);
  if (zn == nullptr || dr == nullptr) return 1;
  for (int i = 0; i < 10; ++i) clank::m4::Step(sw, kDt);
  if (!clank::m4::GetOverlaps(sw).empty()) return 1;
  bool entered = false;
  bool exited = false;
  for (int i = 0; i < 300 && !exited; ++i) {
    clank::m4::Step(sw, kDt);
    for (const auto& t : clank::m4::GetOverlaps(sw)) {
      if (t.began && t.sensor == zn && t.visitor == dr) entered = true;
      if (!t.began && t.sensor == zn && t.visitor == dr) exited = true;
    }
  }
  // WHY -0.5 not -2: the loop stops at the first end event, when the ball just cleared the zone.
  if (clank::m4::GetPosition(dr).y > -0.5f) return 1;
  if (!clank::m4::GetTouches(sw).empty()) return 1;
  std::printf("m4 overlap begin+end=%s\n", entered && exited ? "PASS" : "FAIL");
  clank::m4::DestroyBody(dr);
  clank::m4::DestroyBody(zn);
  clank::m4::DestroyWorld(sw);
  if (!entered || !exited) return 1;
  // WHY destroy mid-run: end events may reference dead shapes; the facade must skip them, not trap.
  clank::m4::World* dw = clank::m4::CreateWorld({0.0f, -10.0f, 0.0f});
  if (dw == nullptr) return 1;
  clank::m4::BodyDef plate{};
  plate.shape = clank::m4::ShapeKind::Box;
  plate.box_hx = 5.0f;
  plate.box_hy = 0.5f;
  plate.box_hz = 5.0f;
  clank::m4::Body* pl = clank::m4::CreateBody(dw, plate);
  clank::m4::BodyDef shot{};
  shot.type = clank::m4::BodyType::Dynamic;
  shot.position = {0.0f, 5.0f, 0.0f};
  clank::m4::Body* sh = clank::m4::CreateBody(dw, shot);
  if (pl == nullptr || sh == nullptr) return 1;
  bool hit = false;
  for (int i = 0; i < 200 && !hit; ++i) {
    clank::m4::Step(dw, kDt);
    for (const auto& t : clank::m4::GetTouches(dw))
      if (t.began) hit = true;
  }
  if (!hit) return 1;
  clank::m4::DestroyBody(pl);
  clank::m4::Step(dw, kDt);
  bool stale = false;
  for (const auto& t : clank::m4::GetTouches(dw)) {
    (void)t;
    stale = true;
  }
  std::printf("m4 destroy mid-run=%s\n", !stale ? "PASS" : "FAIL");
  clank::m4::DestroyBody(sh);
  clank::m4::DestroyWorld(dw);
  return stale ? 1 : 0;
}
