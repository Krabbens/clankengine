// clank app: agent-first game loop on m0 (flags) + m1 (loop) + m2 (render) + m5 (scene) + m6
// (input). Logs -> stderr; machine-readable result paths -> stdout.
#include <cstdio>
#include <fstream>
#include <string>

#include "m0/foundation.hpp"
#include "m1/loop.hpp"
#include "m2/render.hpp"
#include "m5/scene.hpp"
#include "m6/input.hpp"

namespace {

bool WriteText(const std::string& path, const std::string& text) {
  std::ofstream out(path, std::ios::binary);
  if (!out) return false;
  out << text;
  return static_cast<bool>(out);
}

bool HasHelp(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    const std::string t = argv[i];
    if (t == "--help" || t == "-h") return true;
  }
  return false;
}

clank::m5::Scene DemoScene(int seed) {
  clank::m5::Scene s;
  s.seed = seed;
  s.entities.push_back({1, "player one", 0.0f, 0.0f, 0.0f, 1.0f, 1.0f});
  return s;
}

std::string ShotPath(int frame) { return "/tmp/clank_frame" + std::to_string(frame) + ".png"; }

}  // namespace

int main(int argc, char** argv) {
  if (HasHelp(argc, argv)) {
    std::fprintf(stderr,
                 "usage: clank --headless --shot-after N --dump-scene out.json "
                 "[--replay demo.clk --seed 42]\n");
    return 0;
  }
  auto parsed = clank::m0::ParseFlags(argc, argv);
  if (!parsed) {
    std::fprintf(stderr, "%s\n", parsed.error().c_str());
    return 2;
  }
  const clank::m0::Flags flags = *parsed;
  clank::m6::Playback playback;
  if (!flags.replay.empty()) {
    if (auto loaded = playback.Load(flags.replay); !loaded) {
      std::fprintf(stderr, "clank: %s\n", loaded.error().c_str());
      return 2;
    }
  }

  clank::m5::Scene scene = DemoScene(flags.seed);
  // WHY N-1: frames run 0..N-1, so the shot shows state after exactly N frames.
  const int frames = flags.shot_after > 0 ? flags.shot_after : 60;
  std::fprintf(stderr, "clank: %s frames=%d seed=%d\n", flags.headless ? "headless" : "windowed",
               frames, flags.seed);
  auto update = [&](clank::m1::Frame frame, double dt) {
    if (flags.replay.empty()) {
      scene.entities[0].x += static_cast<float>(dt);
      return;
    }
    const float x = static_cast<float>(playback.IsDown(frame.number, clank::m6::Key::Right) -
                                       playback.IsDown(frame.number, clank::m6::Key::Left));
    const float y = static_cast<float>(playback.IsDown(frame.number, clank::m6::Key::Down) -
                                       playback.IsDown(frame.number, clank::m6::Key::Up));
    scene.entities[0].x += x * static_cast<float>(dt);
    scene.entities[0].y += y * static_cast<float>(dt);
  };
  clank::m2::Renderer renderer = clank::m2::Create();

  if (flags.headless) {
    clank::m1::Run({1.0 / 60.0, frames}, update);
    if (flags.shot_after >= 0) {
      auto shot = clank::m2::TakeScreenshot(renderer, ShotPath(flags.shot_after));
      if (!shot) {
        std::fprintf(stderr, "clank: shot failed: %s\n", shot.error().c_str());
        return 1;
      }
      std::printf("%s\n", ShotPath(flags.shot_after).c_str());
    }
  } else {
    // WHY lockstep, not RunWindowed: wall-clock pacing ties sim frames to GPU
    // speed, so frame N would land on different pixels per machine. One update
    // + one draw per iteration makes --shot-after deterministic.
    auto win = clank::m1::OpenWindow({});
    if (!win) {
      std::fprintf(stderr, "clank: %s (hint: use --headless without a display)\n",
                   win.error().c_str());
      return 1;
    }
    int frame = -1;
    int draws = 0;
    auto counted = [&](clank::m1::Frame f, double dt) {
      frame = f.number;
      update(f, dt);
    };
    auto draw = [&]() {
      ++draws;
      clank::m1::BeginFrame(20, 20, 30, 255);
      clank::m2::DrawRect(renderer, scene.entities[0].x * 60.0f, 300.0f, 40.0f, 40.0f,
                          {255, 255, 255, 255});
      if (flags.shot_after > 0 && frame == flags.shot_after - 1) {
        // WHY loud failure: a silent no-shot wastes a whole CI cycle to diagnose.
        auto shot = clank::m2::TakeScreenshot(renderer, ShotPath(flags.shot_after));
        if (shot) {
          std::printf("%s\n", ShotPath(flags.shot_after).c_str());
        } else {
          std::fprintf(stderr, "clank: shot frame=%d failed: %s\n", frame, shot.error().c_str());
        }
      }
      clank::m1::EndFrame();
    };
    clank::m1::Stepper stepper(1.0 / 60.0);
    for (int i = 0; i < frames && !clank::m1::ShouldClose(); ++i) {
      stepper.StepOnce(counted);
      draw();
    }
    std::fprintf(stderr, "clank: windowed done last_frame=%d draws=%d\n", frame, draws);
    clank::m1::CloseWindow();
  }

  clank::m2::Destroy(renderer);
  if (!flags.dump_scene.empty()) {
    if (!WriteText(flags.dump_scene, clank::m5::DumpJson(scene))) {
      std::fprintf(stderr, "clank: cannot write %s\n", flags.dump_scene.c_str());
      return 1;
    }
    std::printf("%s\n", flags.dump_scene.c_str());
  }
  return 0;
}
