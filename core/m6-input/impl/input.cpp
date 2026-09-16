#include "m6/input.hpp"

#include <algorithm>
#include <charconv>
#include <fstream>
#include <sstream>

namespace clank::m6 {
namespace {
// WHY: .clk lines stay human-readable so agents can diff replays as text.
const char* KeyStr(Key k) {
  switch (k) {
    case Key::Up:
      return "Up";
    case Key::Down:
      return "Down";
    case Key::Left:
      return "Left";
    case Key::Right:
      return "Right";
    case Key::Space:
      return "Space";
    case Key::Escape:
      return "Escape";
  }
  return "Up";
}
const char* EdgeStr(Edge e) { return e == Edge::Down ? "Down" : "Up"; }
bool KeyParse(const std::string& s, Key& k) {
  if (s == "Up")
    k = Key::Up;
  else if (s == "Down")
    k = Key::Down;
  else if (s == "Left")
    k = Key::Left;
  else if (s == "Right")
    k = Key::Right;
  else if (s == "Space")
    k = Key::Space;
  else if (s == "Escape")
    k = Key::Escape;
  else
    return false;
  return true;
}
bool EdgeParse(const std::string& s, Edge& e) {
  if (s == "Down")
    e = Edge::Down;
  else if (s == "Up")
    e = Edge::Up;
  else
    return false;
  return true;
}

bool ValidKey(Key k) {
  const int value = static_cast<int>(k);
  return value >= static_cast<int>(Key::Up) && value <= static_cast<int>(Key::Escape);
}

bool ValidEdge(Edge e) {
  const int value = static_cast<int>(e);
  return value >= static_cast<int>(Edge::Down) && value <= static_cast<int>(Edge::Up);
}

bool ParseFrame(const std::string& token, int& frame) {
  const auto [end, error] = std::from_chars(token.data(), token.data() + token.size(), frame);
  return error == std::errc() && end == token.data() + token.size() && frame >= 0;
}
}  // namespace
void Recorder::Push(InputEvent e) { events.push_back(e); }
void Recorder::Clear() { events.clear(); }
std::expected<void, std::string> Recorder::SaveClk(const std::string& path) const {
  for (size_t i = 0; i < events.size(); ++i) {
    const auto& e = events[i];
    if (e.frame < 0 || !ValidKey(e.key) || !ValidEdge(e.edge))
      return std::unexpected("bad event " + std::to_string(i));
  }
  std::ofstream o(path, std::ios::trunc);
  if (!o) return std::unexpected("open " + path);
  for (const auto& e : events)
    o << e.frame << ' ' << KeyStr(e.key) << ' ' << EdgeStr(e.edge) << '\n';
  if (!o) return std::unexpected("write " + path);
  return {};
}
std::expected<void, std::string> Playback::Load(const std::string& path) {
  std::ifstream in(path);
  if (!in) return std::unexpected("open " + path);
  std::vector<InputEvent> v;
  std::string line;
  while (std::getline(in, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    std::istringstream words(line);
    std::string frame_token;
    std::string key_token;
    std::string edge_token;
    std::string extra;
    if (line.find_first_not_of(" \t") == std::string::npos) continue;
    if (!(words >> frame_token >> key_token >> edge_token))
      return std::unexpected("bad line: " + line);
    if (words >> extra) return std::unexpected("bad line: " + line);
    int f = 0;
    Key k = Key::Up;
    Edge e = Edge::Down;
    if (!ParseFrame(frame_token, f) || !KeyParse(key_token, k) || !EdgeParse(edge_token, e))
      return std::unexpected("bad token: " + line);
    v.push_back({f, k, e});
  }
  std::stable_sort(v.begin(), v.end(),
                   [](const InputEvent& a, const InputEvent& b) { return a.frame < b.frame; });
  events = std::move(v);
  return {};
}
bool Playback::IsDown(int frame, Key key) const {
  bool down = false;
  bool found = false;
  int last_frame = 0;
  for (const auto& e : events) {
    if (e.key != key || e.frame > frame || (found && e.frame < last_frame)) continue;
    down = e.edge == Edge::Down;
    last_frame = e.frame;
    found = true;
  }
  return down;
}
}  // namespace clank::m6
