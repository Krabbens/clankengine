#include "m4/physics3d.hpp"

#include <box3d/box3d.h>

#include <algorithm>
#include <new>
#include <vector>

namespace clank::m4 {

// WHY: constant sub-step count keeps solver accuracy identical on every step.
constexpr int kSubSteps = 4;

// WHY: yaw-only facade maps onto rotation about world up (+Y, opposite gravity).
inline b3Quat YawToQuat(float yaw) {
  const b3Vec3 up{0.0f, 1.0f, 0.0f};
  return b3MakeQuatFromAxisAngle(up, yaw);
}

// WHY: Box3D stores full quaternions; project back to yaw via world X heading.
inline float QuatToYaw(b3Quat q) {
  const b3Vec3 axisX{1.0f, 0.0f, 0.0f};
  const b3Vec3 wx = b3RotateVector(q, axisX);
  return b3Atan2(-wx.z, wx.x);
}

// WHY: one b3WorldId per facade world mirrors b3WorldDef ownership.
struct World {
  b3WorldId id{};
  std::vector<Body*> bodies{};
};

// WHY: back-pointer unlinks the wrapper on DestroyBody without a world lookup.
struct Body {
  World* world = nullptr;
  b3BodyId id{};
};

World* CreateWorld(Vec3 gravity) {
  b3WorldDef wdef = b3DefaultWorldDef();
  b3Vec3 g{gravity.x, gravity.y, gravity.z};
  wdef.gravity = g;
  b3WorldId id = b3CreateWorld(&wdef);
  if (B3_IS_NULL(id)) return nullptr;
  World* w = new (std::nothrow) World{};
  if (w == nullptr) {
    // WHY: avoid leaking the Box3D world when the wrapper allocation fails.
    b3DestroyWorld(id);
    return nullptr;
  }
  w->id = id;
  return w;
}

void DestroyWorld(World* world) {
  if (world == nullptr) return;
  // WHY: Box3D destroys all bodies with the world, so only free the wrappers.
  for (Body* b : world->bodies) delete b;
  b3DestroyWorld(world->id);
  delete world;
}

Body* CreateBody(World* world, const BodyDef& def) {
  if (world == nullptr) return nullptr;
  b3BodyDef bdef = b3DefaultBodyDef();
  bdef.type = def.type == BodyType::Dynamic ? b3_dynamicBody : b3_staticBody;
  b3Vec3 p{def.position.x, def.position.y, def.position.z};
  bdef.position = b3ToPos(p);
  bdef.rotation = YawToQuat(def.angle);
  bdef.linearVelocity = {def.linear_vel.x, def.linear_vel.y, def.linear_vel.z};
  // WHY: scalar yaw rate maps onto the world-up axis to match YawToQuat.
  bdef.angularVelocity = {0.0f, def.angular_vel, 0.0f};
  b3BodyId bid = b3CreateBody(world->id, &bdef);
  if (B3_IS_NULL(bid)) return nullptr;
  b3ShapeDef sdef = b3DefaultShapeDef();
  sdef.density = def.density;
  sdef.baseMaterial.friction = def.friction;
  // WHY always on: events are buffered until queried; no callbacks, no cost when unread.
  sdef.enableContactEvents = true;
  // WHY same treatment: any overlap with a sensor reports, whichever side asked for events.
  sdef.enableSensorEvents = true;
  sdef.isSensor = def.sensor;
  b3ShapeId sid = b3_nullShapeId;
  if (def.shape == ShapeKind::Sphere) {
    const b3Sphere sphere{{0.0f, 0.0f, 0.0f}, def.sphere_r};
    sid = b3CreateSphereShape(bid, &sdef, &sphere);
  } else {
    // WHY: Box3D v0.1.0 has no box primitive; boxes are hulls built on the stack.
    b3BoxHull box = b3MakeBoxHull(def.box_hx, def.box_hy, def.box_hz);
    sid = b3CreateHullShape(bid, &sdef, &box.base);
  }
  if (B3_IS_NULL(sid)) {
    // WHY: a body without a shape cannot satisfy the facade contract.
    b3DestroyBody(bid);
    return nullptr;
  }
  Body* b = new (std::nothrow) Body{};
  if (b == nullptr) {
    // WHY: keep Box3D and facade ownership in lockstep on allocation failure.
    b3DestroyBody(bid);
    return nullptr;
  }
  b->world = world;
  b->id = bid;
  world->bodies.push_back(b);
  return b;
}

void DestroyBody(Body* body) {
  if (body == nullptr) return;
  if (World* w = body->world; w != nullptr) {
    auto& v = w->bodies;
    v.erase(std::remove(v.begin(), v.end(), body), v.end());
  }
  b3DestroyBody(body->id);
  delete body;
}

void Step(World* world, float dt) {
  if (world == nullptr || dt <= 0.0f) return;
  // WHY: fixed dt plus constant sub-steps preserves Box3D determinism.
  b3World_Step(world->id, dt, kSubSteps);
}

Vec3 GetPosition(const Body* body) {
  if (body == nullptr) return Vec3{};
  const b3Vec3 p = b3ToVec3(b3Body_GetPosition(body->id));
  return Vec3{p.x, p.y, p.z};
}

float GetAngle(const Body* body) {
  if (body == nullptr) return 0.0f;
  return QuatToYaw(b3Body_GetRotation(body->id));
}

Vec3 GetVelocity(const Body* body) {
  if (body == nullptr) return Vec3{};
  const b3Vec3 v = b3Body_GetLinearVelocity(body->id);
  return Vec3{v.x, v.y, v.z};
}

// WHY linear scan: sample worlds stay tiny; B3_ID_EQUALS compares the opaque handle.
const Body* FindBody(const World* world, b3BodyId id) {
  for (const Body* b : world->bodies)
    if (B3_ID_EQUALS(b->id, id)) return b;
  return nullptr;
}

std::vector<TouchEvent> GetTouches(World* world) {
  std::vector<TouchEvent> out;
  if (world == nullptr) return out;
  // WHY validity first: end events may reference shapes destroyed since the last step.
  const b3ContactEvents events = b3World_GetContactEvents(world->id);
  for (int i = 0; i < events.beginCount; ++i) {
    const b3ShapeId sa = events.beginEvents[i].shapeIdA;
    const b3ShapeId sb = events.beginEvents[i].shapeIdB;
    if (!b3Shape_IsValid(sa) || !b3Shape_IsValid(sb)) continue;
    const Body* a = FindBody(world, b3Shape_GetBody(sa));
    const Body* b = FindBody(world, b3Shape_GetBody(sb));
    if (a != nullptr && b != nullptr) out.push_back(TouchEvent{a, b, true});
  }
  for (int i = 0; i < events.endCount; ++i) {
    const b3ShapeId sa = events.endEvents[i].shapeIdA;
    const b3ShapeId sb = events.endEvents[i].shapeIdB;
    if (!b3Shape_IsValid(sa) || !b3Shape_IsValid(sb)) continue;
    const Body* a = FindBody(world, b3Shape_GetBody(sa));
    const Body* b = FindBody(world, b3Shape_GetBody(sb));
    if (a != nullptr && b != nullptr) out.push_back(TouchEvent{a, b, false});
  }
  return out;
}

std::vector<OverlapEvent> GetOverlaps(World* world) {
  std::vector<OverlapEvent> out;
  if (world == nullptr) return out;
  const b3SensorEvents events = b3World_GetSensorEvents(world->id);
  for (int i = 0; i < events.beginCount; ++i) {
    const b3ShapeId ss = events.beginEvents[i].sensorShapeId;
    const b3ShapeId vs = events.beginEvents[i].visitorShapeId;
    if (!b3Shape_IsValid(ss) || !b3Shape_IsValid(vs)) continue;
    const Body* sensor = FindBody(world, b3Shape_GetBody(ss));
    const Body* visitor = FindBody(world, b3Shape_GetBody(vs));
    if (sensor != nullptr && visitor != nullptr) out.push_back(OverlapEvent{sensor, visitor, true});
  }
  for (int i = 0; i < events.endCount; ++i) {
    const b3ShapeId ss = events.endEvents[i].sensorShapeId;
    const b3ShapeId vs = events.endEvents[i].visitorShapeId;
    if (!b3Shape_IsValid(ss) || !b3Shape_IsValid(vs)) continue;
    const Body* sensor = FindBody(world, b3Shape_GetBody(ss));
    const Body* visitor = FindBody(world, b3Shape_GetBody(vs));
    if (sensor != nullptr && visitor != nullptr)
      out.push_back(OverlapEvent{sensor, visitor, false});
  }
  return out;
}

}  // namespace clank::m4
