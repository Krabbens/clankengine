#pragma once
// WHY: header stays STDLIB-only so Box2D never leaks into dependents.
// WHY: float angle (not b2Rot) keeps the facade tiny; backend converts via b2MakeRot.

namespace clank::m3 {

struct Vec2 {
  float x = 0.0f;
  float y = 0.0f;
};

enum class BodyType { Static, Dynamic };

enum class ShapeKind { Circle, Box };

struct BodyDef {
  Vec2 position{};
  float angle = 0.0f;
  Vec2 linear_vel{};
  float angular_vel = 0.0f;
  BodyType type = BodyType::Static;
  ShapeKind shape = ShapeKind::Circle;
  float circle_r = 0.5f;
  float box_hx = 0.5f;
  float box_hy = 0.5f;
  float density = 1.0f;
  float friction = 0.4f;
};

struct World;
struct Body;

World* CreateWorld(Vec2 gravity);
void DestroyWorld(World* world);
Body* CreateBody(World* world, const BodyDef& def);
void DestroyBody(Body* body);
// WHY: fixed-dt only; variable dt breaks Box2D determinism guarantees.
void Step(World* world, float dt);
Vec2 GetPosition(const Body* body);
float GetAngle(const Body* body);
Vec2 GetVelocity(const Body* body);

}  // namespace clank::m3
