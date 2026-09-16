#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <memory>

#include "m0/foundation.hpp"
#include "m5/scene.hpp"
#include "m6/input.hpp"
#include "sample_loop.hpp"

namespace zelda {
// WHY top-down XZ dungeon: one fixed camera reads as Zelda without a follow cam.
constexpr float kMinX = -6.6f;
constexpr float kMaxX = 6.6f;
constexpr float kMinZ = -4.6f;
constexpr float kMaxZ = 4.6f;
constexpr float kPlayerSpeed = 4.0f;
constexpr float kPlayerR = 0.35f;
constexpr float kEnemySpeed = 1.6f;
constexpr float kWanderSpeed = 0.8f;
constexpr float kExitX = 5.8f;
constexpr float kExitZ = -3.8f;

struct Wall {
  float cx = 0;
  float cz = 0;
  float hx = 1;
  float hz = 1;
};

constexpr Wall kWalls[] = {
    {-1.5f, 1.0f, 2.0f, 0.25f},
    {1.5f, -1.0f, 2.0f, 0.25f},
};

struct Enemy {
  float x = 0;
  float z = 0;
  float wx = 1;
  float wz = 0;
  float wt = 0;
  bool alive = true;
};

struct Rupee {
  float x = 0;
  float z = 0;
  bool taken = false;
};

void Collide(float& x, float& z, float r) {
  x = std::clamp(x, kMinX, kMaxX);
  z = std::clamp(z, kMinZ, kMaxZ);
  for (const Wall& w : kWalls) {
    const float cx = std::clamp(x, w.cx - w.hx, w.cx + w.hx);
    const float cz = std::clamp(z, w.cz - w.hz, w.cz + w.hz);
    float dx = x - cx;
    float dz = z - cz;
    const float d2 = dx * dx + dz * dz;
    if (d2 >= r * r) continue;
    if (d2 > 1e-8f) {
      const float d = std::sqrt(d2);
      x = cx + dx / d * r;
      z = cz + dz / d * r;
    } else {
      z = w.cz + (z >= w.cz ? w.hz + r : -w.hz - r);
    }
  }
}

struct Game {
  float px = -5.5f;
  float pz = 3.5f;
  float fx = 1;
  float fz = 0;
  float atk = 0;
  float inv = 0;
  float hp = 5;
  int rupees = 0;
  int status = 0;  // 0 playing, 1 won, 2 dead
  Enemy enemies[3]{};
  Rupee gems[5]{};
  clank::m0::Rng rng{};
  clank::m2::Texture grass{};
  clank::m2::Audio audio{};
  clank::m2::Sfx pickup{};
  clank::m2::Sfx sword{};
  clank::m2::Sfx fanfare{};

  explicit Game(int seed) {
    clank::m0::Seed(rng, static_cast<uint64_t>(seed));
    // WHY tones here: square pickup, saw sword, square fanfare; silent without a device.
    audio = clank::m2::OpenAudio();
    pickup = clank::m2::LoadTone(audio, 880, 90, 0);
    sword = clank::m2::LoadTone(audio, 180, 140, 1);
    fanfare = clank::m2::LoadTone(audio, 660, 350, 0);
    // WHY checker grass: procedural pattern, no binary assets, deterministic on every backend.
    grass = clank::m2::LoadChecker(8, {24, 42, 32, 255}, {34, 62, 44, 255});
    const float bx[5] = {-3.0f, 3.0f, 0.0f, -4.0f, 4.0f};
    const float bz[5] = {3.0f, 3.0f, 0.0f, -3.0f, 2.0f};
    for (int i = 0; i < 5; ++i) {
      gems[i].x = bx[i] + static_cast<float>(clank::m0::NextFloat01(rng)) * 0.8f - 0.4f;
      gems[i].z = bz[i] + static_cast<float>(clank::m0::NextFloat01(rng)) * 0.8f - 0.4f;
    }
    const float ex[3] = {0.0f, 0.0f, 3.0f};
    const float ez[3] = {2.5f, -2.5f, 0.0f};
    for (int i = 0; i < 3; ++i) {
      enemies[i].x = ex[i] + static_cast<float>(clank::m0::NextFloat01(rng)) * 1.0f - 0.5f;
      enemies[i].z = ez[i] + static_cast<float>(clank::m0::NextFloat01(rng)) * 1.0f - 0.5f;
      const float a = static_cast<float>(clank::m0::NextFloat01(rng)) * 6.2831853f;
      enemies[i].wx = std::cos(a);
      enemies[i].wz = std::sin(a);
      enemies[i].wt = 1.0f + static_cast<float>(clank::m0::NextFloat01(rng)) * 2.0f;
    }
  }

  ~Game() {
    clank::m2::UnloadSfx(audio, pickup);
    clank::m2::UnloadSfx(audio, sword);
    clank::m2::UnloadSfx(audio, fanfare);
    clank::m2::UnloadTexture(grass);
    clank::m2::CloseAudio(audio);
  }

