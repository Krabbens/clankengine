#include "m1/loop.hpp"

#include <raylib.h>

#include <string>

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

std::expected<void, std::string> OpenWindow(const WindowConfig& cfg) {
  // WHY: single window keeps lifecycle explicit for agents driving headless vs windowed.
  if (::IsWindowReady()) return std::unexpected(std::string("m1: window already open"));
  // WHY: config flags must precede InitWindow to take effect.
  if (cfg.resizable) ::SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  ::InitWindow(cfg.width, cfg.height, cfg.title.c_str());
  // WHY: InitWindow has no error return so readiness probe is the failure signal.
  if (!::IsWindowReady()) return std::unexpected(std::string("m1: InitWindow failed"));
  return {};
}

void CloseWindow() {
  // WHY: headless selftest must stay safe with no window, so guard close.
  if (::IsWindowReady()) ::CloseWindow();
}

InputSnapshot PollInput() {
  InputSnapshot snapshot;
  if (!::IsWindowReady()) return snapshot;
  for (int key = 0; key < kKeyCount; ++key) snapshot.down[key] = ::IsKeyDown(key);
  return snapshot;
}

bool ShouldClose() {
  // WHY: WindowShouldClose without a window must read as false so headless stays idle.
  if (!::IsWindowReady()) return false;
  return ::WindowShouldClose();
}

void BeginFrame(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
  // WHY: headless Begin stays a no-op so agents share draw code with and without display.
  if (!::IsWindowReady()) return;
  ::BeginDrawing();
  ::ClearBackground(::Color{r, g, b, a});
}

void EndFrame() {
  // WHY: buffer swap without a window is undefined, so guard keeps headless safe.
  if (::IsWindowReady()) ::EndDrawing();
}

}  // namespace clank::m1
