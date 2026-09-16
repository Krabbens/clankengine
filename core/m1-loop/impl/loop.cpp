#include "m1/loop.hpp"

namespace clank::m1 {

Stepper::Stepper(double dt) : dt_(dt) {}

int Stepper::Advance(int frame_budget, const UpdateFn& update) {
  if (paused_ || frame_budget <= 0 || !update) return 0;
  for (int i = 0; i < frame_budget; ++i) {
    update(Frame{next_}, dt_);
    ++next_;
  }
  return frame_budget;
}

void Stepper::Pause() { paused_ = true; }

void Stepper::Resume() { paused_ = false; }

int Stepper::StepOnce(const UpdateFn& update) {
  // WHY: replay/debug must advance one frame while paused, so ignore paused_.
  if (!update) return 0;
  update(Frame{next_}, dt_);
  ++next_;
  return 1;
}

bool Stepper::paused() const { return paused_; }

int Stepper::next() const { return next_; }

void Run(const LoopConfig& cfg, const UpdateFn& update) {
  Stepper stepper(cfg.dt);
  stepper.Advance(cfg.max_frames, update);
}

}  // namespace clank::m1
