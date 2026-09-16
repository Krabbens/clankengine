#include "m6/input.hpp"
#include <cstdio>
int main() {
  clank::m6::Recorder r;
  r.Push({10, clank::m6::Key::Up, clank::m6::Edge::Down});
  r.Push({12, clank::m6::Key::Up, clank::m6::Edge::Up});
  r.Push({15, clank::m6::Key::Space, clank::m6::Edge::Down});
  const char* p = "/tmp/m6-clk-test.clk";
  std::remove(p);
  if (!r.SaveClk(p)) return 1;
  clank::m6::Playback b;
  if (!b.Load(p)) return 1;
  if (b.events.size() != 3) return 1;
  if (!b.IsDown(10, clank::m6::Key::Up)) return 1;
  if (!b.IsDown(11, clank::m6::Key::Up)) return 1;
  if (b.IsDown(12, clank::m6::Key::Up)) return 1;
  if (b.IsDown(9, clank::m6::Key::Up)) return 1;
  if (b.IsDown(11, clank::m6::Key::Space)) return 1;
  if (!b.IsDown(15, clank::m6::Key::Space)) return 1;
  if (!b.IsDown(99, clank::m6::Key::Space)) return 1;
  if (b.IsDown(99, clank::m6::Key::Left)) return 1;
  std::printf("m6-selftest ok\n");
  return 0;
}
