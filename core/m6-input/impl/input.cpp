#include "m6/input.hpp"
#include <cstdio>
#include <fstream>
namespace clank::m6 {
namespace {
// WHY: .clk lines stay human-readable so agents can diff replays as text.
const char* KeyStr(Key k) {
  switch (k) {
    case Key::Up: return "Up";
    case Key::Down: return "Down";
    case Key::Left: return "Left";
    case Key::Right: return "Right";
    case Key::Space: return "Space";
    case Key::Escape: return "Escape";
  }
  return "Up";
}
const char* EdgeStr(Edge e) { return e == Edge::Down ? "Down" : "Up"; }
bool KeyParse(const std::string& s, Key& k) {
  if (s == "Up") k = Key::Up;
  else if (s == "Down") k = Key::Down;
  else if (s == "Left") k = Key::Left;
  else if (s == "Right") k = Key::Right;
  else if (s == "Space") k = Key::Space;
  else if (s == "Escape") k = Key::Escape;
  else return false;
  return true;
}
bool EdgeParse(const std::string& s, Edge& e) {
  if (s == "Down") e = Edge::Down;
  else if (s == "Up") e = Edge::Up;
  else return false;
  return true;
}
}  // namespace
void Recorder::Push(InputEvent e) { events.push_back(e); }
void Recorder::Clear() { events.clear(); }
std::expected<void, std::string> Recorder::SaveClk(const std::string& path) const {
  std::ofstream o(path, std::ios::trunc);
  if (!o) return std::unexpected("open " + path);
  for (const auto& e : events) o << e.frame << ' ' << KeyStr(e.key) << ' ' << EdgeStr(e.edge) << '\n';
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
    if (line.empty()) continue;
    int f = 0;
    char ks[16] = {};
    char es[16] = {};
    if (std::sscanf(line.c_str(), "%d %15s %15s", &f, ks, es) != 3)
      return std::unexpected("bad line: " + line);
    Key k = Key::Up;
    Edge e = Edge::Down;
    if (!KeyParse(ks, k) || !EdgeParse(es, e)) return std::unexpected("bad token: " + line);
    v.push_back({f, k, e});
  }
  events = std::move(v);
  return {};
}
bool Playback::IsDown(int frame, Key key) const {
  bool down = false;
  for (const auto& e : events)
    if (e.key == key && e.frame <= frame) down = (e.edge == Edge::Down);
  return down;
}
}  // namespace clank::m6
