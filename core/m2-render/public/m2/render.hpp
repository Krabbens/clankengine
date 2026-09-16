#pragma once
#include <cstddef>
#include <expected>
#include <string>
#include <vector>

namespace clank::m2 {

struct Color {
  unsigned char r = 0;
  unsigned char g = 0;
  unsigned char b = 0;
  unsigned char a = 255;
};

enum class DrawKind { Rect, Circle, Triangle, Text };

struct DrawEntry {
  DrawKind kind = DrawKind::Rect;
  float x = 0;
  float y = 0;
  float w = 0;
  float h = 0;
  // WHY second/third vertex live here: triangles reuse one entry instead of three rect-shaped rows.
  float x2 = 0, y2 = 0, x3 = 0, y3 = 0;
  std::string text;
  Color color{};
};

struct Renderer {
  int id = -1;
  bool valid = false;
};

Renderer Create();
void Destroy(Renderer& r);
void Clear(Renderer& r, Color c);
void DrawRect(Renderer& r, float x, float y, float w, float h, Color c);
void DrawCircle(Renderer& r, float x, float y, float radius, Color c);
void DrawTriangle(Renderer& r, float x1, float y1, float x2, float y2, float x3, float y3, Color c);
void DrawText(Renderer& r, const std::string& text, float x, float y, float size, Color c);

// WHY perspective-only: both 3D samples use a fixed perspective camera; ortho can extend this.
struct Vec3 {
  float x = 0;
  float y = 0;
  float z = 0;
};

struct Camera {
  Vec3 position{};
  Vec3 target{};
  Vec3 up{0, 1, 0};
  float fov = 45;
};

enum class Draw3DKind { Cube, CubeWires, Sphere, SphereWires, Cylinder, CylinderWires };

// WHY one entry: a/b/c read as sizes (cube), radius (sphere) or rTop/rBottom/height (cylinder);
// n/m read as rings/slices. Agents get the full 3D call without a log type per shape.
struct Draw3DEntry {
  Draw3DKind kind = Draw3DKind::Cube;
  float x = 0, y = 0, z = 0;
  float a = 0, b = 0, c = 0;
  int n = 0, m = 0;
  Color color{};
};

void BeginMode3D(Renderer& r, Camera camera);
void EndMode3D(Renderer& r);
void DrawCube(Renderer& r, float x, float y, float z, float sx, float sy, float sz, Color c);
void DrawCubeWires(Renderer& r, float x, float y, float z, float sx, float sy, float sz, Color c);
void DrawSphere(Renderer& r, float x, float y, float z, float radius, Color c);
void DrawSphereWires(Renderer& r, float x, float y, float z, float radius, int rings, int slices,
                     Color c);
void DrawCylinder(Renderer& r, float x, float y, float z, float r_top, float r_bottom, float height,
                  int slices, Color c);
void DrawCylinderWires(Renderer& r, float x, float y, float z, float r_top, float r_bottom,
                       float height, int slices, Color c);
[[nodiscard]] std::size_t Draw3DLogCount(const Renderer& r);
[[nodiscard]] const Draw3DEntry* Draw3DLogAt(const Renderer& r, std::size_t i);
[[nodiscard]] std::size_t DrawLogCount(const Renderer& r);
[[nodiscard]] const DrawEntry* DrawLogAt(const Renderer& r, std::size_t i);
// Backend calls raylib only when IsWindowReady(); headless rasterizes the 2D DrawLog into a
// deterministic 1280x720 PNG (WHY: agents need useful see-channel pixels without a display).
std::expected<void, std::string> TakeScreenshot(const Renderer& r, const std::string& path);
// Fuzz compare of two PNGs: true when the fraction of pixels differing by more
// than 5 per channel is within max_diff_frac. Same aspect required; the larger
// is downscaled nearest-neighbor (WHY: Retina vs logical captures, GPU dither).
[[nodiscard]] std::expected<bool, std::string> CompareImages(const std::string& path_a,
                                                             const std::string& path_b,
                                                             double max_diff_frac);

// WHY integer synth: libm varies across platforms; square/saw buffers stay bit-exact everywhere.
// WHY silent-sink: CI has no audio device; calls never fail, they just make no sound headless.
struct Audio {
  int id = -1;
  bool valid = false;
};

struct Sfx {
  int id = -1;
  std::vector<short> frames{};
  int rate = 22050;
};

Audio OpenAudio();
void CloseAudio(Audio& audio);
bool AudioReady(const Audio& audio);
// wave: 0 square, 1 saw. Always synthesizes into frames; backend Sound only when a device is ready.
Sfx LoadTone(Audio& audio, int freq_hz, int millis, int wave);
void PlaySfx(Audio& audio, const Sfx& sfx);
void UnloadSfx(Audio& audio, const Sfx& sfx);

}  // namespace clank::m2
