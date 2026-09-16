#include "m2/render.hpp"

#include <raylib.h>
#include <rlgl.h>

#include <filesystem>
#include <fstream>
#include <system_error>
#include <unordered_map>
#include <vector>

namespace clank::m2 {
namespace {

// STUB: minimal 1x1 PNG until raylib backend lands (WHY: agents need pixels with no window).
constexpr unsigned char kDotPng[] = {
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48,
    0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x06, 0x00, 0x00,
    0x00, 0x1F, 0x15, 0xC4, 0x89, 0x00, 0x00, 0x00, 0x0A, 0x49, 0x44, 0x41, 0x54, 0x78,
    0x9C, 0x63, 0x00, 0x01, 0x00, 0x00, 0x05, 0x00, 0x01, 0x0D, 0x0A, 0x2D, 0xB4, 0x00,
    0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82};

std::unordered_map<int, std::vector<DrawEntry>>& Logs() {
  static std::unordered_map<int, std::vector<DrawEntry>> logs;
  return logs;
}

int& NextId() {
  static int next = 0;
  return next;
}

void Push(const Renderer& r, DrawEntry e) {
  auto it = Logs().find(r.id);
  if (r.valid && it != Logs().end()) it->second.push_back(std::move(e));
}

// WHY: public header stays stdlib-only so translate to raylib color at the boundary.
::Color ToRay(Color c) { return ::Color{c.r, c.g, c.b, c.a}; }

}  // namespace

Renderer Create() {
  int id = NextId()++;
  Logs()[id] = {};
  return Renderer{.id = id, .valid = true};
}

void Destroy(Renderer& r) {
  Logs().erase(r.id);
  r.id = -1;
  r.valid = false;
}

void Begin(Renderer& r) {
  (void)r;
  // WHY: headless runs have no window; only mirror to GPU once m1 owns one.
  // Precondition: ::IsWindowReady() must be true before ::BeginDrawing().
  if (::IsWindowReady()) ::BeginDrawing();
}

void End(Renderer& r) {
  (void)r;
  // WHY: buffer swap without a window is undefined; guard keeps headless safe.
  // Precondition: ::IsWindowReady() must be true before ::EndDrawing().
  if (::IsWindowReady()) ::EndDrawing();
}

void Clear(Renderer& r, Color c) {
  (void)r;
  // WHY: headless Clear stays a no-op for the log; mirror clear only with a window.
  // Precondition: ::IsWindowReady() must be true before ::ClearBackground().
  if (::IsWindowReady()) ::ClearBackground(ToRay(c));
}

void DrawRect(Renderer& r, float x, float y, float w, float h, Color c) {
  Push(r,
       DrawEntry{.kind = DrawKind::Rect, .x = x, .y = y, .w = w, .h = h, .text = {}, .color = c});
  // WHY: DrawLog is headless truth; mirror to screen only when a window exists.
  // Precondition: ::IsWindowReady() must be true before ::DrawRectangleV().
  if (::IsWindowReady()) ::DrawRectangleV(::Vector2{x, y}, ::Vector2{w, h}, ToRay(c));
}

void DrawCircle(Renderer& r, float x, float y, float radius, Color c) {
  Push(r, DrawEntry{.kind = DrawKind::Circle, .x = x, .y = y, .w = radius, .text = {}, .color = c});
  // WHY: keep float center in V variant; skip GPU work headless.
  // Precondition: ::IsWindowReady() must be true before ::DrawCircleV().
  if (::IsWindowReady()) ::DrawCircleV(::Vector2{x, y}, radius, ToRay(c));
}

void DrawText(Renderer& r, const std::string& text, float x, float y, float size, Color c) {
  Push(r, DrawEntry{.kind = DrawKind::Text, .x = x, .y = y, .w = size, .text = text, .color = c});
  // WHY: default-font text needs a GL context; headless keeps log only.
  // Precondition: ::IsWindowReady() must be true before ::DrawText().
  if (::IsWindowReady())
    ::DrawText(text.c_str(), static_cast<int>(x), static_cast<int>(y), static_cast<int>(size),
               ToRay(c));
}

std::size_t DrawLogCount(const Renderer& r) {
  auto it = Logs().find(r.id);
  if (!r.valid || it == Logs().end()) return 0;
  return it->second.size();
}

const DrawEntry* DrawLogAt(const Renderer& r, std::size_t i) {
  auto it = Logs().find(r.id);
  if (!r.valid || it == Logs().end() || i >= it->second.size()) return nullptr;
  return &it->second[i];
}

std::expected<void, std::string> TakeScreenshot(const Renderer& r, const std::string& path) {
  (void)r;
  // WHY: real pixels need a window; headless agents still need see-channel bytes.
  // Precondition: ::IsWindowReady() selects real ::TakeScreenshot vs stub PNG.
  if (::IsWindowReady()) {
    // WHY flush first: raylib batches shapes on CPU and submits at EndDrawing;
    // reading pixels before the flush would capture only the cleared background.
    ::rlDrawRenderBatchActive();
    // WHY two-path check: raylib 5.5 TakeScreenshot drops directories and saves
    // basename(file) to cwd (observed in CI). Honor the contract either way.
    // WHY pre-remove: a stale file at path (e.g. headless stub from an earlier
    // run) would fake success; both candidates are ours to overwrite.
    namespace fs = std::filesystem;
    const std::string base = path.substr(path.find_last_of("/\\") + 1);
    std::error_code ec;
    fs::remove(path, ec);
    if (base != path) fs::remove(base, ec);
    ec.clear();
    ::TakeScreenshot(path.c_str());
    if (fs::exists(path, ec) && !ec) return {};
    if (base != path && fs::exists(base, ec) && !ec) {
      fs::copy_file(base, path, fs::copy_options::overwrite_existing, ec);
      std::error_code rm_ec;
      fs::remove(base, rm_ec);
      if (!ec) return {};
      return std::unexpected("m2: cannot move screenshot: " + ec.message());
    }
    return std::unexpected("m2: screenshot missing at " + path);
  }
  std::ofstream out(path, std::ios::binary);
  if (!out) return std::unexpected("m2: cannot open " + path);
  out.write(reinterpret_cast<const char*>(kDotPng), sizeof(kDotPng));
  if (!out) return std::unexpected("m2: cannot write " + path);
  return {};
}

}  // namespace clank::m2
