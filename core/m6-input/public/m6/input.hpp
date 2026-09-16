#pragma once
// WHY: record/replay must reuse .clk text lines for determinism.
#include <expected>
#include <string>
#include <vector>
namespace clank::m6 {
enum class Key : int { Up, Down, Left, Right, Space, Escape };
enum class Edge : int { Down, Up };
struct InputEvent {
  int frame = 0;
  Key key = Key::Up;
  Edge edge = Edge::Down;
};
struct Recorder {
  std::vector<InputEvent> events;
  void Push(InputEvent e);
  void Clear();
  std::expected<void, std::string> SaveClk(const std::string& path) const;
};
struct Playback {
  std::vector<InputEvent> events;
  std::expected<void, std::string> Load(const std::string& path);
  bool IsDown(int frame, Key key) const;
};
}  // namespace clank::m6
