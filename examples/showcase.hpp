#pragma once
#include <raylib.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <vector>

#include "m0/foundation.hpp"
#include "m1/loop.hpp"
#include "m2/render.hpp"
#include "m5/scene.hpp"
#include "m6/input.hpp"

namespace showcase {
constexpr Color ink{226, 235, 244, 255}, muted{135, 158, 181, 255};
constexpr Color colors[] = {{70, 218, 195, 255}, {255, 186, 99, 255}, {139, 151, 255, 255}};

inline int Fail(const std::string& error) {
  std::fprintf(stderr, "%s\n", error.c_str());
  return 1;
}

template <class Demo>
int Run(int argc, char** argv, const char* name, const char* title, const char* subtitle) {
  if (argc == 2 && std::string(argv[1]) == "--help") {
    std::fprintf(stderr,
                 "%s [--headless] [--shot-after N] [--dump-scene out.json] "
                 "[--replay input.clk] [--seed N]\n"
                 "Left/Right: emitter | Space: drop | P: pause | N: step | Esc: exit\n",
                 name);
    return 0;
  }
  auto flags = clank::m0::ParseFlags(argc, argv);
  if (!flags) return Fail(flags.error());
  clank::m6::Playback playback;
  if (!flags->replay.empty()) {
    auto loaded = playback.Load(flags->replay);
    if (!loaded) return Fail(loaded.error());
  }
  SetTraceLogLevel(LOG_NONE);
  if (!flags->headless) {
    auto opened = clank::m1::OpenWindow({1280, 720, title, false});
    if (!opened) return Fail(opened.error());
    SetTargetFPS(60);
  }
  Demo demo(flags->seed);
  clank::m1::Stepper stepper(1.0 / 60.0);
  const int limit = flags->shot_after >= 0 ? flags->shot_after : flags->headless ? 240 : -1;
  float emitter = 0;
  bool previous_drop = false;
  std::vector<double> timings;
  auto update = [&](clank::m1::Frame f, double dt) {
    auto down = [&](clank::m6::Key key, int live) {
      return !flags->replay.empty() ? playback.IsDown(f.number, key)
                                    : !flags->headless && IsKeyDown(live);
    };
    emitter = std::clamp(emitter + 5 * static_cast<float>(dt) *
                                       (down(clank::m6::Key::Right, KEY_RIGHT) -
                                        down(clank::m6::Key::Left, KEY_LEFT)),
                         -5.0f, 5.0f);
    bool drop = down(clank::m6::Key::Space, KEY_SPACE);
    const auto start = std::chrono::steady_clock::now();
    if (drop && !previous_drop) demo.Drop(emitter);
    demo.Step(static_cast<float>(dt));
    if (limit >= 0)
      timings.push_back(
          std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start)
              .count());
    previous_drop = drop;
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
      demo.Draw(emitter);
      DrawRectangle(0, 0, 1280, 108, {12, 19, 30, 255});
      DrawText("CLANK / PHYSICS LAB", 40, 22, 18, colors[0]);
      DrawText(title, 40, 48, 34, ink);
      DrawText(subtitle, 40, 88, 16, muted);
      DrawRectangle(0, 660, 1280, 60, {12, 19, 30, 255});
      DrawText("LEFT / RIGHT  move emitter     SPACE  drop     P  pause     N  step", 40, 684, 18,
               ink);
      DrawText(TextFormat("%s  /  %04d", stepper.paused() ? "PAUSED" : "60 HZ", stepper.next()),
               1030, 684, 18, colors[0]);
    }
    if (done && flags->shot_after >= 0) {
      const std::string path = std::string(name) + "_frame" + std::to_string(limit) + ".png";
      auto renderer = clank::m2::Create();
      auto shot = clank::m2::TakeScreenshot(renderer, path);
      clank::m2::Destroy(renderer);
      if (!shot)
        result = Fail(shot.error());
      else
        std::printf("%s\n", path.c_str());
    }
    if (!flags->headless) clank::m1::EndFrame();
    if (done) break;
  }
  clank::m1::CloseWindow();
  if (!flags->dump_scene.empty()) {
    std::ofstream out(flags->dump_scene);
    out << clank::m5::DumpJson(demo.Scene(flags->seed, emitter));
    out.close();
    if (!out)
      result = Fail("cannot write " + flags->dump_scene);
    else
      std::printf("%s\n", flags->dump_scene.c_str());
  }
  if (!timings.empty()) {
    std::sort(timings.begin(), timings.end());
    auto p = [&](double q) { return timings[static_cast<size_t>((timings.size() - 1) * q)]; };
    std::fprintf(stderr, "%s: frames=%d update_ms p50=%.4f p95=%.4f p99=%.4f\n", name,
                 stepper.next(), p(.50), p(.95), p(.99));
  }
  return result;
}
}  // namespace showcase
