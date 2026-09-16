#include "m0/foundation.hpp"

#include <cassert>

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
  assert(ok.has_value());
  assert(ok->headless && ok->shot_after == 60 && ok->seed == 42);
  assert(ok->dump_scene == "out.json" && ok->replay == "demo.clk");

  char bad0[] = "clank";
  char bad1[] = "--bogus";
  char* bad[] = {bad0, bad1};
  assert(!clank::m0::ParseFlags(2, bad).has_value());

  clank::m0::Rng a, b;
  clank::m0::Seed(a, 1234);
  clank::m0::Seed(b, 1234);
  for (int i = 0; i < 4; ++i) assert(clank::m0::NextU64(a) == clank::m0::NextU64(b));

  clank::m0::Clock c = clank::m0::Construct(1.0 / 60.0);
  assert(clank::m0::Advance(c, 1.0) == 60);
  assert(clank::m0::StepCount(c) == 60);
  return 0;
}
