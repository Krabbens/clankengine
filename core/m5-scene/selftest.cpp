#include <cmath>
#include <cstdio>
#include <utility>

#include "m5/scene.hpp"
int main() {
  clank::m5::Scene s;
  s.seed = 42;
  s.entities.push_back({1, "player one", 1.5f, -2.0f, 0.25f, 1.0f, 1.0f});
  s.entities.push_back({2, "hi \"q\" \\ r", 0.0f, 0.5f, -1.0f, 2.0f, 0.5f});
  s.entities.push_back({3, "line\nbreak", 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.5f});
  s.entities.push_back({4, "child", 2.0f, 0.0f, 0.5f, 0.5f, 1.0f, 0.25f, 1});
  std::string j = clank::m5::DumpJson(s);
  if (j.find("\"z\":1.5") == std::string::npos || j.find("\"parent\":1") == std::string::npos)
    return 1;
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
        a.sx != b.sx || a.sy != b.sy || a.z != b.z || a.parent != b.parent) {
      std::fprintf(stderr, "roundtrip mismatch entity %zu\n", i);
      return 1;
    }
  }
  clank::m5::Scene tree;
  tree.entities.push_back({10, "root", 10, 20, 0.5f, 2, 3, 4});
  tree.entities.push_back({11, "child", 1, 0, 0.25f, 0.5f, 2, 6, 10});
  tree.entities.push_back({12, "grandchild", 0, 1, 0, 1, 1, 2, 11});
  auto world = clank::m5::ResolveWorldTransforms(tree);
  if (!world) return 1;
  if (std::fabs((*world)[1].x - 11.755165f) > 0.001f ||
      std::fabs((*world)[1].y - 20.958851f) > 0.001f || std::fabs((*world)[1].z - 10.0f) > 0.001f ||
      std::fabs((*world)[2].x - 8.099567f) > 0.001f ||
      std::fabs((*world)[2].y - 25.586208f) > 0.001f)
    return 1;
  auto missing = tree;
  missing.entities[1].parent = 99;
  if (clank::m5::ResolveWorldTransforms(missing)) return 1;
  auto cycle = tree;
  cycle.entities[0].parent = 12;
  if (clank::m5::ResolveWorldTransforms(cycle)) return 1;
  auto duplicate = tree;
  duplicate.entities[2].id = 11;
  if (clank::m5::ResolveWorldTransforms(duplicate)) return 1;
  auto invalid_parent = tree;
  invalid_parent.entities[1].parent = -2;
  if (clank::m5::ResolveWorldTransforms(invalid_parent)) return 1;
  const std::vector<clank::m5::Component> components = {
      {100, 11, "Sprite"}, {101, 10, "Health"}, {102, 11, "Collider"}};
  auto actor_components = clank::m5::ResolveComponents(tree, components, 11);
  if (!actor_components || *actor_components != std::vector<int>{100, 102}) return 1;
  auto reordered_tree = tree;
  std::swap(reordered_tree.entities[0], reordered_tree.entities[1]);
  auto reordered_components = clank::m5::ResolveComponents(reordered_tree, components, 11);
  if (!reordered_components || *reordered_components != std::vector<int>{100, 102}) return 1;
  auto reordered_registry = components;
  std::swap(reordered_registry[0], reordered_registry[2]);
  auto input_order = clank::m5::ResolveComponents(tree, reordered_registry, 11);
  if (!input_order || *input_order != std::vector<int>{102, 100}) return 1;
  if (clank::m5::ResolveComponents(tree, components, 99)) return 1;
  auto missing_owner = components;
  missing_owner[0].owner = 99;
  if (clank::m5::ResolveComponents(tree, missing_owner, 11)) return 1;
  auto duplicate_component = components;
  duplicate_component[2].id = duplicate_component[0].id;
  if (clank::m5::ResolveComponents(tree, duplicate_component, 11)) return 1;
  auto duplicate_component_cross_owner = components;
  duplicate_component_cross_owner[1].id = duplicate_component_cross_owner[0].id;
  if (clank::m5::ResolveComponents(tree, duplicate_component_cross_owner, 11)) return 1;
  auto duplicate_actor = tree;
  duplicate_actor.entities[2].id = 11;
  if (clank::m5::ResolveComponents(duplicate_actor, components, 11)) return 1;
  auto empty_type = components;
  empty_type[1].type.clear();
  if (clank::m5::ResolveComponents(tree, empty_type, 11)) return 1;
  clank::m5::ComponentStore store;
  auto attached_collider = clank::m5::AttachComponent(store, tree, components[2]);
  if (!attached_collider || *attached_collider != 102 || store.components.size() != 1) return 1;
  auto attached_sprite = clank::m5::AttachComponent(store, tree, components[0]);
  if (!attached_sprite || *attached_sprite != 100 || store.components.size() != 2) return 1;
  auto by_id = clank::m5::LookupComponent(store, 100);
  if (!by_id || by_id->id != 100 || by_id->owner != 11 || by_id->type != "Sprite") return 1;
  auto by_owner = clank::m5::LookupComponents(store, 11);
  if (!by_owner || *by_owner != std::vector<int>{100, 102}) return 1;
  auto duplicate_store_id = clank::m5::AttachComponent(store, tree, {100, 10, "Health"});
  if (duplicate_store_id) return 1;
  auto missing_store_owner = clank::m5::AttachComponent(store, tree, {103, 99, "Audio"});
  if (missing_store_owner) return 1;
  auto empty_store_type = clank::m5::AttachComponent(store, tree, {103, 10, ""});
  if (empty_store_type) return 1;
  std::swap(store.components[0], store.components[1]);
  std::swap(tree.entities[0], tree.entities[1]);
  auto reordered_by_owner = clank::m5::LookupComponents(store, 11);
  auto reordered_by_id = clank::m5::LookupComponent(store, 102);
  if (!reordered_by_owner || *reordered_by_owner != std::vector<int>{100, 102} ||
      !reordered_by_id || reordered_by_id->type != "Collider")
    return 1;
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
  if (clank::m5::LoadJson("{\"version\":0,\"seed\":1,\"entities\":[{\"id\":1,\"name\":\"a\","
                          "\"x\":0,\"y\":0,\"angle\":0,\"sx\":1,\"sy\":1,\"parent\":-2}]}"))
    return 1;
  if (clank::m5::LoadJson("{\"version\":0,\"seed\":1,\"entities\":[{\"id\":1,\"name\":\"a\","
                          "\"x\":0,\"y\":0,\"angle\":0,\"sx\":1,\"sy\":1},{\"id\":1,"
                          "\"name\":\"b\",\"x\":0,\"y\":0,\"angle\":0,\"sx\":1,\"sy\":1}]}"))
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
