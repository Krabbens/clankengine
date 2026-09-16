#pragma once
// WHY one loop for all samples: CLI, window, replay and timing plumbing must not be
// copy-pasted per demo; demos keep only sim + draw + scene.
#include <raylib.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "m0/foundation.hpp"
#include "m1/loop.hpp"
#include "m2/render.hpp"
#include "m5/scene.hpp"
#include "m6/input.hpp"

namespace sample_loop {

struct Config {
  const char* name = "";
  const char* header = "";
  const char* title = "";
  const char* subtitle = "";
  const char* help_keys = "";
  const char* controls = "";
  ::Color accent{70, 218, 195, 255};
  ::Color ink{226, 235, 244, 255};
  ::Color muted{135, 158, 181, 255};
};

struct Context {
  const clank::m6::Playback* playback = nullptr;
  bool headless = false;
  bool has_replay = false;
  bool isDown(int frame, clank::m6::Key key, int live) const {
    if (has_replay) return playback->IsDown(frame, key);
    return !headless && IsKeyDown(live);
  }
};

inline int Fail(const std::string& error) {
  std::fprintf(stderr, "%s\n", error.c_str());
  return 1;
}

template <class State>
int Run(int argc, char** argv, const Config& cfg,
        std::function<std::unique_ptr<State>(int seed)> make,
        std::function<void(State&, const Context&, int frame, double dt)> step,
        std::function<void(const State&)> draw,
        std::function<clank::m5::Scene(const State&, int seed)> scene) {
  if (argc == 2 && std::string(argv[1]) == "--help") {
    std::fprintf(stderr,
                 "%s [--headless] [--shot-after N] [--dump-scene out.json] "
                 "[--replay input.clk] [--seed N]\n%s",
                 cfg.name, cfg.help_keys);
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
    auto opened = clank::m1::OpenWindow({1280, 720, cfg.title, false});
    if (!opened) return Fail(opened.error());
    SetTargetFPS(60);
  }
  auto state = make(flags->seed);
  Context ctx{&playback, flags->headless, !flags->replay.empty()};
  clank::m1::Stepper stepper(1.0 / 60.0);
  const int limit = flags->shot_after >= 0 ? flags->shot_after : flags->headless ? 240 : -1;
  std::vector<double> timings;
  auto update = [&](clank::m1::Frame f, double dt) {
    const auto start = std::chrono::steady_clock::now();
    step(*state, ctx, f.number, dt);
    if (limit >= 0)
      timings.push_back(
          std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start)
              .count());
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
      draw(*state);
      DrawRectangle(0, 0, 1280, 108, {12, 19, 30, 255});
      DrawText(cfg.header, 40, 22, 18, cfg.accent);
      DrawText(cfg.title, 40, 48, 34, cfg.ink);
      DrawText(cfg.subtitle, 40, 88, 16, cfg.muted);
      DrawRectangle(0, 660, 1280, 60, {12, 19, 30, 255});
      DrawText(cfg.controls, 40, 684, 18, cfg.ink);
      DrawText(TextFormat("%s  /  %04d", stepper.paused() ? "PAUSED" : "60 HZ", stepper.next()),
               1030, 684, 18, cfg.accent);
    }
    if (done && flags->shot_after >= 0) {
      const std::string path = std::string(cfg.name) + "_frame" + std::to_string(limit) + ".png";
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
    out << clank::m5::DumpJson(scene(*state, flags->seed));
    out.close();
    if (!out)
      result = Fail("cannot write " + flags->dump_scene);
    else
      std::printf("%s\n", flags->dump_scene.c_str());
  }
  if (!timings.empty()) {
    std::sort(timings.begin(), timings.end());
    auto p = [&](double q) { return timings[static_cast<size_t>((timings.size() - 1) * q)]; };
    std::fprintf(stderr, "%s: frames=%d update_ms p50=%.4f p95=%.4f p99=%.4f\n", cfg.name,
                 stepper.next(), p(.50), p(.95), p(.99));
  }
  return result;
}
}  // namespace sample_loop
