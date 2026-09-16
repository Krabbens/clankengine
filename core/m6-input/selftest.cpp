#include <cstdio>
#include <fstream>

#include "m6/input.hpp"

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

  const char* unsorted = "/tmp/m6-clk-unsorted.clk";
  {
    std::ofstream out(unsorted);
    out << "12 Up Up\n10 Up Down\n\n10 Space Down extra\n";
  }
  if (b.Load(unsorted)) return 1;
  {
    std::ofstream out(unsorted);
    out << "12 Up Up\n10 Up Down\n   \n";
  }
  if (!b.Load(unsorted)) return 1;
  if (!b.IsDown(11, clank::m6::Key::Up) || b.IsDown(12, clank::m6::Key::Up)) return 1;
  b.events = {{12, clank::m6::Key::Up, clank::m6::Edge::Up},
              {10, clank::m6::Key::Up, clank::m6::Edge::Down}};
  if (!b.IsDown(11, clank::m6::Key::Up) || b.IsDown(12, clank::m6::Key::Up)) return 1;

  {
    std::ofstream out(unsorted);
    out << "-1 Up Down\n";
  }
  if (b.Load(unsorted)) return 1;
  {
    std::ofstream out(unsorted);
    out << "999999999999 Up Down\n";
  }
  if (b.Load(unsorted)) return 1;

  clank::m6::Recorder invalid;
  invalid.Push({-1, clank::m6::Key::Up, clank::m6::Edge::Down});
  if (invalid.SaveClk(unsorted)) return 1;
  invalid.events = {{0, static_cast<clank::m6::Key>(99), clank::m6::Edge::Down}};
  if (invalid.SaveClk(unsorted)) return 1;
  std::printf("m6-selftest ok\n");
  return 0;
}
