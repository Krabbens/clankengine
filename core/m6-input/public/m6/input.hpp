#pragma once
// WHY: record/replay must reuse .clk text lines for determinism.
#include <expected>
#include <string>
#include <string_view>
#include <vector>
namespace clank::m6 {
enum class Key : int { Up, Down, Left, Right, Space, Restart, Interact, Escape };
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
struct ActionBinding {
  std::string name;
  Key key = Key::Up;
};
struct ActionMap {
  // WHY: a small vector keeps binding order explicit and lookup deterministic.
  std::vector<ActionBinding> bindings;
};
std::expected<void, std::string> Bind(ActionMap& map, std::string name, Key key);
std::expected<Key, std::string> Lookup(const ActionMap& map, std::string_view name);
std::expected<bool, std::string> IsDown(const ActionMap& map, const Playback& playback, int frame,
                                        std::string_view name);
}  // namespace clank::m6