  void Update(float dt, bool up, bool down, bool left, bool right, bool atk_edge) {
    if (status == 2) return;
    const float ix = (right ? 1.0f : 0.0f) - (left ? 1.0f : 0.0f);
    const float iz = (down ? 1.0f : 0.0f) - (up ? 1.0f : 0.0f);
    if (ix != 0.0f || iz != 0.0f) {
      const float len = std::sqrt(ix * ix + iz * iz);
      fx = ix / len;
      fz = iz / len;
      px += fx * kPlayerSpeed * dt;
      pz += fz * kPlayerSpeed * dt;
      Collide(px, pz, kPlayerR);
    }
    if (atk > 0) atk -= dt;
    if (atk_edge && atk <= 0) {
      atk = 0.25f;
      clank::m2::PlaySfx(audio, sword);
      // WHY spin, not cone: fixed camera + replay input make facing fiddly;
      // a 360-degree spin keeps combat deterministic and fun with one button.
      for (auto& e : enemies) {
        if (!e.alive) continue;
        const float dx = e.x - px;
        const float dz = e.z - pz;
        if (dx * dx + dz * dz < 1.96f) e.alive = false;
      }
    }
    if (inv > 0) inv -= dt;
    for (auto& e : enemies) {
      if (!e.alive) continue;
      const float dx = px - e.x;
      const float dz = pz - e.z;
      const float d = std::sqrt(dx * dx + dz * dz);
      float vx;
      float vz;
      if (status == 0 && d < 6.0f && d > 1e-6f) {
        vx = dx / d * kEnemySpeed;
        vz = dz / d * kEnemySpeed;
      } else {
        e.wt -= dt;
        if (e.wt <= 0) {
          const float a = static_cast<float>(clank::m0::NextFloat01(rng)) * 6.2831853f;
          e.wx = std::cos(a);
          e.wz = std::sin(a);
          e.wt = 1.0f + static_cast<float>(clank::m0::NextFloat01(rng)) * 2.0f;
        }
        vx = e.wx * kWanderSpeed;
        vz = e.wz * kWanderSpeed;
      }
      e.x += vx * dt;
      e.z += vz * dt;
      Collide(e.x, e.z, 0.4f);
      const float hx = px - e.x;
      const float hz = pz - e.z;
      if (status == 0 && inv <= 0 && hx * hx + hz * hz < 0.49f) {
        hp -= 1.0f;
        inv = 1.0f;
        if (hp <= 0) {
          hp = 0;
          status = 2;
        }
      }
    }
    for (auto& g : gems) {
      if (g.taken) continue;
      const float dx = g.x - px;
      const float dz = g.z - pz;
      if (dx * dx + dz * dz < 0.49f) {
        g.taken = true;
        ++rupees;
        clank::m2::PlaySfx(audio, pickup);
      }
    }
    if (status == 0 && rupees >= 5) {
      const float dx = kExitX - px;
      const float dz = kExitZ - pz;
      // WHY inside status==0: the outer guard makes this edge-triggered, so the fanfare fires once.
      if (dx * dx + dz * dz < 1.0f) {
        status = 1;
        clank::m2::PlaySfx(audio, fanfare);
      }
    }
  }

