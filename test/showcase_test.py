"""Exercise observable behavior through the same CLI used by an agent."""
import json
import math
from pathlib import Path
import subprocess
import sys
import tempfile

binary, replay = map(lambda p: str(Path(p).resolve()), sys.argv[1:])
name = Path(binary).name
with tempfile.TemporaryDirectory() as temp:
    root = Path(temp)

    def run(frames, seed=42, drive=False):
        command = [binary, "--headless", "--shot-after", str(frames),
                   "--dump-scene", "scene.json", "--seed", str(seed)]
        if drive:
            command += ["--replay", replay]
        result = subprocess.run(command, cwd=root, capture_output=True, text=True, check=True)
        assert result.stdout.splitlines() == [f"{name}_frame{frames}.png", "scene.json"]
        assert (root / f"{name}_frame{frames}.png").read_bytes().startswith(b"\x89PNG\r\n\x1a\n")
        text = (root / "scene.json").read_text()
        scene = json.loads(text)
        assert scene["seed"] == seed
        ids = [e["id"] for e in scene["entities"]]
        assert len(ids) == len(set(ids))
        for entity in scene["entities"]:
            for key in ("x", "y", "angle", "sx", "sy"):
                assert math.isfinite(entity[key])
        return text, scene["entities"]

    initial_text, initial = run(0)
    assert initial != run(0, seed=43)[1], "seed must change body positions"
    driven_text, driven = run(240, drive=True)
    assert driven_text == run(240, drive=True)[0], "replay must be byte-identical"
    settled_text, settled = run(240)
    assert driven_text != settled_text, "replay must affect simulation"
    projections = 2 if name == "marble_tray" else 1
    assert len(driven) == len(settled) + 2 * projections, "two replay drops"
    assert abs(driven[-1]["x"] + 2.5) < .001, "replay steers the emitter"
    initial_bodies = {e["id"]: e for e in initial if e["name"].startswith("obstacle")}
    for e in settled:
        if e["id"] in initial_bodies:
            assert e == initial_bodies[e["id"]], "static geometry must not move"
    balls = [e for e in settled if e["name"] in ("ball", "sphere.xy")]
    assert len(balls) == (30 if projections == 2 else 24)
    assert all(.1 < e["y"] < 4 for e in balls), "gravity and tray contacts must settle balls"
    assert all(abs(e["x"]) < 7 for e in balls), "walls must contain balls"
    if projections == 2:
        assert all(abs(e["y"]) < 5 for e in settled if e["name"] == "sphere.xz")
    for args in (["--replay", "missing.clk"], ["--unknown"],
                 ["--shot-after", "0", "--dump-scene", "missing/scene.json"]):
        result = subprocess.run([binary, "--headless", *args], cwd=root, capture_output=True)
        assert result.returncode != 0, f"must fail: {args}"
print(f"{name}: seed, replay, gravity, contacts, bounds and errors PASS")
