#pragma once
#include <cstddef>
#include <expected>
#include <string>

namespace clank::m2 {

struct Color {
  unsigned char r = 0;
  unsigned char g = 0;
  unsigned char b = 0;
  unsigned char a = 255;
};

enum class DrawKind { Rect, Circle, Text };

struct DrawEntry {
  DrawKind kind = DrawKind::Rect;
  float x = 0;
  float y = 0;
  float w = 0;
  float h = 0;
  std::string text;
  Color color{};
};

struct Renderer {
  int id = -1;
  bool valid = false;
};

Renderer Create();
void Destroy(Renderer& r);
void Begin(Renderer& r);
void End(Renderer& r);
void Clear(Renderer& r, Color c);
void DrawRect(Renderer& r, float x, float y, float w, float h, Color c);
void DrawCircle(Renderer& r, float x, float y, float radius, Color c);
void DrawText(Renderer& r, const std::string& text, float x, float y, float size, Color c);
[[nodiscard]] std::size_t DrawLogCount(const Renderer& r);
[[nodiscard]] const DrawEntry* DrawLogAt(const Renderer& r, std::size_t i);
// STUB: always writes 1x1 PNG until raylib backend lands (WHY: agents need see-channel headless).
std::expected<void, std::string> TakeScreenshot(const Renderer& r, const std::string& path);

}  // namespace clank::m2
