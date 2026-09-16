#include "m2/render.hpp"

#include <fstream>
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

void Begin(Renderer& r) { (void)r; }

void End(Renderer& r) { (void)r; }

void Clear(Renderer& r, Color c) {
  (void)r;
  (void)c;
}

void DrawRect(Renderer& r, float x, float y, float w, float h, Color c) {
  Push(r, DrawEntry{.kind = DrawKind::Rect, .x = x, .y = y, .w = w, .h = h, .color = c});
}

void DrawCircle(Renderer& r, float x, float y, float radius, Color c) {
  Push(r, DrawEntry{.kind = DrawKind::Circle, .x = x, .y = y, .w = radius, .color = c});
}

void DrawText(Renderer& r, const std::string& text, float x, float y, float size, Color c) {
  Push(r, DrawEntry{.kind = DrawKind::Text, .x = x, .y = y, .w = size, .text = text, .color = c});
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
  std::ofstream out(path, std::ios::binary);
  if (!out) return std::unexpected("m2: cannot open " + path);
  out.write(reinterpret_cast<const char*>(kDotPng), sizeof(kDotPng));
  if (!out) return std::unexpected("m2: cannot write " + path);
  return {};
}

}  // namespace clank::m2
