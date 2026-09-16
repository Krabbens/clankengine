#pragma once
#include <functional>

namespace clank::m1 {

struct LoopConfig {
  double dt = 1.0 / 60.0;
  int max_frames = 60;
};

struct Frame {
  int number = 0;
};

using UpdateFn = std::function<void(Frame, double)>;

class Stepper {
 public:
  explicit Stepper(double dt);
  int Advance(int frame_budget, const UpdateFn& update);
  void Pause();
  void Resume();
  int StepOnce(const UpdateFn& update);
  [[nodiscard]] bool paused() const;
  [[nodiscard]] int next() const;

 private:
  double dt_;
  int next_ = 0;
  bool paused_ = false;
};

void Run(const LoopConfig& cfg, const UpdateFn& update);

// TODO(wave2): attach raylib window/event pump inside Run; headless stays default for agents.

}  // namespace clank::m1
