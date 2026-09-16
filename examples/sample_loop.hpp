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
        std::function<void(State&, const Context&, int frame, double dt)> physics,
        std::function<void(const State&, clank::m2::Renderer&)> draw,
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
  auto renderer = clank::m2::Create();
  Context ctx{&playback, flags->headless, !flags->replay.empty()};
  clank::m1::Stepper stepper(1.0 / 60.0);
  const int limit = flags->shot_after >= 0 ? flags->shot_after : flags->headless ? 240 : -1;
  std::vector<double> timings;
  std::vector<double> physics_timings;
  std::vector<double> draw_timings;
  std::vector<double> frame_timings;
  std::vector<double> draw2d_counts;
  std::vector<double> draw3d_counts;
  auto update = [&](clank::m1::Frame f, double dt) {
    const auto start = std::chrono::steady_clock::now();
    step(*state, ctx, f.number, dt);
    const auto physics_start = std::chrono::steady_clock::now();
    if (physics) physics(*state, ctx, f.number, dt);
    if (limit >= 0) {
      timings.push_back(
          std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start)
              .count());
      physics_timings.push_back(physics ? std::chrono::duration<double, std::milli>(
                                              std::chrono::steady_clock::now() - physics_start)
                                              .count()
                                        : 0.0);
    }
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
    const bool stepped = (limit < 0 || stepper.next() < limit);
    const auto frame_start = std::chrono::steady_clock::now();
    if (stepped) stepper.Advance(1, update);
    const bool done = limit >= 0 && stepper.next() == limit;
    if (!flags->headless) clank::m1::BeginFrame(12, 19, 30, 255);
    // WHY timed submit: draw calls cost CPU even headless (logs); present/swap stays unmeasured.
    const auto draw_start = std::chrono::steady_clock::now();
    clank::m2::Clear(renderer, {12, 19, 30, 255});
    draw(*state, renderer);
    // WHY convert once: Config keeps raylib colors for raw callers, the facade takes its own.
    const clank::m2::Color accent{cfg.accent.r, cfg.accent.g, cfg.accent.b, cfg.accent.a};
    const clank::m2::Color ink{cfg.ink.r, cfg.ink.g, cfg.ink.b, cfg.ink.a};
    const clank::m2::Color muted{cfg.muted.r, cfg.muted.g, cfg.muted.b, cfg.muted.a};
    clank::m2::DrawRect(renderer, 0, 0, 1280, 108, {12, 19, 30, 255});
    clank::m2::DrawText(renderer, cfg.header, 40, 22, 18, accent);
    clank::m2::DrawText(renderer, cfg.title, 40, 48, 34, ink);
    clank::m2::DrawText(renderer, cfg.subtitle, 40, 88, 16, muted);
    clank::m2::DrawRect(renderer, 0, 660, 1280, 60, {12, 19, 30, 255});
    clank::m2::DrawText(renderer, cfg.controls, 40, 684, 18, ink);
    clank::m2::DrawText(
        renderer, TextFormat("%s  /  %04d", stepper.paused() ? "PAUSED" : "60 HZ", stepper.next()),
        1030, 684, 18, accent);
    const auto draw_end = std::chrono::steady_clock::now();
    if (stepped && limit >= 0) {
      using Ms = std::chrono::duration<double, std::milli>;
      draw_timings.push_back(Ms(draw_end - draw_start).count());
      frame_timings.push_back(Ms(draw_end - frame_start).count());
      draw2d_counts.push_back(static_cast<double>(clank::m2::DrawLogCount(renderer)));
      draw3d_counts.push_back(static_cast<double>(clank::m2::Draw3DLogCount(renderer)));
    }
    if (done && flags->shot_after >= 0) {
      const std::string path = std::string(cfg.name) + "_frame" + std::to_string(limit) + ".png";
      auto shot = clank::m2::TakeScreenshot(renderer, path);
      if (!shot)
        result = Fail(shot.error());
      else
        std::printf("%s\n", path.c_str());
    }
    if (!flags->headless) clank::m1::EndFrame();
    if (done) break;
  }
  clank::m1::CloseWindow();
  clank::m2::Destroy(renderer);
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
    std::sort(physics_timings.begin(), physics_timings.end());
    std::sort(draw_timings.begin(), draw_timings.end());
    std::sort(frame_timings.begin(), frame_timings.end());
    std::sort(draw2d_counts.begin(), draw2d_counts.end());
    std::sort(draw3d_counts.begin(), draw3d_counts.end());
    auto p = [&](const std::vector<double>& v, double q) {
      return v[static_cast<size_t>((v.size() - 1) * q)];
    };
    // WHY one line: agents scrape a single stderr row per run; physics is isolated when present.
    std::fprintf(stderr,
                 "%s: frames=%d update_ms p50=%.4f p95=%.4f p99=%.4f "
                 "physics_ms p50=%.4f p95=%.4f p99=%.4f "
                 "draw_ms p50=%.4f p95=%.4f p99=%.4f frame_ms p50=%.4f p95=%.4f p99=%.4f "
                 "draw2d p50=%.0f p95=%.0f draw3d p50=%.0f p95=%.0f\n",
                 cfg.name, stepper.next(), p(timings, .50), p(timings, .95), p(timings, .99),
                 p(physics_timings, .50), p(physics_timings, .95), p(physics_timings, .99),
                 p(draw_timings, .50), p(draw_timings, .95), p(draw_timings, .99),
                 p(frame_timings, .50), p(frame_timings, .95), p(frame_timings, .99),
                 p(draw2d_counts, .50), p(draw2d_counts, .95), p(draw3d_counts, .50),
                 p(draw3d_counts, .95));
  }
  return result;
}
}  // namespace sample_loop
