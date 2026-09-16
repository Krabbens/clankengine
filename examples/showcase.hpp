#pragma once
// WHY thin adapter: the Drop/emitter demo shape stays here; the outer loop lives in sample_loop.
#include <raylib.h>

#include <algorithm>
#include <memory>

#include "m6/input.hpp"
#include "sample_loop.hpp"

namespace showcase {
constexpr Color ink{226, 235, 244, 255}, muted{135, 158, 181, 255};
constexpr Color colors[] = {{70, 218, 195, 255}, {255, 186, 99, 255}, {139, 151, 255, 255}};

template <class Demo>
int Run(int argc, char** argv, const char* name, const char* title, const char* subtitle) {
  float emitter = 0;
  bool previous_drop = false;
  return sample_loop::Run<Demo>(
      argc, argv,
      {name, "CLANK / PHYSICS LAB", title, subtitle,
       "Left/Right: emitter | Space: drop | P: pause | N: step | Esc: exit\n",
       "LEFT / RIGHT  move emitter     SPACE  drop     P  pause     N  step", colors[0], ink,
       muted},
      [](int seed) { return std::make_unique<Demo>(seed); },
      [&](Demo& demo, const sample_loop::Context& ctx, int frame, double dt) {
        emitter = std::clamp(emitter + 5 * static_cast<float>(dt) *
                                           (ctx.isDown(frame, clank::m6::Key::Right, KEY_RIGHT) -
                                            ctx.isDown(frame, clank::m6::Key::Left, KEY_LEFT)),
                             -5.0f, 5.0f);
        const bool drop = ctx.isDown(frame, clank::m6::Key::Space, KEY_SPACE);
        if (drop && !previous_drop) demo.Drop(emitter);
        demo.Step(static_cast<float>(dt));
        previous_drop = drop;
      },
      [&](const Demo& demo, clank::m2::Renderer& renderer) { demo.Draw(emitter, renderer); },
      [&](const Demo& demo, int seed) { return demo.Scene(seed, emitter); });
}
}  // namespace showcase
