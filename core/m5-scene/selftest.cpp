#include "m5/scene.hpp"
#include <cstdio>
int main() {
  clank::m5::Scene s;
  s.seed = 42;
  s.entities.push_back({1, "player one", 1.5f, -2.0f, 0.25f, 1.0f, 1.0f});
  s.entities.push_back({2, "hi \"q\" \\ r", 0.0f, 0.5f, -1.0f, 2.0f, 0.5f});
  std::string j = clank::m5::DumpJson(s);
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
        a.sx != b.sx || a.sy != b.sy) {
      std::fprintf(stderr, "roundtrip mismatch entity %zu\n", i);
      return 1;
    }
  }
  const char* p = "/tmp/m5-clk-test.clk";
  std::remove(p);
  if (!clank::m5::AppendClk(p, "10 spawn player one") || !clank::m5::AppendClk(p, "12 move 1 2.0 3.0")) return 1;
  auto lines = clank::m5::ReadClk(p);
  if (!lines || lines->size() != 2) return 1;
  if ((*lines)[0] != "10 spawn player one" || (*lines)[1] != "12 move 1 2.0 3.0") return 1;
  std::printf("m5-selftest ok\n");
  return 0;
}
