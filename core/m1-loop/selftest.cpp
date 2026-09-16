#include "m1/loop.hpp"

#include <cstdio>

int main() {
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
