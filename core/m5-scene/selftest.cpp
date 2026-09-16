#include <cstdio>

#include "m5/scene.hpp"
int main() {
  clank::m5::Scene s;
  s.seed = 42;
  s.entities.push_back({1, "player one", 1.5f, -2.0f, 0.25f, 1.0f, 1.0f});
  s.entities.push_back({2, "hi \"q\" \\ r", 0.0f, 0.5f, -1.0f, 2.0f, 0.5f});
  s.entities.push_back({3, "line\nbreak", 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.5f});
  std::string j = clank::m5::DumpJson(s);
  if (j.find("\"z\":1.5") == std::string::npos) return 1;
  auto back = clank::m5::LoadJson(j);
  if (!back) {
    std::fprintf(stderr, "load: %s\n", back.error().c_str());
    return 1;
  }
  if (back->seed != s.seed || back->entities.size() != s.entities.size()) return 1;
  for (size_t i = 0; i < s.entities.size(); ++i) {
    const auto& a = s.entities[i];
    const auto& b = back->entities[i];
    if (a.id != b.id || a.name != b.name || a.x != b.x || a.y != b.y || a.angle != b.angle ||
        a.sx != b.sx || a.sy != b.sy || a.z != b.z) {
      std::fprintf(stderr, "roundtrip mismatch entity %zu\n", i);
      return 1;
    }
  }
  // Any key order, extra whitespace, version 1, \u escapes.
  auto reord = clank::m5::LoadJson(
      "{ \"seed\" : 7 , \"entities\" : [ { \"sy\" : 1 , \"sx\" : 1 , \"angle\" : 0 , "
      "\"y\" : 2 , \"x\" : 1 , \"z\" : 0.5 , \"name\" : \"a\\u2603b\\n\" , \"id\" : 9 } ] , "
      "\"version\" : 1 }");
  if (!reord || reord->seed != 7 || reord->entities.size() != 1) return 1;
  const auto& e = reord->entities[0];
  const std::string snow =
      "a\xE2\x98\x83"
      "b\n";
  if (e.id != 9 || e.z != 0.5f || e.name != snow) return 1;
  // Strict: unknown/dup/missing keys, bad version, trailing comma, lone surrogate.
  if (clank::m5::LoadJson("{\"version\":2,\"seed\":1,\"entities\":[]}")) return 1;
  if (clank::m5::LoadJson("{\"version\":0,\"seed\":1,\"entities\":[{\"id\":1}]}")) return 1;
  if (clank::m5::LoadJson("{\"version\":0,\"seed\":1,\"entities\":[],\"seed\":2}")) return 1;
  if (clank::m5::LoadJson("{\"version\":0,\"seed\":1,\"entities\":[],\"bogus\":0}")) return 1;
  if (clank::m5::LoadJson("{\"version\":0,\"seed\":1,\"entities\":[{\"id\":1,}]}")) return 1;
  if (clank::m5::LoadJson(
          "{\"version\":0,\"seed\":1,\"entities\":[{\"id\":1,\"name\":\"\\ud800\"}]}"))
    return 1;
  if (clank::m5::LoadJson("{\"version\":0,\"seed\":1.5,\"entities\":[]}")) return 1;
  if (clank::m5::LoadJson("{\"version\":0,\"seed\":2147483648,\"entities\":[]}")) return 1;
  if (clank::m5::LoadJson("{\"version\":0,\"seed\":1,\"entities\":[{\"id\":1.5,\"name\":\"a\","
                          "\"x\":0,\"y\":0,\"angle\":0,\"sx\":1,\"sy\":1}]}"))
    return 1;
  if (clank::m5::LoadJson("{\"version\":0,\"seed\":1,\"entities\":[{\"id\":1,\"name\":\"a\","
                          "\"x\":1e9999,\"y\":0,\"angle\":0,\"sx\":1,\"sy\":1}]}"))
    return 1;
  std::printf("m5-selftest ok\n");
  return 0;
}
