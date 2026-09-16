#include <raylib.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "m0/foundation.hpp"
#include "m1/loop.hpp"
#include "m2/render.hpp"
#include "m5/scene.hpp"
#include "m6/input.hpp"

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

  explicit Game(int seed) {
    clank::m0::Seed(rng, static_cast<uint64_t>(seed));
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
      }
    }
    if (status == 0 && rupees >= 5) {
      const float dx = kExitX - px;
      const float dz = kExitZ - pz;
      if (dx * dx + dz * dz < 1.0f) status = 1;
    }
  }

  void Draw() const {
    const Camera3D cam{{0, 14, 10}, {0, 0, 0.3f}, {0, 1, 0}, 45, CAMERA_PERSPECTIVE};
    BeginMode3D(cam);
    DrawCube({0, -0.15f, 0}, 14.4f, 0.3f, 10.4f, {24, 42, 32, 255});
    DrawCube({0, -0.12f, 0}, 13.6f, 0.3f, 9.6f, {34, 62, 44, 255});
    for (const Wall& w : kWalls) {
      DrawCube({w.cx, 0.5f, w.cz}, w.hx * 2, 1.0f, w.hz * 2, {96, 110, 128, 255});
      DrawCubeWires({w.cx, 0.5f, w.cz}, w.hx * 2, 1.0f, w.hz * 2, {160, 180, 200, 255});
    }
    const bool open = rupees >= 5;
    DrawCylinder({kExitX, 0.05f, kExitZ}, 0.7f, 0.7f, 0.1f, 24,
                 open ? Color{255, 210, 90, 255} : Color{60, 70, 90, 255});
    DrawCylinderWires({kExitX, 0.05f, kExitZ}, 0.7f, 0.7f, 0.1f, 24,
                      open ? Color{255, 240, 180, 255} : Color{100, 110, 130, 255});
    for (const auto& g : gems) {
      if (g.taken) continue;
      DrawSphere({g.x, 0.45f, g.z}, 0.28f, {70, 218, 150, 255});
      DrawSphereWires({g.x, 0.45f, g.z}, 0.3f, 6, 8, {200, 255, 220, 255});
    }
    for (const auto& e : enemies) {
      if (!e.alive) continue;
      DrawSphere({e.x, 0.45f, e.z}, 0.42f, {210, 70, 70, 255});
      DrawSphereWires({e.x, 0.45f, e.z}, 0.43f, 6, 8, {255, 200, 200, 255});
    }
    const bool blink = inv > 0 && static_cast<int>(inv * 10) % 2 == 0;
    if (!blink) {
      DrawCube({px, 0.4f, pz}, 0.62f, 0.8f, 0.62f, {90, 200, 120, 255});
      DrawCubeWires({px, 0.4f, pz}, 0.62f, 0.8f, 0.62f, {220, 255, 230, 255});
      DrawSphere({px, 1.0f, pz}, 0.24f, {240, 220, 180, 255});
    }
    if (atk > 0) {
      const Vector3 tip{px + fx * 1.1f, 0.6f, pz + fz * 1.1f};
      DrawSphere(tip, 0.16f, {255, 240, 150, 255});
    }
    EndMode3D();
    DrawText("03 / ZELDA", 44, 146, 20, {226, 235, 244, 255});
    DrawText(TextFormat("HP %.0f/5   RUPEE %d/5   %s", static_cast<double>(hp), rupees,
                        status == 1   ? "WIN!"
                        : status == 2 ? "DEAD"
                        : open        ? "EXIT OPEN"
                                      : ""),
             44, 184, 18, {135, 158, 181, 255});
    DrawText("Find 5 rupees, reach gold pad", 44, 210, 18, {135, 158, 181, 255});
    DrawText("3D TOP-DOWN", 1000, 146, 20, {70, 218, 195, 255});
    DrawText("Arrows/WASD move, Space sword", 1000, 184, 18, {135, 158, 181, 255});
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
  if (argc == 2 && std::string(argv[1]) == "--help") {
    std::fprintf(
        stderr,
        "zelda [--headless] [--shot-after N] [--dump-scene out.json] "
        "[--replay input.clk] [--seed N]\nArrows/WASD: move | Space: sword | P: pause | N: "
        "step | Esc: exit\n");
    return 0;
  }
  auto flags = clank::m0::ParseFlags(argc, argv);
  if (!flags) {
    std::fprintf(stderr, "%s\n", flags.error().c_str());
    return 1;
  }
  clank::m6::Playback playback;
  if (!flags->replay.empty()) {
    if (auto loaded = playback.Load(flags->replay); !loaded) {
      std::fprintf(stderr, "%s\n", loaded.error().c_str());
      return 1;
    }
  }
  SetTraceLogLevel(LOG_NONE);
  if (!flags->headless) {
    if (auto opened = clank::m1::OpenWindow({1280, 720, "Zelda / 3D", false}); !opened) {
      std::fprintf(stderr, "%s\n", opened.error().c_str());
      return 1;
    }
    SetTargetFPS(60);
  }
  zelda::Game game(flags->seed);
  clank::m1::Stepper stepper(1.0 / 60.0);
  const int limit = flags->shot_after >= 0 ? flags->shot_after : flags->headless ? 240 : -1;
  bool prev_atk = false;
  std::vector<double> timings;
  auto update = [&](clank::m1::Frame f, double dt) {
    auto held = [&](clank::m6::Key key, int live1, int live2) {
      if (!flags->replay.empty()) return playback.IsDown(f.number, key);
      if (flags->headless) return false;
      return IsKeyDown(live1) || IsKeyDown(live2);
    };
    const bool up = held(clank::m6::Key::Up, KEY_UP, KEY_W);
    const bool dn = held(clank::m6::Key::Down, KEY_DOWN, KEY_S);
    const bool lf = held(clank::m6::Key::Left, KEY_LEFT, KEY_A);
    const bool rt = held(clank::m6::Key::Right, KEY_RIGHT, KEY_D);
    const bool atk = held(clank::m6::Key::Space, KEY_SPACE, KEY_SPACE);
    const auto start = std::chrono::steady_clock::now();
    game.Update(static_cast<float>(dt), up, dn, lf, rt, atk && !prev_atk);
    if (limit >= 0)
      timings.push_back(
          std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start)
              .count());
    prev_atk = atk;
  };
  int result = 0;
  while (!clank::m1::ShouldClose()) {
    if (!flags->headless && limit < 0) {
      if (IsKeyPressed(KEY_P)) {
        if (stepper.paused())
          stepper.Resume();
        else
          stepper.Pause();
      }
      if (stepper.paused() && IsKeyPressed(KEY_N)) stepper.StepOnce(update);
    }
    if (limit < 0 || stepper.next() < limit) stepper.Advance(1, update);
    const bool done = limit >= 0 && stepper.next() == limit;
    if (!flags->headless) {
      clank::m1::BeginFrame(12, 19, 30, 255);
      game.Draw();
      DrawRectangle(0, 0, 1280, 108, {12, 19, 30, 255});
      DrawText("CLANK / ZELDA LAB", 40, 22, 18, {70, 218, 195, 255});
      DrawText("Triforce garden / 3D", 40, 48, 34, {226, 235, 244, 255});
      DrawText("Seeded dungeon, sword combat, replayable input", 40, 88, 16, {135, 158, 181, 255});
      DrawRectangle(0, 660, 1280, 60, {12, 19, 30, 255});
      DrawText("ARROWS/WASD move  SPACE sword  P pause  N step", 40, 684, 18, {226, 235, 244, 255});
      DrawText(TextFormat("%s  /  %04d", stepper.paused() ? "PAUSED" : "60 HZ", stepper.next()),
               1030, 684, 18, {70, 218, 195, 255});
    }
    if (done && flags->shot_after >= 0) {
      const std::string path = "zelda_frame" + std::to_string(limit) + ".png";
      auto renderer = clank::m2::Create();
      if (auto shot = clank::m2::TakeScreenshot(renderer, path); !shot) {
        std::fprintf(stderr, "%s\n", shot.error().c_str());
        result = 1;
      } else {
        std::printf("%s\n", path.c_str());
      }
      clank::m2::Destroy(renderer);
    }
    if (!flags->headless) clank::m1::EndFrame();
    if (done) break;
  }
  clank::m1::CloseWindow();
  if (!flags->dump_scene.empty()) {
    std::ofstream out(flags->dump_scene);
    out << clank::m5::DumpJson(game.Scene(flags->seed));
    out.close();
    if (!out) {
      std::fprintf(stderr, "cannot write %s\n", flags->dump_scene.c_str());
      result = 1;
    } else {
      std::printf("%s\n", flags->dump_scene.c_str());
    }
  }
  if (!timings.empty()) {
    std::sort(timings.begin(), timings.end());
    auto pct = [&](double q) { return timings[static_cast<size_t>((timings.size() - 1) * q)]; };
    std::fprintf(stderr, "zelda: frames=%d update_ms p50=%.4f p95=%.4f p99=%.4f\n", stepper.next(),
                 pct(.50), pct(.95), pct(.99));
  }
  return result;
}
