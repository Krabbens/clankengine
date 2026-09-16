# Wave 5 — observability first (decided 2026-09-16)

> Local continuity note. UNTRACKED on purpose: `claim-guard` rejects PRs that
> touch only non-module paths, and `loc-guard` counts `tools/ + examples/`.
> Do NOT commit this file alone. The durable record lives in claim `intent`
> fields + PR descriptions. This file keeps the wave direction across sessions.

## Decision

Priority: observability (dump / see / drive) before physics events, input
polling, or render unification. Poll result: option 1.

## State snapshot at decision time

- `main = 14676de` (PR #11 merged: m8 2D+3D showcases).
- `core = 1322 / 5000` lines. All modules free, no active claims.
- 1 open PR: #10 (`a/m2-shot-stale-cache`, green) — merge FIRST, m2 slice depends on it.
- Unstaged local deletion of `.agents/claims/m8.json` = pending release cleanup
  of the stale claim merged inside PR #11 (heartbeat 03:03Z, ttl 30min).

## Known gaps (from public/impl/selftest read)

- m2: headless `TakeScreenshot` is a baked 1x1 PNG stub (`impl/render.cpp:136-140`);
  `Clear` headless no-op; no `CompareImages`.
- m5: schema `stub v0`; `LoadJson` takes `version==0` only, field-order dependent
  (`TakeEntity:68-77`), escapes `\" \\` only; no `z` (marble uses dual `.xy/.xz`).
- m7: only `shot.sh`; no `dump/diff` helpers.
- `src/main.cpp:53-56`: `--replay` parsed but ignored (m6 not wired).

## Slices (one claim + one branch + one PR each, in order)

1. `m2` — `a/m2-shot-compare` — `CompareImages(pathA,pathB,fuzzPct)` stdlib-only
   (reuse logic from `test/showcase_pixels.cpp`), `Clear` headless clears DrawLog.
   `core/` added <= 200.
2. `m5` — `a/m5-schema-v1` — order-independent parser, full JSON escapes,
   `version 0|1`, optional `z` default 0. Selftest roundtrip + `spec/scene.schema.json` v1.
3. `m7` — `a/m7-dump-diff` — `dump.sh` (schema validation) + `diff.sh`
   (scene cmp + image compare). `extra/` added <= 400.
4. `m8` — `a/m8-obs-goldens` — wire showcases to m2 DrawLog where 1-liner,
   regen goldens with `--seed 42`, PR desc with before/after PNG + bench delta.

## No-wait rule (binding for this wave)

Never idle on GitHub CI. Local gates mirror CI, so:

1. `cmake -B build -G Ninja && cmake --build build`
2. `ctest --test-dir build --output-on-failure`
3. `./build/clank --headless --shot-after 60 --dump-scene /tmp/scene.json --seed 42`
4. `clang-format --dry-run -Werror` on touched files
5. Push branch early, open PR, then keep working the SAME branch
   (full-diff review, fixups) while CI runs. `heartbeat` every ~15min (ttl 30min).
6. Poll `gh pr checks` only between tasks. Fix red fast, merge when green,
   `release` claim, next slice. One agent = one module at a time (AGENTS.md §5).
