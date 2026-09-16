#pragma once
#include <expected>
#include <functional>
#include <string>

namespace clank::m1 {

struct LoopConfig {
  double dt = 1.0 / 60.0;
  int max_frames = 60;
};

struct Frame {
  int number = 0;
};

using UpdateFn = std::function<void(Frame, double)>;

struct WindowConfig {
  int width = 1280;
  int height = 720;
  std::string title = "clank";
  bool resizable = false;
};

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

// WHY: header stays stdlib-only; raylib is included only in impl so headless builds need no
// display.
std::expected<void, std::string> OpenWindow(const WindowConfig& cfg);
void CloseWindow();
[[nodiscard]] bool WindowOpen();
[[nodiscard]] bool ShouldClose();
// WHY: m2 draw calls belong between BeginFrame/EndFrame so one frame batches GPU work.
void BeginFrame(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
void EndFrame();

}  // namespace clank::m1
