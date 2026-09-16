#pragma once
// WHY: header stays STDLIB-only so Box3D never leaks outside this module.
// WHY: scalar yaw (not b3Quat) keeps the facade tiny; backend converts to quaternion.
#include <vector>

namespace clank::m4 {

struct Vec3 {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};

enum class BodyType { Static, Dynamic };

enum class ShapeKind { Sphere, Box };

struct BodyDef {
  Vec3 position{};
  float angle = 0.0f;
  Vec3 linear_vel{};
  float angular_vel = 0.0f;
  BodyType type = BodyType::Static;
  ShapeKind shape = ShapeKind::Sphere;
  float sphere_r = 0.5f;
  float box_hx = 0.5f;
  float box_hy = 0.5f;
  float box_hz = 0.5f;
  float density = 1.0f;
  float friction = 0.4f;
  // WHY opt-in: sensors skip collision response, so only explicit triggers observe overlaps.
  bool sensor = false;
};

struct World;
struct Body;

World* CreateWorld(Vec3 gravity);
void DestroyWorld(World* world);
Body* CreateBody(World* world, const BodyDef& def);
void DestroyBody(Body* body);
// WHY: fixed-dt only; variable dt breaks Box3D determinism guarantees.
void Step(World* world, float dt);
Vec3 GetPosition(const Body* body);
float GetAngle(const Body* body);
Vec3 GetVelocity(const Body* body);
// WHY snapshot, not drain: Box3D keeps events until the next Step, so this reflects the last step.
struct TouchEvent {
  const Body* a = nullptr;
  const Body* b = nullptr;
  bool began = true;
};
std::vector<TouchEvent> GetTouches(World* world);
// WHY sensor/visitor, not a/b: Box3D always reports which side is the sensor.
struct OverlapEvent {
  const Body* sensor = nullptr;
  const Body* visitor = nullptr;
  bool began = true;
};
std::vector<OverlapEvent> GetOverlaps(World* world);

}  // namespace clank::m4
