#pragma once
// m0-foundation: tiny shared contract for agents (types, RNG, flags).
// WHY one header: a single include keeps the core API surface countable (RISC).
#include <cstdint>
#include <expected>
#include <string>

namespace clank::m0 {

template <typename T>
using Result = std::expected<T, std::string>;

// WHY splitmix64: ~10 lines, u64 state only, bit-identical across platforms.
struct Rng {
  uint64_t state = 0;
};
void Seed(Rng& rng, uint64_t seed);
uint64_t NextU64(Rng& rng);
double NextFloat01(Rng& rng);

// WHY mirror src/main.cpp defaults: flags are the agent observability contract.
// WHY strict: any unknown flag (incl. --help) is an error, matching the stub.
struct Flags {
  bool headless = false;
  int shot_after = -1;
  std::string dump_scene;
  std::string replay;
  int seed = 42;
};
Result<Flags> ParseFlags(int argc, char** argv);

}  // namespace clank::m0
