#include <cstdio>
#include <fstream>

#include "m2/render.hpp"

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
  // Clear starts a new frame in the log.
  clank::m2::Clear(r, white);
  if (clank::m2::DrawLogCount(r) != 0) return 1;
  clank::m2::DrawRect(r, 0, 0, 1, 1, white);
  if (clank::m2::DrawLogCount(r) != 1) return 1;
  // Identical stubs compare equal; bad inputs are errors, not false.
  const std::string shot_b = "/tmp/m2-selftest-shot-b.png";
  if (!clank::m2::TakeScreenshot(r, shot_b)) return 1;
  auto same = clank::m2::CompareImages(path, shot_b, 0.0);
  if (!same || !*same) return 1;
  if (clank::m2::CompareImages(path, "/tmp/m2-selftest-nope.png", 1.0)) return 1;
  const std::string not_png = "/tmp/m2-selftest-notpng.txt";
  {
    std::ofstream o(not_png);
    o << "nope";
  }
  if (clank::m2::CompareImages(path, not_png, 1.0)) return 1;
  if (clank::m2::CompareImages(path, shot_b, -0.5)) return 1;
  std::remove(path.c_str());
  std::remove(shot_b.c_str());
  std::remove(not_png.c_str());
  clank::m2::Destroy(r);
  return r.valid ? 1 : 0;
}
