#include <raylib.h>

#include <cstdio>
#include <fstream>

#include "m2/render.hpp"

int main() {
  clank::m2::Renderer r = clank::m2::Create();
  if (!r.valid) return 1;
  clank::m2::Color white{.r = 255, .g = 255, .b = 255, .a = 255};
  clank::m2::DrawRect(r, 1, 2, 3, 4, white);
  clank::m2::DrawCircle(r, 5, 6, 7, white);
  clank::m2::DrawText(r, "hi", 8, 9, 10, white);
  if (clank::m2::DrawLogCount(r) != 3) return 1;
  const auto* e0 = clank::m2::DrawLogAt(r, 0);
  const auto* e1 = clank::m2::DrawLogAt(r, 1);
  const auto* e2 = clank::m2::DrawLogAt(r, 2);
  if (!e0 || !e1 || !e2) return 1;
  if (e0->kind != clank::m2::DrawKind::Rect) return 1;
  if (e1->kind != clank::m2::DrawKind::Circle) return 1;
  if (e2->kind != clank::m2::DrawKind::Text || e2->text != "hi") return 1;
  clank::m2::DrawTriangle(r, 1, 2, 3, 4, 5, 6, white);
  if (clank::m2::DrawLogCount(r) != 4) return 1;
  const auto* e3 = clank::m2::DrawLogAt(r, 3);
  if (!e3) return 1;
  if (e3->kind != clank::m2::DrawKind::Triangle || e3->x != 1 || e3->y != 2 || e3->x2 != 3 ||
      e3->y2 != 4 || e3->x3 != 5 || e3->y3 != 6)
    return 1;
  const std::string path = "/tmp/m2-selftest-shot.png";
  if (!clank::m2::TakeScreenshot(r, path)) return 1;
  ::Image shot = ::LoadImage(path.c_str());
  if (!shot.data || shot.width != 1280 || shot.height != 720) return 1;
  ::UnloadImage(shot);
  std::ifstream in(path, std::ios::binary);
  if (!in) return 1;
  // Clear starts a new frame in the log.
  clank::m2::Clear(r, white);
  if (clank::m2::DrawLogCount(r) != 0 || clank::m2::Draw3DLogCount(r) != 0) return 1;
  clank::m2::Camera cam{{0, 14, 10}, {0, 0, 0}, {0, 1, 0}, 45};
  clank::m2::Color blue{.r = 50, .g = 100, .b = 150, .a = 255};
  clank::m2::BeginMode3D(r, cam);
  clank::m2::DrawCube(r, 1, 2, 3, 4, 5, 6, blue);
  clank::m2::DrawCubeWires(r, 1, 2, 3, 4, 5, 6, blue);
  clank::m2::DrawSphere(r, 1, 2, 3, 4, blue);
  clank::m2::DrawSphereWires(r, 1, 2, 3, 4, 6, 8, blue);
  clank::m2::DrawCylinder(r, 1, 2, 3, 4, 5, 6, 8, blue);
  clank::m2::DrawCylinderWires(r, 1, 2, 3, 4, 5, 6, 8, blue);
  clank::m2::EndMode3D(r);
  if (clank::m2::Draw3DLogCount(r) != 6) return 1;
  const auto* c0 = clank::m2::Draw3DLogAt(r, 0);
  const auto* c4 = clank::m2::Draw3DLogAt(r, 4);
  const auto* c5 = clank::m2::Draw3DLogAt(r, 5);
  if (!c0 || !c4 || !c5) return 1;
  if (c0->kind != clank::m2::Draw3DKind::Cube || c0->a != 4 || c0->c != 6) return 1;
  if (c4->kind != clank::m2::Draw3DKind::Cylinder || c4->a != 4 || c4->b != 5) return 1;
  if (c5->kind != clank::m2::Draw3DKind::CylinderWires || c5->n != 8) return 1;
  if (clank::m2::Draw3DLogAt(r, 6) != nullptr) return 1;
  const std::string shot_3d = "/tmp/m2-selftest-shot-3d.png";
  if (!clank::m2::TakeScreenshot(r, shot_3d)) return 1;
  clank::m2::Clear(r, white);
  const std::string shot_blank = "/tmp/m2-selftest-shot-blank.png";
  if (!clank::m2::TakeScreenshot(r, shot_blank)) return 1;
  auto rendered_3d = clank::m2::CompareImages(shot_3d, shot_blank, 0.0);
  if (!rendered_3d || *rendered_3d) return 1;
  if (clank::m2::DrawLogCount(r) != 0 || clank::m2::Draw3DLogCount(r) != 0) return 1;
  clank::m2::DrawRect(r, 0, 0, 1, 1, white);
  if (clank::m2::DrawLogCount(r) != 1) return 1;
  // Rasterized headless frames compare by pixels; bad inputs are errors, not false.
  const std::string shot_b = "/tmp/m2-selftest-shot-b.png";
  if (!clank::m2::TakeScreenshot(r, shot_b)) return 1;
  auto different = clank::m2::CompareImages(path, shot_b, 0.0);
  if (!different || *different) return 1;
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
  std::remove(shot_3d.c_str());
  std::remove(shot_blank.c_str());
  std::remove(not_png.c_str());
  // WHY no device asserts: CI has no audio hardware, so only the silent path is provable here.
  clank::m2::Audio audio = clank::m2::OpenAudio();
  if (!audio.valid) return 1;
  clank::m2::Sfx bad = clank::m2::LoadTone(audio, 440, 0, 0);
  if (bad.id >= 0 || !bad.frames.empty()) return 1;
  clank::m2::Sfx tone = clank::m2::LoadTone(audio, 440, 100, 0);
  if (tone.id < 0 || tone.rate != 22050 || tone.frames.size() != 2205) return 1;
  // WHY exact samples: integer synth is bit-exact on every platform, so assert bytes.
  const size_t period = static_cast<size_t>(22050 / 440);
  if (tone.frames[0] != 32767 || tone.frames[period / 2] != -32767) return 1;
  clank::m2::Sfx saw = clank::m2::LoadTone(audio, 440, 100, 1);
  if (saw.frames[1] <= saw.frames[0]) return 1;
  clank::m2::PlaySfx(audio, tone);
  clank::m2::PlaySfx(audio, bad);
  clank::m2::UnloadSfx(audio, tone);
  clank::m2::UnloadSfx(audio, bad);
  clank::m2::PlaySfx(audio, tone);
  clank::m2::CloseAudio(audio);
  if (audio.valid) return 1;
  clank::m2::Destroy(r);
  return r.valid ? 1 : 0;
}
