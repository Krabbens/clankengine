# clankengine — AGENTS.md

> Read this file first. It is normative for all agents and human contributors.
> Language of the repo (code, docs, commits, PRs) is English.

## 1. Mission

Commoditize the game loop: a tiny, hackable, agent-first game engine on top of
raylib (render/input/audio/window) + Box2D v3 (2D physics) + Box3D (3D physics).

An agent with no monitor, no mouse, and no editor must be able to
build → run → see → drive → verify a game without human help.

## 2. Philosophy (after tinygrad / George Hotz)

1. **Low line count is a guiding light, but NO code golf.** The goal is less
   complexity and more readability. Deleting `\n`s is a reject.
2. **The best part is no part.** 98% of software is workarounds for other
   abstractions. Delete requirements, delete code. Dead-code removal is a feature.
3. **Surface all complexity.** Thin wrappers over raylib/Box2D/Box3D. No hidden
   managers, no magic callbacks, no 5-level inheritance, no DI frameworks.
4. **RISC, not CISC.** Target ~20-30 functions in core public API, not 500.
   Prefer one explicit function with arguments over two 100-line special cases.
5. **Bitter Lesson over heuristics.** No hand-tuned heuristics without a bench.
   Every claimed speedup must ship numbers (frame time p50/p95/p99,
   physics ms, draw calls).
6. **Small PRs or no PRs.** A PR that looks complex, is a big diff, or adds
   lots of lines will not be reviewed. Refactor first so the feature is tiny.
7. **Bugfix only with a regression test. Refactor only as a clear win + replay.**

## 3. Stack (frozen)

* Language: **C++23** (subset, see §8). Compiler: clang-18+ / gcc-13+.
* Build: **CMake + Ninja**. Single preset: `cmake -B build -G Ninja && cmake --build build`.
* Render/input/audio: **raylib** (pinned tag in `ci-config.yml`).
* Physics 2D: **Box2D v3.1** (Erin Catto, C17, SIMD, deterministic).
* Physics 3D: **Box3D alpha** (Erin Catto, pinned commit, isolated behind own facade).
* Test: **CTest**. Lint: **clang-format + clang-tidy** with `-Wall -Wextra -Werror`.

### Dependency rule

You may add a dependency ONLY if you prove you cannot beat it performance-wise
with ~200 lines of your own code. New dep requires in the PR: what it replaces,
bench delta, LOC tradeoff, license. No Turing-complete scripting runtimes in `core/`.

Box3D is alpha: never include its headers outside `core/m4-physics3d/`.
Pin the commit hash; swapping it must not leak into other modules.

## 4. Repo layout and module ownership

```
core/m0-foundation/   # types, Log, Config, deterministic Clock/RNG, CLI flags
core/m1-loop/         # Run(), window lifecycle, headless, fixed timestep
core/m2-render/       # raylib facade, TakeScreenshot, screenshot compare
core/m3-physics2d/    # Box2D v3 facade (world/step/query only)
core/m4-physics3d/    # Box3D facade (isolated, pinned)
core/m5-scene/        # scene graph, scene JSON dump/load, .clk replay format
core/m6-input/        # input record/replay, scripted actions
tools/m7-harness/     # shot/dump/diff helpers, stdin/stdout proto
examples/ test/ spec/ # m8 samples, golden PNGs, schemas, benches
.agents/claims/       # live work claims, one file per module (see §5)
.github/workflows/    # CI gates (see §7)
tools/clank-claim     # claim CLI
```

* Each module has `public/` (frozen contract) + `impl/` (free).
* You may freely change only the `impl/` of your claimed module + its own tests.
* `public/` of another module is read-only. Changing it requires
  `exception.type = allow-cross` + RFC section in the PR.
* `core/m0-foundation/public/` is the most frozen: max one agent at a time.

Module → paths mapping is normative in `.github/ci-config.yml`.

## 5. Parallel work — claims (normative)

Goal: two agents NEVER work on the same module.

* Claim store: `.agents/claims/mX.json`, ONE file per module (`m0`..`m8`).
* Branch MUST match claim: `a/<m>-<topic>`, e.g. `a/m3-box2d-wrap`.
  Regex enforced by CI: `^a/m[0-8]-[a-z0-9-]{1,40}$`.
* Before starting: `git pull` then `tools/clank-claim list`. If `mX.json`
  exists and fresh (`now - heartbeat_utc < ttl_min`, default 30), pick another module.
* Claim JSON schema (all fields required, `exception` may be null):

```json
{
  "module": "m3",
  "agent": "agent-02",
  "branch": "a/m3-box2d-wrap",
  "started_utc": "2026-09-16T10:00:00Z",
  "heartbeat_utc": "2026-09-16T10:15:00Z",
  "ttl_min": 30,
  "intent": "Wrap b2WorldId create/destroy + fixed step",
  "paths": ["core/m3-physics2d/"],
  "exception": null
}
```

* `exception` (optional) MUST be:
  `{"type":"allow-big|allow-cross|allow-growth","reason":"...","extra_bench":"..."}`
  Empty reason = CI fail.
* Lifecycle:
  1. `tools/clank-claim claim m3 --agent agent-02 --topic box2d-wrap --intent "Wrap Box2D world step"`
  2. work (small commits, see §6)
  3. `tools/clank-claim heartbeat m3 --agent agent-02` every ~15 min
  4. open PR, pass CI
  5. `tools/clank-claim release m3 --agent agent-02` (deletes the file)
* Stale claims (`now - heartbeat > ttl`) are labelled `stale` by CI cron.
  Takeover requires `claim --force` + link to the stale PR.
* One agent = one module at a time. No exceptions.

### 5.1 Shared checkout safety (normative)

