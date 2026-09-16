# Visual and performance target

The reference for the showcase is a small, stylized 3D diorama: low-poly
silhouettes, a restrained green/teal/gold palette, crisp checker grass, warm
directional light, and readable ground shadows. The current Zelda scene is the
reference implementation. There is no post-processing requirement; effects are
limited to the shadows and palette grade needed for that look.

The baseline target is a four-core x86-64 laptop with an integrated OpenGL 3.3
GPU and 8 GB of RAM. Linux CI runs the same logical 1280x720 surface under
Xvfb/llvmpipe so the see-channel remains available without a monitor. The game
loop is fixed at 60 Hz:

| Budget | Target |
| --- | ---: |
| Full frame | 16.67 ms p95, 20 ms p99 |
| Simulation/update | 2 ms p95 |
| Draw submission | 8 ms p95 |
| Logged 2D + 3D draw entries | 256 per frame |

The numbers are budgets, not unverified claims of hardware performance. Every
sample run with `--shot-after` reports update, draw, and full-frame p50/p95/p99
to stderr, together with logged 2D and 3D draw counts. Use the real windowed
path for GPU measurements:

```sh
CLANK_WINDOWED_TEST=1 ctest --test-dir build --output-on-failure
xvfb-run -a ./build/zelda --shot-after 1080 \
  --replay examples/replays/zelda_win.clk --dump-scene /tmp/zelda.json --seed 42
```

Deterministic headless output is still required for CI: `--seed`, fixed ticks,
`--replay`, `--dump-scene`, and `--shot-after` must reproduce the JSON and PNG
on the same build/backend. Goldens in `test/golden/` are the pixel references;
the windowed test exercises the actual raylib renderer when Xvfb is available.
