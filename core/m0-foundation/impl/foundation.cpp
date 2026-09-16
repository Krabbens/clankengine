#include "m0/foundation.hpp"

#include <charconv>
#include <cstdio>
#include <string>

namespace clank::m0 {

void Log(Level level, std::string_view msg) {
  const char* tag = level == Level::Warn ? "warn" : level == Level::Error ? "error" : "info";
  std::fprintf(stderr, "clank [%s] %.*s\n", tag, static_cast<int>(msg.size()), msg.data());
}

void Seed(Rng& rng, uint64_t seed) { rng.state = seed; }

uint64_t NextU64(Rng& rng) {
  uint64_t z = (rng.state += 0x9E3779B97F4A7C15ULL);
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
  return z ^ (z >> 31);
}

double NextFloat01(Rng& rng) {
  // WHY top 53 bits: exactly the mantissa width, so every double in [0,1) is reachable.
  return static_cast<double>(NextU64(rng) >> 11) * (1.0 / 9007199254740992.0);
}

Clock Construct(double dt) {
  Clock c;
  c.dt = dt;
  return c;
}

int Advance(Clock& c, double elapsed) {
  // WHY epsilon: 60*(1/60) rounds above 1.0 in binary; without it step 60 is lost.
  if (c.dt <= 0.0 || elapsed <= 0.0) return 0;
  c.acc += elapsed;
  int n = static_cast<int>((c.acc + 1e-9) / c.dt);
  if (n <= 0) return 0;
  c.acc -= static_cast<double>(n) * c.dt;
  if (c.acc < 0.0) c.acc = 0.0;
  c.steps += static_cast<uint64_t>(n);
  return n;
}

uint64_t StepCount(const Clock& c) { return c.steps; }

namespace {
int ParseIntOr(const char* s, int fallback) {
  // WHY from_chars: stoi throws; the cold CLI path keeps the no-exceptions rule simple.
  // WHY fallback (not error): matches src/main.cpp stub for bad integers.
  if (s == nullptr || *s == '\0') return fallback;
  const char* end = s;
  while (*end != '\0') ++end;
  int v = fallback;
  const auto [parsed, error] = std::from_chars(s, end, v);
  return error == std::errc() && parsed == end ? v : fallback;
}
}  // namespace

Result<Flags> ParseFlags(int argc, char** argv) {
  Flags f;
  for (int i = 1; i < argc; ++i) {
    std::string_view t = argv[i];
    if (t == "--headless") {
      f.headless = true;
    } else if (t == "--shot-after") {
      if (i + 1 >= argc) return std::unexpected(std::string("clank: --shot-after needs a value"));
      f.shot_after = ParseIntOr(argv[++i], -1);
    } else if (t == "--dump-scene") {
      if (i + 1 >= argc) return std::unexpected(std::string("clank: --dump-scene needs a value"));
      f.dump_scene = argv[++i];
    } else if (t == "--replay") {
      if (i + 1 >= argc) return std::unexpected(std::string("clank: --replay needs a value"));
      f.replay = argv[++i];
    } else if (t == "--seed") {
      if (i + 1 >= argc) return std::unexpected(std::string("clank: --seed needs a value"));
      f.seed = ParseIntOr(argv[++i], 42);
    } else {
      return std::unexpected(std::string("clank: unknown flag ") + std::string(t));
    }
  }
  return f;
}

}  // namespace clank::m0