  void Draw(clank::m2::Renderer& renderer) const {
    namespace m2 = clank::m2;
    const m2::Camera cam{{0, 14, 10}, {0, 0, 0.3f}, {0, 1, 0}, 45};
    m2::BeginMode3D(renderer, cam);
    m2::DrawCubeTextured(renderer, 0, -0.15f, 0, 14.4f, 0.3f, 10.4f, grass, {255, 255, 255, 255});
    m2::DrawCubeTextured(renderer, 0, -0.12f, 0, 13.6f, 0.3f, 9.6f, grass, {255, 255, 255, 255});
    for (const Wall& w : kWalls) {
      m2::DrawCube(renderer, w.cx, 0.5f, w.cz, w.hx * 2, 1.0f, w.hz * 2, {96, 110, 128, 255});
      m2::DrawCubeWires(renderer, w.cx, 0.5f, w.cz, w.hx * 2, 1.0f, w.hz * 2, {160, 180, 200, 255});
    }
    const bool open = rupees >= 5;
    m2::DrawCylinder(renderer, kExitX, 0.05f, kExitZ, 0.7f, 0.7f, 0.1f, 24,
                     open ? m2::Color{255, 210, 90, 255} : m2::Color{60, 70, 90, 255});
    m2::DrawCylinderWires(renderer, kExitX, 0.05f, kExitZ, 0.7f, 0.7f, 0.1f, 24,
                          open ? m2::Color{255, 240, 180, 255} : m2::Color{100, 110, 130, 255});
    for (const auto& g : gems) {
      if (g.taken) continue;
      m2::DrawSphere(renderer, g.x, 0.45f, g.z, 0.28f, {70, 218, 150, 255});
      m2::DrawSphereWires(renderer, g.x, 0.45f, g.z, 0.3f, 6, 8, {200, 255, 220, 255});
    }
    for (const auto& e : enemies) {
      if (!e.alive) continue;
      m2::DrawSphere(renderer, e.x, 0.45f, e.z, 0.42f, {210, 70, 70, 255});
      m2::DrawSphereWires(renderer, e.x, 0.45f, e.z, 0.43f, 6, 8, {255, 200, 200, 255});
    }
    const bool blink = inv > 0 && static_cast<int>(inv * 10) % 2 == 0;
    if (!blink) {
      m2::DrawCube(renderer, px, 0.4f, pz, 0.62f, 0.8f, 0.62f, {90, 200, 120, 255});
      m2::DrawCubeWires(renderer, px, 0.4f, pz, 0.62f, 0.8f, 0.62f, {220, 255, 230, 255});
      m2::DrawSphere(renderer, px, 1.0f, pz, 0.24f, {240, 220, 180, 255});
    }
    if (atk > 0) {
      m2::DrawSphere(renderer, px + fx * 1.1f, 0.6f, pz + fz * 1.1f, 0.16f, {255, 240, 150, 255});
    }
    m2::EndMode3D(renderer);
    m2::DrawText(renderer, "03 / ZELDA", 44, 146, 20, {226, 235, 244, 255});
    m2::DrawText(renderer,
                 TextFormat("HP %.0f/5   RUPEE %d/5   %s", static_cast<double>(hp), rupees,
                            status == 1   ? "WIN!"
                            : status == 2 ? "DEAD"
                            : open        ? "EXIT OPEN"
                                          : ""),
                 44, 184, 18, {135, 158, 181, 255});
    m2::DrawText(renderer, "Find 5 rupees, reach gold pad", 44, 210, 18, {135, 158, 181, 255});
    m2::DrawText(renderer, "3D TOP-DOWN", 1000, 146, 20, {70, 218, 195, 255});
    m2::DrawText(renderer, "Arrows/WASD move, Space sword", 1000, 184, 18, {135, 158, 181, 255});
  }

  clank::m5::Scene Scene(int seed) const {
    clank::m5::Scene s{seed, {}};
    int id = 0;
    const float ang = std::atan2(fz, fx);
    s.entities.push_back({id++, "player", px, pz, ang, 0.62f, 0.62f, 0.4f});
    for (const auto& e : enemies) {
      if (!e.alive) continue;
      s.entities.push_back({id++, "enemy", e.x, e.z, 0, 0.84f, 0.84f, 0.45f});
    }
    for (const auto& g : gems) {
      if (g.taken) continue;
      s.entities.push_back({id++, "rupee", g.x, g.z, 0, 0.56f, 0.56f, 0.45f});
    }
    for (const Wall& w : kWalls)
      s.entities.push_back({id++, "wall", w.cx, w.cz, 0, w.hx * 2, w.hz * 2, 0.5f});
    s.entities.push_back({id++, "exit", kExitX, kExitZ, open() ? 1.0f : 0.0f, 1.4f, 1.4f, 0.05f});
    s.entities.push_back({id++, "state", hp, static_cast<float>(rupees), static_cast<float>(status),
                          1, 1, static_cast<float>(atk)});
    return s;
  }

  bool open() const { return rupees >= 5; }
};
}  // namespace zelda

int main(int argc, char** argv) {
  bool prev_atk = false;
  return sample_loop::Run<zelda::Game>(
      argc, argv,
      {"zelda",
       "CLANK / ZELDA LAB",
       "Triforce garden / 3D",
       "Seeded dungeon, sword combat, replayable input",
       "Arrows/WASD: move | Space: sword | P: pause | N: step | Esc: exit\n",
       "ARROWS/WASD move  SPACE sword  P pause  N step",
       {70, 218, 195, 255},
       {226, 235, 244, 255},
       {135, 158, 181, 255}},
      [](int seed) { return std::make_unique<zelda::Game>(seed); },
      [&](zelda::Game& game, const sample_loop::Context& ctx, int frame, double dt) {
        // WHY two calls: with a replay both read playback (same value); live they OR arrows+WASD.
        auto held = [&](clank::m6::Key key, int live1, int live2) {
          return ctx.isDown(frame, key, live1) || ctx.isDown(frame, key, live2);
        };
        const bool up = held(clank::m6::Key::Up, KEY_UP, KEY_W);
        const bool dn = held(clank::m6::Key::Down, KEY_DOWN, KEY_S);
        const bool lf = held(clank::m6::Key::Left, KEY_LEFT, KEY_A);
        const bool rt = held(clank::m6::Key::Right, KEY_RIGHT, KEY_D);
        const bool atk = held(clank::m6::Key::Space, KEY_SPACE, KEY_SPACE);
        game.Update(static_cast<float>(dt), up, dn, lf, rt, atk && !prev_atk);
        prev_atk = atk;
      },
      [](const zelda::Game& game, clank::m2::Renderer& renderer) { game.Draw(renderer); },
      [](const zelda::Game& game, int seed) { return game.Scene(seed); });
}
