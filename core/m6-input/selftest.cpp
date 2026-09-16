#include <cstdio>
#include <fstream>

#include "m6/input.hpp"

int main() {
  clank::m6::Recorder r;
  r.Push({10, clank::m6::Key::Up, clank::m6::Edge::Down});
  r.Push({12, clank::m6::Key::Up, clank::m6::Edge::Up});
  r.Push({15, clank::m6::Key::Space, clank::m6::Edge::Down});
  r.Push({20, clank::m6::Key::Restart, clank::m6::Edge::Down});
  r.Push({21, clank::m6::Key::Restart, clank::m6::Edge::Up});
  r.Push({25, clank::m6::Key::Interact, clank::m6::Edge::Down});
  const char* p = "/tmp/m6-clk-test.clk";
  std::remove(p);
  if (!r.SaveClk(p)) return 1;
  clank::m6::Playback b;
  if (!b.Load(p)) return 1;
  if (b.events.size() != 6) return 1;
  if (!b.IsDown(10, clank::m6::Key::Up)) return 1;
  if (!b.IsDown(11, clank::m6::Key::Up)) return 1;
  if (b.IsDown(12, clank::m6::Key::Up)) return 1;
  if (b.IsDown(9, clank::m6::Key::Up)) return 1;
  if (b.IsDown(11, clank::m6::Key::Space)) return 1;
  if (!b.IsDown(15, clank::m6::Key::Space)) return 1;
  if (!b.IsDown(99, clank::m6::Key::Space)) return 1;
  if (!b.IsDown(20, clank::m6::Key::Restart)) return 1;
  if (b.IsDown(21, clank::m6::Key::Restart)) return 1;
  if (!b.IsDown(25, clank::m6::Key::Interact)) return 1;
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

  clank::m6::Playback action_playback;
  if (!action_playback.Load(p)) return 1;
  clank::m6::ActionMap actions;
  if (!clank::m6::Bind(actions, "MoveUp", clank::m6::Key::Up)) return 1;
  if (!clank::m6::Bind(actions, "Jump", clank::m6::Key::Space)) return 1;
  auto move_up = clank::m6::IsDown(actions, action_playback, 11, "MoveUp");
  if (!move_up || !*move_up) return 1;
  auto jump = clank::m6::IsDown(actions, action_playback, 11, "Jump");
  if (!jump || *jump) return 1;
  jump = clank::m6::IsDown(actions, action_playback, 15, "Jump");
  if (!jump || !*jump) return 1;
  auto lookup = clank::m6::Lookup(actions, "MoveUp");
  if (!lookup || *lookup != clank::m6::Key::Up) return 1;
  auto unknown = clank::m6::IsDown(actions, action_playback, 11, "Unknown");
  if (unknown || unknown.error() != "unknown action: Unknown") return 1;
  auto empty = clank::m6::Bind(actions, "", clank::m6::Key::Escape);
  if (empty || empty.error() != "empty action name") return 1;
  auto duplicate = clank::m6::Bind(actions, "MoveUp", clank::m6::Key::Down);
  if (duplicate || duplicate.error() != "duplicate action name: MoveUp") return 1;
  auto conflict = clank::m6::Bind(actions, "Crouch", clank::m6::Key::Up);
  if (conflict || conflict.error() != "binding conflict for key") return 1;

  std::printf("m6-selftest ok\n");
  return 0;
}
