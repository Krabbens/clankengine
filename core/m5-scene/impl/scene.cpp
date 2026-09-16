#include "m5/scene.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
namespace clank::m5 {
namespace {
// WHY: dump must survive spaces/quotes in names with only stdlib, no JSON dep.
void AppendEscaped(std::string& o, const std::string& s) {
  o.push_back('"');
  for (unsigned char c : s) {
    if (c == '"') o += "\\\"";
    else if (c == '\\') o += "\\\\";
    else o.push_back(static_cast<char>(c));
  }
  o.push_back('"');
}
// WHY: 9 significant digits roundtrip float exactly, keeps dumps deterministic.
void AppendFloat(std::string& o, float v) {
  char b[32];
  std::snprintf(b, sizeof b, "%.9g", static_cast<double>(v));
  o += b;
}
struct Cur { const char* p; const char* end; };
void SkipWs(Cur& c) {
  while (c.p < c.end && (*c.p == ' ' || *c.p == '\t' || *c.p == '\n' || *c.p == '\r')) ++c.p;
}
bool TakeLit(Cur& c, const char* s) {
  SkipWs(c);
  size_t n = std::strlen(s);
  if (static_cast<size_t>(c.end - c.p) < n || std::strncmp(c.p, s, n) != 0) return false;
  c.p += n;
  return true;
}
bool TakeNum(Cur& c, float& v) {
  SkipWs(c);
  char* e = nullptr;
  float x = std::strtof(c.p, &e);
  if (e == c.p || e > c.end) return false;
  v = x;
  c.p = e;
  return true;
}
// WHY: ints reuse the float path so there is one number parser, not two.
bool TakeInt(Cur& c, int& v) {
  float f = 0;
  if (!TakeNum(c, f)) return false;
  v = static_cast<int>(f);
  return true;
}
bool TakeStr(Cur& c, std::string& v) {
  SkipWs(c);
  if (c.p >= c.end || *c.p != '"') return false;
  ++c.p;
  v.clear();
  while (c.p < c.end) {
    char ch = *c.p++;
    if (ch == '"') return true;
    if (ch != '\\') { v.push_back(ch); continue; }
    if (c.p >= c.end) return false;
    char e = *c.p++;
    if (e == '"') v.push_back('"');
    else if (e == '\\') v.push_back('\\');
    else return false;
  }
  return false;
}
bool TakeEntity(Cur& c, Entity& e) {
  return TakeLit(c, "{") && TakeLit(c, "\"id\"") && TakeLit(c, ":") && TakeInt(c, e.id) &&
         TakeLit(c, ",") && TakeLit(c, "\"name\"") && TakeLit(c, ":") && TakeStr(c, e.name) &&
         TakeLit(c, ",") && TakeLit(c, "\"x\"") && TakeLit(c, ":") && TakeNum(c, e.x) &&
         TakeLit(c, ",") && TakeLit(c, "\"y\"") && TakeLit(c, ":") && TakeNum(c, e.y) &&
         TakeLit(c, ",") && TakeLit(c, "\"angle\"") && TakeLit(c, ":") && TakeNum(c, e.angle) &&
         TakeLit(c, ",") && TakeLit(c, "\"sx\"") && TakeLit(c, ":") && TakeNum(c, e.sx) &&
         TakeLit(c, ",") && TakeLit(c, "\"sy\"") && TakeLit(c, ":") && TakeNum(c, e.sy) &&
         TakeLit(c, "}");
}
}  // namespace
std::string DumpJson(const Scene& s) {
  std::string o = "{\"version\":0,\"seed\":" + std::to_string(s.seed) + ",\"entities\":[";
  bool first = true;
  for (const Entity& e : s.entities) {
    if (!first) o.push_back(',');
    first = false;
    o += "{\"id\":" + std::to_string(e.id);
    o += ",\"name\":";
    AppendEscaped(o, e.name);
    o += ",\"x\":";
    AppendFloat(o, e.x);
    o += ",\"y\":";
    AppendFloat(o, e.y);
    o += ",\"angle\":";
    AppendFloat(o, e.angle);
    o += ",\"sx\":";
    AppendFloat(o, e.sx);
    o += ",\"sy\":";
    AppendFloat(o, e.sy);
    o.push_back('}');
  }
  o += "]}";
  return o;
}
std::expected<Scene, std::string> LoadJson(const std::string& text) {
  Cur c{text.data(), text.data() + text.size()};
  Scene s;
  int ver = -1;
  if (!TakeLit(c, "{")) return std::unexpected("want {");
  if (!TakeLit(c, "\"version\"") || !TakeLit(c, ":") || !TakeInt(c, ver)) return std::unexpected("bad version");
  if (ver != 0) return std::unexpected("unsupported version");
  if (!TakeLit(c, ",") || !TakeLit(c, "\"seed\"") || !TakeLit(c, ":") || !TakeInt(c, s.seed))
    return std::unexpected("bad seed");
  if (!TakeLit(c, ",") || !TakeLit(c, "\"entities\"") || !TakeLit(c, ":") || !TakeLit(c, "["))
    return std::unexpected("bad entities");
  SkipWs(c);
  if (!TakeLit(c, "]")) {
    while (true) {
      Entity e;
      if (!TakeEntity(c, e)) return std::unexpected("bad entity");
      s.entities.push_back(std::move(e));
      if (TakeLit(c, ",")) continue;
      if (TakeLit(c, "]")) break;
      return std::unexpected("want , or ]");
    }
  }
  if (!TakeLit(c, "}")) return std::unexpected("want }");
  SkipWs(c);
  if (c.p != c.end) return std::unexpected("trailing bytes");
  return s;
}
std::expected<void, std::string> AppendClk(const std::string& path, const std::string& line) {
  std::ofstream o(path, std::ios::app);
  if (!o) return std::unexpected("open " + path);
  o << line << '\n';
  if (!o) return std::unexpected("write " + path);
  return {};
}
std::expected<std::vector<std::string>, std::string> ReadClk(const std::string& path) {
  std::ifstream in(path);
  if (!in) return std::unexpected("open " + path);
  std::vector<std::string> v;
  std::string l;
  while (std::getline(in, l)) {
    if (!l.empty() && l.back() == '\r') l.pop_back();
    v.push_back(l);
  }
  return v;
}
}  // namespace clank::m5
