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

bool WindowOpen() { return ::IsWindowReady(); }

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

std::expected<void, std::string> RunWindowed(const LoopConfig& loop, const WindowConfig& win,
                                             const UpdateFn& update, const DrawFn& draw) {
  // WHY: non-positive dt would divide by zero in the accumulator, so fail before opening.
  if (loop.dt <= 0.0) return std::unexpected(std::string("m1: bad dt"));
  // WHY: fail fast when already open so ownership stays with the caller.
  if (::IsWindowReady()) return std::unexpected(std::string("m1: window already open"));
  std::expected<void, std::string> opened = OpenWindow(win);
  if (!opened) return opened;
  // WHY: leave pacing to raylib default vsync instead of SetTargetFPS so timing stays in one place.
  Stepper stepper(loop.dt);
  double acc = 0.0;
  int rendered = 0;
  while (rendered < loop.max_frames && !ShouldClose()) {
    acc += static_cast<double>(::GetFrameTime());
    // WHY: clamp accumulator to 5 steps so a stalled frame cannot spiral into catch-up debt.
    const double cap = 5.0 * loop.dt;
    if (acc > cap) acc = cap;
    int budget = static_cast<int>(acc / loop.dt);
    if (budget > 5) budget = 5;
    if (budget > 0) {
      stepper.Advance(budget, update);
      acc -= static_cast<double>(budget) * loop.dt;
    }
    // WHY: draw owns the BeginFrame/EndFrame pair so callers pick the clear color.
    if (draw) draw();
    ++rendered;
  }
  CloseWindow();
  return {};
}

}  // namespace clank::m1
