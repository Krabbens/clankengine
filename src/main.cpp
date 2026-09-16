// clank skeleton: CLI + observability stub (dump/shot/drive).
// Real loop/render/physics land in core/mX/ as agents claim modules.
// Logs -> stderr, machine-readable results (paths) -> stdout.
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

namespace {

struct Args {
  bool headless = false;
  int shot_after = -1;
  std::string dump_scene;
  std::string replay;
  int seed = 42;
};

// Minimal 1x1 transparent PNG (67 bytes). Placeholder until m2-render
// wires raylib TakeScreenshot(). Deterministic by construction.
constexpr unsigned char kDotPng[] = {
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 0x15, 0xC4,
    0x89, 0x00, 0x00, 0x00, 0x0A, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9C, 0x63, 0x00, 0x01, 0x00, 0x00,
    0x05, 0x00, 0x01, 0x0D, 0x0A, 0x2D, 0xB4, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE,
    0x42, 0x60, 0x82};

bool WriteFile(const std::string& path, const char* data, std::size_t n) {
  std::ofstream out(path, std::ios::binary);
  if (!out) return false;
  out.write(data, static_cast<std::streamsize>(n));
  return static_cast<bool>(out);
}

int ParseInt(const char* s, int fallback) {
  if (s == nullptr) return fallback;
  try {
    return std::stoi(s);
  } catch (...) {
    return fallback;
  }
}

}  // namespace

int main(int argc, char** argv) {
  Args a;
  for (int i = 1; i < argc; ++i) {
    std::string t = argv[i];
    auto need = [&](const char* flag) {
      if (i + 1 >= argc) {
        std::fprintf(stderr, "clank: %s needs a value\n", flag);
        return false;
      }
      return true;
    };
    if (t == "--headless") {
      a.headless = true;
    } else if (t == "--shot-after") {
      if (!need("--shot-after")) return 2;
      a.shot_after = ParseInt(argv[++i], -1);
    } else if (t == "--dump-scene") {
      if (!need("--dump-scene")) return 2;
      a.dump_scene = argv[++i];
    } else if (t == "--replay") {
      if (!need("--replay")) return 2;
      a.replay = argv[++i];
    } else if (t == "--seed") {
      if (!need("--seed")) return 2;
      a.seed = ParseInt(argv[++i], 42);
    } else if (t == "--help" || t == "-h") {
      std::fprintf(stderr,
                   "usage: clank --headless --shot-after N --dump-scene out.json "
                   "[--replay demo.clk --seed 42]\n");
      return 0;
    } else {
      std::fprintf(stderr, "clank: unknown flag %s\n", t.c_str());
      return 2;
    }
  }

  std::fprintf(stderr, "clank: stub seed=%d headless=%d replay=%s\n", a.seed, int(a.headless),
               a.replay.empty() ? "-" : a.replay.c_str());

  if (!a.dump_scene.empty()) {
    std::ofstream out(a.dump_scene);
    if (!out) {
      std::fprintf(stderr, "clank: cannot write %s\n", a.dump_scene.c_str());
      return 1;
    }
    // Minimal scene doc; real schema lives in spec/scene.schema.json.
    out << "{\"version\":0,\"seed\":" << a.seed << ",\"entities\":[]}\n";
    out.close();
    std::printf("%s\n", a.dump_scene.c_str());
  }
  if (a.shot_after >= 0) {
    std::string png = "/tmp/clank_frame" + std::to_string(a.shot_after) + ".png";
    if (!WriteFile(png, reinterpret_cast<const char*>(kDotPng), sizeof(kDotPng))) {
      std::fprintf(stderr, "clank: cannot write %s\n", png.c_str());
      return 1;
    }
    std::fprintf(stderr, "clank: stub shot frame %d -> %s (1x1 until m2-render)\n", a.shot_after,
                 png.c_str());
    std::printf("%s\n", png.c_str());
  }
  return 0;
}
