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
    if (c == '"')
      o += "\\\"";
    else if (c == '\\')
      o += "\\\\";
    else if (c == '\n')
      o += "\\n";
    else if (c == '\r')
      o += "\\r";
    else if (c == '\t')
      o += "\\t";
    else if (c == '\b')
      o += "\\b";
    else if (c == '\f')
      o += "\\f";
    else if (c < 0x20) {
      // WHY \u00XX, not octal: strict JSON forbids raw control bytes and \x.
      char b[8];
      std::snprintf(b, sizeof b, "\\u%04x", c);
      o += b;
    } else
      o.push_back(static_cast<char>(c));
  }
  o.push_back('"');
}
// WHY: 9 significant digits roundtrip float exactly, keeps dumps deterministic.
void AppendFloat(std::string& o, float v) {
  char b[32];
  std::snprintf(b, sizeof b, "%.9g", static_cast<double>(v));
  o += b;
}
struct Cur {
  const char* p;
  const char* end;
};
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
bool TakeHex4(Cur& c, unsigned& v) {
  v = 0;
  for (int i = 0; i < 4; ++i) {
    if (c.p >= c.end) return false;
    char ch = *c.p++;
    v <<= 4;
    if (ch >= '0' && ch <= '9')
      v |= static_cast<unsigned>(ch - '0');
    else if (ch >= 'a' && ch <= 'f')
      v |= static_cast<unsigned>(ch - 'a' + 10);
    else if (ch >= 'A' && ch <= 'F')
      v |= static_cast<unsigned>(ch - 'A' + 10);
    else
      return false;
  }
  return true;
}
// WHY: JSON \uXXXX is UTF-16; encode UTF-8 so names survive any language.
void AppendUtf8(std::string& v, unsigned cp) {
  if (cp < 0x80) {
    v.push_back(static_cast<char>(cp));
  } else if (cp < 0x800) {
    v.push_back(static_cast<char>(0xC0 | (cp >> 6)));
    v.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  } else if (cp < 0x10000) {
    v.push_back(static_cast<char>(0xE0 | (cp >> 12)));
    v.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    v.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  } else {
    v.push_back(static_cast<char>(0xF0 | (cp >> 18)));
    v.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
    v.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    v.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  }
}
// WHY table, not switch: 8 simple escapes share one lookup; \u keeps its own path.
bool TakeEscape(Cur& c, std::string& v) {
  static const char kFrom[] = "\"\\/bfnrt";
  static const char kTo[] = "\"\\/\b\f\n\r\t";
  if (c.p >= c.end) return false;
  char e = *c.p++;
  if (e != 'u') {
    const char* f = std::strchr(kFrom, e);
    if (!f) return false;
    v.push_back(kTo[f - kFrom]);
    return true;
  }
  unsigned cp = 0;
  if (!TakeHex4(c, cp)) return false;
  // WHY: lone surrogates are ill-formed; a high+low pair merges to one scalar.
  if (cp >= 0xD800 && cp <= 0xDBFF) {
    if (c.end - c.p < 2 || c.p[0] != '\\' || c.p[1] != 'u') return false;
    c.p += 2;
    unsigned lo = 0;
    if (!TakeHex4(c, lo) || lo < 0xDC00 || lo > 0xDFFF) return false;
    cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
  } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
    return false;
  }
  AppendUtf8(v, cp);
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
    if (ch != '\\') {
      v.push_back(ch);
      continue;
    }
    if (!TakeEscape(c, v)) return false;
  }
  return false;
}
// WHY dispatch: any key order; unknown/dup keys fail loud; z optional.
bool TakeEntity(Cur& c, Entity& e) {
  e = Entity{};
  if (!TakeLit(c, "{")) return false;
  bool id = false, name = false, x = false, y = false, a = false, sx = false, sy = false, z = false;
  while (true) {
    std::string key;
    if (!TakeStr(c, key) || !TakeLit(c, ":")) return false;
    if (key == "id") {
      if (id || !TakeInt(c, e.id)) return false;
      id = true;
    } else if (key == "name") {
      if (name || !TakeStr(c, e.name)) return false;
      name = true;
    } else if (key == "x") {
      if (x || !TakeNum(c, e.x)) return false;
      x = true;
    } else if (key == "y") {
      if (y || !TakeNum(c, e.y)) return false;
      y = true;
    } else if (key == "angle") {
      if (a || !TakeNum(c, e.angle)) return false;
      a = true;
    } else if (key == "sx") {
      if (sx || !TakeNum(c, e.sx)) return false;
      sx = true;
    } else if (key == "sy") {
      if (sy || !TakeNum(c, e.sy)) return false;
      sy = true;
    } else if (key == "z") {
      if (z || !TakeNum(c, e.z)) return false;
      z = true;
    } else {
      return false;
    }
    if (TakeLit(c, ",")) continue;
    if (TakeLit(c, "}")) break;
    return false;
  }
  return id && name && x && y && a && sx && sy;
}
}  // namespace
std::string DumpJson(const Scene& s) {
  // WHY version 0 on wire: bump + golden regen ride the m8 schema PR.
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
    if (e.z != 0.0f) {
      o += ",\"z\":";
      AppendFloat(o, e.z);
    }
    o.push_back('}');
  }
  o += "]}";
  return o;
}
std::expected<Scene, std::string> LoadJson(const std::string& text) {
  Cur c{text.data(), text.data() + text.size()};
  Scene s;
  int ver = -1;
  bool seen_v = false, seen_seed = false, seen_ent = false;
  if (!TakeLit(c, "{")) return std::unexpected("want {");
  while (true) {
    std::string key;
    if (!TakeStr(c, key) || !TakeLit(c, ":")) return std::unexpected("bad key");
    if (key == "version") {
      if (seen_v || !TakeInt(c, ver)) return std::unexpected("bad version");
      seen_v = true;
      if (ver != 0 && ver != 1) return std::unexpected("unsupported version");
    } else if (key == "seed") {
      if (seen_seed || !TakeInt(c, s.seed)) return std::unexpected("bad seed");
      seen_seed = true;
    } else if (key == "entities") {
      if (seen_ent || !TakeLit(c, "[")) return std::unexpected("bad entities");
      seen_ent = true;
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
    } else {
      return std::unexpected("unknown key " + key);
    }
    if (TakeLit(c, ",")) continue;
    if (TakeLit(c, "}")) break;
    return std::unexpected("want , or }");
  }
  if (!seen_v || !seen_seed || !seen_ent) return std::unexpected("missing key");
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
