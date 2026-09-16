#include "m3/physics2d.hpp"

#include <box2d/box2d.h>

#include <algorithm>
#include <new>
#include <vector>

namespace clank::m3 {

// TODO: no contact/overlap events yet; expose b2World_GetContactEvents when facade grows.
// WHY: b2 ids are handles by value; wrappers own lifetime, Box2D owns simulation.
struct World;
struct Body {
  b2BodyId id{};
  World* world = nullptr;
};

// WHY: one b2 world per facade world mirrors b2WorldDef::gravity.
struct World {
  b2WorldId id{};
  std::vector<Body*> bodies{};
};

World* CreateWorld(Vec2 gravity) {
  // WHY: defaults pin Box2D tuning; only gravity comes from the caller.
  b2WorldDef def = b2DefaultWorldDef();
  def.gravity = {gravity.x, gravity.y};
  b2WorldId id = b2CreateWorld(&def);
  World* w = new (std::nothrow) World{};
  if (w == nullptr) {
    b2DestroyWorld(id);
    return nullptr;
  }
  w->id = id;
  return w;
}

void DestroyWorld(World* world) {
  if (world == nullptr) return;
  // WHY: b2DestroyWorld reclaims b2 bodies; wrappers hold ids only.
  for (Body* b : world->bodies) delete b;
  b2DestroyWorld(world->id);
  delete world;
}

Body* CreateBody(World* world, const BodyDef& def) {
  if (world == nullptr) return nullptr;
  // WHY: b2 defs are stack temporaries; Box2D clones all creation parameters.
  b2BodyDef bdef = b2DefaultBodyDef();
  bdef.type = def.type == BodyType::Dynamic ? b2_dynamicBody : b2_staticBody;
  bdef.position = {def.position.x, def.position.y};
  bdef.rotation = b2MakeRot(def.angle);
  bdef.linearVelocity = {def.linear_vel.x, def.linear_vel.y};
  bdef.angularVelocity = def.angular_vel;
  b2BodyId id = b2CreateBody(world->id, &bdef);
  // WHY: density/friction ride the shape def; Box2D derives mass from geometry.
  b2ShapeDef sdef = b2DefaultShapeDef();
  sdef.density = def.density;
  sdef.material.friction = def.friction;
  if (def.shape == ShapeKind::Circle) {
    b2Circle circle = {{0.0f, 0.0f}, def.circle_r};
    b2CreateCircleShape(id, &sdef, &circle);
  } else {
    // WHY: b2MakeBox builds the convex polygon; facade stores half-extents only.
    b2Polygon box = b2MakeBox(def.box_hx, def.box_hy);
    b2CreatePolygonShape(id, &sdef, &box);
  }
  Body* b = new (std::nothrow) Body{};
  if (b == nullptr) {
    b2DestroyBody(id);
    return nullptr;
  }
  b->id = id;
  b->world = world;
  world->bodies.push_back(b);
  return b;
}

void DestroyBody(Body* body) {
  if (body == nullptr) return;
  // WHY: detach wrapper first so DestroyWorld never sees a dangling entry.
  b2DestroyBody(body->id);
  if (World* w = body->world; w != nullptr) {
    auto& v = w->bodies;
    v.erase(std::remove(v.begin(), v.end(), body), v.end());
  }
  delete body;
}

void Step(World* world, float dt) {
  if (world == nullptr || dt <= 0.0f) return;
  // WHY: fixed dt only; 4 substeps is the upstream Box2D default for accuracy.
  b2World_Step(world->id, dt, 4);
}

Vec2 GetPosition(const Body* body) {
  if (body == nullptr) return Vec2{};
  // WHY: query b2 origin directly; facade keeps no shadow state.
  b2Vec2 p = b2Body_GetPosition(body->id);
  return Vec2{p.x, p.y};
}

float GetAngle(const Body* body) {
  if (body == nullptr) return 0.0f;
  // WHY: facade uses float angle; backend converts from b2Rot deterministically.
  b2Rot r = b2Body_GetRotation(body->id);
  return b2Rot_GetAngle(r);
}

Vec2 GetVelocity(const Body* body) {
  if (body == nullptr) return Vec2{};
  b2Vec2 v = b2Body_GetLinearVelocity(body->id);
  return Vec2{v.x, v.y};
}

}  // namespace clank::m3
