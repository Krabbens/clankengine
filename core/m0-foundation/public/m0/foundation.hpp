#pragma once
// m0-foundation: tiny shared contract for agents (types, log, RNG, clock, flags).
// WHY one header: a single include keeps the core API surface countable (RISC).
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>

namespace clank::m0 {

template <typename T>
using Result = std::expected<T, std::string>;

enum class Level { Info, Warn, Error };

// WHY stderr: logs must never pollute stdout (machine paths live there).
void Log(Level level, std::string_view msg);

// WHY splitmix64: ~10 lines, u64 state only, bit-identical across platforms.
struct Rng {
  uint64_t state = 0;
};
void Seed(Rng& rng, uint64_t seed);
uint64_t NextU64(Rng& rng);
double NextFloat01(Rng& rng);

// WHY manual stepper: fixed dt + seeded RNG is what makes headless replay exact.
struct Clock {
  double dt = 1.0 / 60.0;
  double acc = 0.0;
  uint64_t steps = 0;
};
Clock Construct(double dt);
int Advance(Clock& clock, double elapsed);
uint64_t StepCount(const Clock& clock);

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