Codex tasks can run concurrently against this repository. The repository root
is not an agent workspace: never run `git switch`, `git checkout`, `git reset`,
or a long-lived build from a checkout another task may use. Those operations
move the shared HEAD and can silently destroy another agent's context.

Each agent MUST create and use a dedicated worktree before claiming a module:

```bash
git fetch origin main
git worktree add ../clankengine-<agent>-<topic> -b a/m3-box2d-wrap origin/main
cd ../clankengine-<agent>-<topic>
tools/clank-claim claim m3 --agent <agent> --topic box2d-wrap --intent "Wrap Box2D world step"
```

`tools/clank-claim` keeps an untracked mutex in the shared `.git` directory
and mirrors the CI claim file into the active worktree. Therefore claims are
visible immediately across worktrees, even before the claim commit is pushed.
All build, test, commit, and push commands MUST run from the dedicated
worktree. Remove it only after the PR is merged and the claim is released.

## 6. Agent loop (every task)

1. Fetch `origin/main`, create a dedicated worktree, then run
   `tools/clank-claim list` from that worktree.
2. Claim one free module in that worktree.
3. Read the module `public/` headers + its tests. No drive-by edits elsewhere.
4. Minimal change (§7 limits). Build + test locally.
5. Visual verify (§9): `dump-scene` + `shot-after` + diff before/after.
6. Push the branch early, open a PR, get green CI + review (§6.1), merge,
   release claim, and remove the dedicated worktree.

### 6.1 Wave flow (every wave ships via PRs, no exceptions)

1. One claim + one branch `a/<m>-<topic>` per module slice. Work lands only
   on its branch.
2. Open the PR as soon as CI can run on it. Description uses the template below.
3. CI (`claim-guard`, `loc-guard`, `build-test`, `lint`) must be green.
   Branch protection blocks merge otherwise. No bypass, no human exception.
4. Review: the brain reads the FULL diff (line budget §7, style §8, tests,
   replay/golden evidence) and posts findings as PR review comments.
   Fix, re-push, re-review until clean.
5. Merge with `gh pr merge --merge` (one merge commit per module per wave),
   delete the branch, `tools/clank-claim release <mX>`.
6. Direct pushes to `main` are forbidden by branch protection.
   `main` moves only via reviewed, green PRs.

PR description MUST contain:

```text
module: m3
claim-file: .agents/claims/m3.json
intent: one line
before/after PNG: attached or path in test/golden/
bench delta: physics ms / frame time before → after
```

Big diff / complex look / missing bench = close.

## 7. Hard gates (enforced by CI, no human bypass)

All limits are defined once in `.github/ci-config.yml`. CI fails hard on breach.

| Gate | Limit | Counter |
|---|---|---|
| `core_added` | <= 200 added lines per PR | `git diff origin/main...HEAD --numstat -- core/` (added col) |
| `extra_added` | <= 400 added lines per PR (`extra/ tools/ examples/`) | same, `numstat` added col |
| `core_total` | <= 5000 lines total in `core/` | `git ls-files core/ \| xargs wc -l` |
| duplicate module | 0 second open PR on same `mX` | `gh pr list --state open` |
| branch name | MUST match `^a/m[0-8]-...` | claim-guard |
| claim fresh | file exists, `branch == head`, heartbeat fresh | claim-guard |

Excluded from limits: `test/`, `*.png`, `.agents/claims/`.

`exception.type=allow-big` raises the PR caps to hard-cap 400 core / 800 extra
and still requires `reason + bench`. Above hard-cap: split into 2 PRs, no debate.
`allow-cross` permits touching a second module's `public/` with RFC.
`allow-growth` permits raising `core_total` with justification.
Every agent may use exceptions, which is why they are strict and mechanical.

## 8. C++ style

* Subset of C++23: `std::expected`, `std::span`, `std::optional`, concepts where
  they delete code. No exceptions in hot path, no RTTI-heavy hierarchies.
* Plain structs + free functions + explicit context objects.
  Match raylib naming where wrapping raylib, Box2D naming where wrapping physics.
* `clang-format` clean, `clang-tidy` clean, `-Wall -Wextra -Werror`.
* No `using namespace` in headers. Headers self-contained (`#pragma once`).
* Comments explain WHY, not WHAT. No commented-out code.

## 9. Agent observability (every sample/game MUST implement)

Headless alone is not enough. All three channels are mandatory:

1. **dump (text):** `--dump-scene out.json` — full scene graph as JSON per
   `spec/scene.schema.json`. Must work headless, deterministic with `--seed`.
2. **see (pixels):** `--shot-after N` + `TakeScreenshot()` — deterministic
   framebuffer dump. Fixed timestep + seeded RNG in test mode. Golden images
   live in `test/golden/<sample>_frame<N>.png`, compared with fuzz 2%.
3. **drive (replay):** `--replay demo.clk --seed 42`, pause/step frame.
   Reuse Box2D/Box3D determinism + recording where possible; do not invent
   a second physics replay.

Flags style: `clank --headless --shot-after 120 --dump-scene out.json --replay demo.clk --seed 42`.
Logs go to stderr; machine-readable results (JSON paths, PNG paths) go to stdout.

## 10. Checks (local, before push)

```bash
cmake -B build -G Ninja && cmake --build build
ctest --test-dir build --output-on-failure
./build/clank --headless --shot-after 60 --dump-scene /tmp/scene.json
clang-format --dry-run -Werror $(git ls-files '*.hpp' '*.cpp')
```

MUST NOT:

* touch another module's `public/` without `allow-cross`
* add a dep without bench + LOC tradeoff
* ship an editor-only feature (no headless path = reject)
* commit golden PNGs generated with RNG unseeded or variable timestep
* amend other agent's commits; always a new commit
* whitespace-only or docs-only PRs from unknown contributors (close, like tinygrad)
