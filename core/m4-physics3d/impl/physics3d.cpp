#include "m4/physics3d.hpp"

#include <algorithm>
#include <new>
#include <vector>

namespace clank::m4 {

// WHY: one gravity vector per world mirrors b3WorldDef::gravity.
struct World {
  Vec3 gravity{};
  std::vector<Body*> bodies{};
};

// WHY: yaw-only rotation keeps the stub tiny; full b3Quat arrives in wave 2.
struct Body {
  World* world = nullptr;
  Vec3 position{};
  float angle = 0.0f;
  Vec3 velocity{};
  float angular_vel = 0.0f;
  BodyType type = BodyType::Static;
};

World* CreateWorld(Vec3 gravity) {
  // STUB: replaced by Box3D backend in wave 2.
  World* w = new (std::nothrow) World{};
  if (w != nullptr) w->gravity = gravity;
  return w;
}

void DestroyWorld(World* world) {
  // STUB: replaced by Box3D backend in wave 2.
  if (world == nullptr) return;
  for (Body* b : world->bodies) delete b;
  delete world;
}

Body* CreateBody(World* world, const BodyDef& def) {
  // STUB: replaced by Box3D backend in wave 2.
  if (world == nullptr) return nullptr;
  Body* b = new (std::nothrow) Body{};
  if (b == nullptr) return nullptr;
  b->world = world;
  b->position = def.position;
  b->angle = def.angle;
  b->velocity = def.linear_vel;
  b->angular_vel = def.angular_vel;
  b->type = def.type;
  world->bodies.push_back(b);
  return b;
}

void DestroyBody(Body* body) {
  // STUB: replaced by Box3D backend in wave 2.
  if (body == nullptr) return;
  if (World* w = body->world; w != nullptr) {
    auto& v = w->bodies;
    v.erase(std::remove(v.begin(), v.end(), body), v.end());
  }
  delete body;
}

void Step(World* world, float dt) {
  // STUB: replaced by Box3D backend in wave 2.
  if (world == nullptr || dt <= 0.0f) return;
  for (Body* b : world->bodies) {
    if (b == nullptr || b->type == BodyType::Static) continue;
    // WHY: semi-implicit Euler (velocity first) stays stable under gravity.
    b->velocity.x += world->gravity.x * dt;
    b->velocity.y += world->gravity.y * dt;
    b->velocity.z += world->gravity.z * dt;
    b->position.x += b->velocity.x * dt;
    b->position.y += b->velocity.y * dt;
    b->position.z += b->velocity.z * dt;
    b->angle += b->angular_vel * dt;
  }
}

Vec3 GetPosition(const Body* body) {
  // STUB: replaced by Box3D backend in wave 2.
  return body != nullptr ? body->position : Vec3{};
}

float GetAngle(const Body* body) {
  // STUB: replaced by Box3D backend in wave 2.
  return body != nullptr ? body->angle : 0.0f;
}

Vec3 GetVelocity(const Body* body) {
  // STUB: replaced by Box3D backend in wave 2.
  return body != nullptr ? body->velocity : Vec3{};
}

}  // namespace clank::m4
