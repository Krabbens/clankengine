#include "m2/render.hpp"

#include <raylib.h>
#include <rlgl.h>

#include <cmath>
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

std::unordered_map<int, std::vector<Draw3DEntry>>& Logs3D() {
  static std::unordered_map<int, std::vector<Draw3DEntry>> logs;
  return logs;
}

void Push3D(const Renderer& r, Draw3DEntry e) {
  auto it = Logs3D().find(r.id);
  if (r.valid && it != Logs3D().end()) it->second.push_back(std::move(e));
}

// WHY: public header stays stdlib-only so translate to raylib color at the boundary.
::Color ToRay(Color c) { return ::Color{c.r, c.g, c.b, c.a}; }

}  // namespace

Renderer Create() {
  int id = NextId()++;
  Logs()[id] = {};
  Logs3D()[id] = {};
  return Renderer{.id = id, .valid = true};
}

void Destroy(Renderer& r) {
  Logs().erase(r.id);
  Logs3D().erase(r.id);
  r.id = -1;
  r.valid = false;
}

void Clear(Renderer& r, Color c) {
  // WHY: DrawLog is frame truth, so Clear starts a new frame headless too.
  // Precondition: ::IsWindowReady() must be true before ::ClearBackground().
  auto it = Logs().find(r.id);
  if (r.valid && it != Logs().end()) it->second.clear();
  auto it3 = Logs3D().find(r.id);
  if (r.valid && it3 != Logs3D().end()) it3->second.clear();
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

void DrawTriangle(Renderer& r, float x1, float y1, float x2, float y2, float x3, float y3,
                  Color c) {
  Push(r, DrawEntry{.kind = DrawKind::Triangle,
                    .x = x1,
                    .y = y1,
                    .x2 = x2,
                    .y2 = y2,
                    .x3 = x3,
                    .y3 = y3,
                    .text = {},
                    .color = c});
  // WHY: DrawLog is headless truth; mirror to screen only when a window exists.
  // Precondition: ::IsWindowReady() must be true before ::DrawTriangle().
  if (::IsWindowReady())
    ::DrawTriangle(::Vector2{x1, y1}, ::Vector2{x2, y2}, ::Vector2{x3, y3}, ToRay(c));
}

void DrawText(Renderer& r, const std::string& text, float x, float y, float size, Color c) {
  Push(r, DrawEntry{.kind = DrawKind::Text, .x = x, .y = y, .w = size, .text = text, .color = c});
  // WHY: default-font text needs a GL context; headless keeps log only.
  // Precondition: ::IsWindowReady() must be true before ::DrawText().
  if (::IsWindowReady())
    ::DrawText(text.c_str(), static_cast<int>(x), static_cast<int>(y), static_cast<int>(size),
               ToRay(c));
}

// WHY: public header stays stdlib-only so translate the camera at the boundary, like colors.
::Camera3D ToRayCam(Camera camera) {
  return ::Camera3D{{camera.position.x, camera.position.y, camera.position.z},
                    {camera.target.x, camera.target.y, camera.target.z},
                    {camera.up.x, camera.up.y, camera.up.z},
                    camera.fov,
                    CAMERA_PERSPECTIVE};
}

void BeginMode3D(Renderer& r, Camera camera) {
  (void)r;
  // WHY: headless keeps the 3D log only; mirror to GPU once m1 owns a window.
  // Precondition: ::IsWindowReady() must be true before ::BeginMode3D().
  if (::IsWindowReady()) ::BeginMode3D(ToRayCam(camera));
}

void EndMode3D(Renderer& r) {
  (void)r;
  // WHY: 3D state without a window is undefined; guard keeps headless safe.
  // Precondition: ::IsWindowReady() must be true before ::EndMode3D().
  if (::IsWindowReady()) ::EndMode3D();
}

void PushCube(Renderer& r, Draw3DKind kind, float x, float y, float z, float sx, float sy, float sz,
              Color c) {
  Push3D(r,
         Draw3DEntry{.kind = kind, .x = x, .y = y, .z = z, .a = sx, .b = sy, .c = sz, .color = c});
}

void DrawCube(Renderer& r, float x, float y, float z, float sx, float sy, float sz, Color c) {
  PushCube(r, Draw3DKind::Cube, x, y, z, sx, sy, sz, c);
  // WHY: Draw3DLog is headless truth; mirror to screen only when a window exists.
  // Precondition: ::IsWindowReady() must be true before ::DrawCube().
  if (::IsWindowReady()) ::DrawCube(::Vector3{x, y, z}, sx, sy, sz, ToRay(c));
}

void DrawCubeWires(Renderer& r, float x, float y, float z, float sx, float sy, float sz, Color c) {
  PushCube(r, Draw3DKind::CubeWires, x, y, z, sx, sy, sz, c);
  // Precondition: ::IsWindowReady() must be true before ::DrawCubeWires().
  if (::IsWindowReady()) ::DrawCubeWires(::Vector3{x, y, z}, sx, sy, sz, ToRay(c));
}

void DrawSphere(Renderer& r, float x, float y, float z, float radius, Color c) {
  Push3D(r,
         Draw3DEntry{.kind = Draw3DKind::Sphere, .x = x, .y = y, .z = z, .a = radius, .color = c});
  // Precondition: ::IsWindowReady() must be true before ::DrawSphere().
  if (::IsWindowReady()) ::DrawSphere(::Vector3{x, y, z}, radius, ToRay(c));
}

void DrawSphereWires(Renderer& r, float x, float y, float z, float radius, int rings, int slices,
                     Color c) {
  Push3D(r, Draw3DEntry{.kind = Draw3DKind::SphereWires,
                        .x = x,
                        .y = y,
                        .z = z,
                        .a = radius,
                        .n = rings,
                        .m = slices,
                        .color = c});
  // Precondition: ::IsWindowReady() must be true before ::DrawSphereWires().
  if (::IsWindowReady()) ::DrawSphereWires(::Vector3{x, y, z}, radius, rings, slices, ToRay(c));
}

void DrawCylinder(Renderer& r, float x, float y, float z, float r_top, float r_bottom, float height,
                  int slices, Color c) {
  Push3D(r, Draw3DEntry{.kind = Draw3DKind::Cylinder,
                        .x = x,
                        .y = y,
                        .z = z,
                        .a = r_top,
                        .b = r_bottom,
                        .c = height,
                        .n = slices,
                        .color = c});
  // Precondition: ::IsWindowReady() must be true before ::DrawCylinder().
  if (::IsWindowReady())
    ::DrawCylinder(::Vector3{x, y, z}, r_top, r_bottom, height, slices, ToRay(c));
}

void DrawCylinderWires(Renderer& r, float x, float y, float z, float r_top, float r_bottom,
                       float height, int slices, Color c) {
  Push3D(r, Draw3DEntry{.kind = Draw3DKind::CylinderWires,
                        .x = x,
                        .y = y,
                        .z = z,
                        .a = r_top,
                        .b = r_bottom,
                        .c = height,
                        .n = slices,
                        .color = c});
  // Precondition: ::IsWindowReady() must be true before ::DrawCylinderWires().
  if (::IsWindowReady())
    ::DrawCylinderWires(::Vector3{x, y, z}, r_top, r_bottom, height, slices, ToRay(c));
}

std::size_t Draw3DLogCount(const Renderer& r) {
  auto it = Logs3D().find(r.id);
  if (!r.valid || it == Logs3D().end()) return 0;
  return it->second.size();
}

const Draw3DEntry* Draw3DLogAt(const Renderer& r, std::size_t i) {
  auto it = Logs3D().find(r.id);
  if (!r.valid || it == Logs3D().end() || i >= it->second.size()) return nullptr;
  return &it->second[i];
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

std::expected<bool, std::string> CompareImages(const std::string& path_a, const std::string& path_b,
                                               double max_diff_frac) {
  // WHY NaN-safe range check: !(NaN in range) is true, so NaN is rejected.
  if (!(max_diff_frac >= 0.0 && max_diff_frac <= 1.0))
    return std::unexpected("m2: max_diff_frac out of [0,1]");
  namespace fs = std::filesystem;
  std::error_code ec;
  if (!fs::exists(path_a, ec) || ec) return std::unexpected("m2: missing " + path_a);
  if (!fs::exists(path_b, ec) || ec) return std::unexpected("m2: missing " + path_b);
  // WHY raylib decode, not stdlib: PNG+zlib by hand is ~200 lines; LoadImage is
  // CPU-side and needs no window, so headless agents can compare goldens.
  ::Image a = ::LoadImage(path_a.c_str());
  ::Image b = ::LoadImage(path_b.c_str());
  if (!a.data) {
    if (b.data) ::UnloadImage(b);
    return std::unexpected("m2: cannot decode " + path_a);
  }
  if (!b.data) {
    ::UnloadImage(a);
    return std::unexpected("m2: cannot decode " + path_b);
  }
  const long long aw = a.width, ah = a.height, bw = b.width, bh = b.height;
  if (aw <= 0 || ah <= 0 || bw <= 0 || bh <= 0 || aw * bh != bw * ah) {
    ::UnloadImage(a);
    ::UnloadImage(b);
    return std::unexpected("m2: size mismatch");
  }
  // WHY downscale the larger: shrinking loses the least; nearest keeps the flat
  // palette while filtering would invent edge colors.
  if (aw != bw || ah != bh) {
    if (aw > bw)
      ::ImageResizeNN(&a, b.width, b.height);
    else
      ::ImageResizeNN(&b, a.width, a.height);
  }
  ::Color* pa = ::LoadImageColors(a);
  ::Color* pb = ::LoadImageColors(b);
  if (!pa || !pb) {
    if (pa) ::UnloadImageColors(pa);
    if (pb) ::UnloadImageColors(pb);
    ::UnloadImage(a);
    ::UnloadImage(b);
    return std::unexpected("m2: cannot read pixels");
  }
  const long long pixels = static_cast<long long>(a.width) * a.height;
  long long changed = 0;
  for (long long i = 0; i < pixels; ++i) {
    if (std::abs(pa[i].r - pb[i].r) > 5 || std::abs(pa[i].g - pb[i].g) > 5 ||
        std::abs(pa[i].b - pb[i].b) > 5)
      ++changed;
  }
  const double frac = static_cast<double>(changed) / static_cast<double>(pixels);
  ::UnloadImageColors(pa);
  ::UnloadImageColors(pb);
  ::UnloadImage(a);
  ::UnloadImage(b);
  return frac <= max_diff_frac;
}

namespace {
// WHY one backend per Audio: miniaudio device init is process-global, sounds are per-device.
struct AudioBackend {
  bool device_ok = false;
  int next_sfx = 0;
  std::unordered_map<int, ::Sound> sounds{};
};
std::unordered_map<int, AudioBackend>& AudioBackends() {
  static std::unordered_map<int, AudioBackend> backends;
  return backends;
}
int& NextAudioId() {
  static int next = 0;
  return next;
}
int& AudioOpenCount() {
  static int count = 0;
  return count;
}
bool& AudioDeviceStarted() {
  static bool started = false;
  return started;
}
short SynthSample(int wave, int phase, int period) {
  if (period <= 1) return 32767;
  if (wave == 1) return static_cast<short>(phase * 65534 / period - 32767);
  return phase * 2 < period ? 32767 : -32767;
}
}  // namespace

Audio OpenAudio() {
  Audio audio{.id = NextAudioId(), .valid = true};
  // WHY refcount: double InitAudioDevice corrupts miniaudio state; last close shuts it down.
  if (AudioOpenCount() == 0) ::InitAudioDevice();
  ++AudioOpenCount();
  const bool ready = ::IsAudioDeviceReady();
  if (ready) AudioDeviceStarted() = true;
  AudioBackends()[audio.id] = AudioBackend{.device_ok = ready};
  return audio;
}

void CloseAudio(Audio& audio) {
  if (!audio.valid) return;
  AudioBackends().erase(audio.id);
  if (--AudioOpenCount() == 0 && AudioDeviceStarted()) ::CloseAudioDevice();
  audio.id = -1;
  audio.valid = false;
}

bool AudioReady(const Audio& audio) {
  if (!audio.valid) return false;
  auto it = AudioBackends().find(audio.id);
  return it != AudioBackends().end() && it->second.device_ok && ::IsAudioDeviceReady();
}

Sfx LoadTone(Audio& audio, int freq_hz, int millis, int wave) {
  Sfx sfx;
  if (!audio.valid || freq_hz <= 0 || millis <= 0) return sfx;
  const int count = sfx.rate * millis / 1000;
  if (count <= 0) return sfx;
  const int raw_period = sfx.rate / freq_hz;
  const int period = raw_period > 0 ? raw_period : 1;
  sfx.frames.reserve(static_cast<size_t>(count));
  for (int i = 0; i < count; ++i) sfx.frames.push_back(SynthSample(wave, i % period, period));
  auto it = AudioBackends().find(audio.id);
  if (it == AudioBackends().end()) return sfx;
  // WHY id before device check: the handle stays valid headless; only playback goes silent.
  sfx.id = it->second.next_sfx++;
  if (!it->second.device_ok) return sfx;
  ::Wave wave_data{static_cast<unsigned>(count), static_cast<unsigned>(sfx.rate), 16, 1,
                   sfx.frames.data()};
  it->second.sounds[sfx.id] = ::LoadSoundFromWave(wave_data);
  return sfx;
}

void PlaySfx(Audio& audio, const Sfx& sfx) {
  if (!audio.valid || sfx.id < 0) return;
  auto it = AudioBackends().find(audio.id);
  if (it == AudioBackends().end() || !it->second.device_ok) return;
  auto sound = it->second.sounds.find(sfx.id);
  if (sound == it->second.sounds.end()) return;
  ::PlaySound(sound->second);
}

void UnloadSfx(Audio& audio, const Sfx& sfx) {
  if (!audio.valid || sfx.id < 0) return;
  auto it = AudioBackends().find(audio.id);
  if (it == AudioBackends().end()) return;
  auto sound = it->second.sounds.find(sfx.id);
  if (sound == it->second.sounds.end()) return;
  if (it->second.device_ok) ::UnloadSound(sound->second);
  it->second.sounds.erase(sound);
}

}  // namespace clank::m2
