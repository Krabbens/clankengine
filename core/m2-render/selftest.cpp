#include "m2/render.hpp"

#include <cstdio>
#include <fstream>

int main() {
  clank::m2::Renderer r = clank::m2::Create();
  if (!r.valid) return 1;
  clank::m2::Begin(r);
  clank::m2::Color white{.r = 255, .g = 255, .b = 255, .a = 255};
  clank::m2::DrawRect(r, 1, 2, 3, 4, white);
  clank::m2::DrawCircle(r, 5, 6, 7, white);
  clank::m2::DrawText(r, "hi", 8, 9, 10, white);
  clank::m2::End(r);
  if (clank::m2::DrawLogCount(r) != 3) return 1;
  const auto* e0 = clank::m2::DrawLogAt(r, 0);
  const auto* e1 = clank::m2::DrawLogAt(r, 1);
  const auto* e2 = clank::m2::DrawLogAt(r, 2);
  if (!e0 || !e1 || !e2) return 1;
  if (e0->kind != clank::m2::DrawKind::Rect) return 1;
  if (e1->kind != clank::m2::DrawKind::Circle) return 1;
  if (e2->kind != clank::m2::DrawKind::Text || e2->text != "hi") return 1;
  const std::string path = "/tmp/m2-selftest-shot.png";
  if (!clank::m2::TakeScreenshot(r, path)) return 1;
  std::ifstream in(path, std::ios::binary);
  if (!in) return 1;
  std::remove(path.c_str());
  clank::m2::Destroy(r);
  return r.valid ? 1 : 0;
}
