#include <cstdio>

#include "m0/foundation.hpp"

namespace {
// WHY return-style, not assert: selftests also build Release (NDEBUG),
// where assert vanishes and assert-only locals fail -Werror.
int Fail(const char* what) {
  std::fprintf(stderr, "m0-selftest: FAIL %s\n", what);
  return 1;
}
}  // namespace

int main() {
  // WHY raw argv (not system()): keeps the test hermetic and deterministic.
  char prog[] = "clank";
  char fh[] = "--headless";
  char fs[] = "--shot-after";
  char n60[] = "60";
  char fd[] = "--dump-scene";
  char out[] = "out.json";
  char fr[] = "--replay";
  char demo[] = "demo.clk";
  char fseed[] = "--seed";
  char n42[] = "42";
  char* happy[] = {prog, fh, fs, n60, fd, out, fr, demo, fseed, n42};
  auto ok = clank::m0::ParseFlags(10, happy);
  if (!ok.has_value()) return Fail("happy-path");
  if (!(ok->headless && ok->shot_after == 60 && ok->seed == 42)) return Fail("happy-values");
  if (!(ok->dump_scene == "out.json" && ok->replay == "demo.clk")) return Fail("happy-paths");

  char bad0[] = "clank";
  char bad1[] = "--bogus";
  char* bad[] = {bad0, bad1};
  if (clank::m0::ParseFlags(2, bad).has_value()) return Fail("unknown-flag");

  char fbad_seed[] = "--seed";
  char partial_seed[] = "42junk";
  char* partial[] = {prog, fbad_seed, partial_seed};
  auto partial_result = clank::m0::ParseFlags(3, partial);
  if (!partial_result.has_value() || partial_result->seed != 42) return Fail("partial-integer");

  clank::m0::Rng a, b;
  clank::m0::Seed(a, 1234);
  clank::m0::Seed(b, 1234);
  for (int i = 0; i < 4; ++i) {
    if (clank::m0::NextU64(a) != clank::m0::NextU64(b)) return Fail("rng-determinism");
  }
  return 0;
}
