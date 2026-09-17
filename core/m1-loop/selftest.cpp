#include <cstdio>

#include "m1/loop.hpp"

int main() {
  // WHY: windowed code is verified by compile+link only; CI has no display so never open here.
  clank::m1::CloseWindow();
  const auto first_input = clank::m1::PollInput();
  const auto second_input = clank::m1::PollInput();
  if (first_input.down != second_input.down) {
    std::fprintf(stderr, "m1: PollInput changed between headless polls\n");
    return 1;
  }
  for (int key = 0; key < clank::m1::kKeyCount; ++key) {
    if (first_input.down[key] || first_input.IsDown(key)) {
      std::fprintf(stderr, "m1: PollInput headless key %d was down\n", key);
      return 1;
    }
  }
  if (first_input.IsDown(-1) || first_input.IsDown(clank::m1::kKeyCount)) return 1;
  clank::m1::CloseWindow();
  if (clank::m1::PollInput().down != first_input.down) return 1;
  if (clank::m1::ShouldClose()) {
    std::fprintf(stderr, "m1: ShouldClose want false headless\n");
    return 1;
  }
  clank::m1::LoopConfig cfg{.dt = 1.0 / 60.0, .max_frames = 60};
  int count = 0;
  int last = -1;
  clank::m1::Run(cfg, [&](clank::m1::Frame f, double) {
    last = f.number;
    ++count;
  });
  if (count != 60 || last != 59) {
    std::fprintf(stderr, "m1: Run got count=%d last=%d want 60/59\n", count, last);
    return 1;
  }
  clank::m1::Stepper stepper(1.0 / 60.0);
  stepper.Pause();
  if (stepper.Advance(4, [&](clank::m1::Frame, double) {}) != 0) return 1;
  int before = stepper.next();
  if (stepper.StepOnce([&](clank::m1::Frame, double) {}) != 1) return 1;
  if (stepper.next() != before + 1) return 1;
  stepper.Resume();
  if (stepper.Advance(3, [&](clank::m1::Frame, double) {}) != 3) return 1;
  return 0;
}
