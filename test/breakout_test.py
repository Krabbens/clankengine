"""Prove breakout works: replayed launches break bricks deterministically."""
import json
import math
from pathlib import Path
import subprocess
import sys
import tempfile

binary, replay = map(lambda p: str(Path(p).resolve()), sys.argv[1:])
assert Path(binary).name == "breakout"
with tempfile.TemporaryDirectory() as temp:
    root = Path(temp)

    def run(frames, seed=42, drive=False):
        command = [binary, "--headless", "--shot-after", str(frames),
                   "--dump-scene", "scene.json", "--seed", str(seed)]
        if drive:
            command += ["--replay", replay]
        result = subprocess.run(command, cwd=root, capture_output=True, text=True, check=True)
        assert result.stdout.splitlines() == [f"breakout_frame{frames}.png", "scene.json"]
        scene = json.loads((root / "scene.json").read_text())
        assert scene["seed"] == seed
        ids = [e["id"] for e in scene["entities"]]
        assert len(ids) == len(set(ids))
        for entity in scene["entities"]:
            for key in ("x", "y", "angle", "sx", "sy"):
                assert math.isfinite(entity[key])
        return scene["entities"]

    def bricks(entities):
        return [e for e in entities if e["name"] == "brick"]

    assert len(bricks(run(0))) == 24, "full wall of bricks"
    driven = run(240, drive=True)
    assert json.dumps(driven, sort_keys=True) == json.dumps(run(240, drive=True), sort_keys=True), \
        "replay must be byte-identical"
    settled = run(240)
    assert len(bricks(driven)) < len(bricks(settled)), "replay must break bricks"
    assert len(bricks(settled)) == 24, "no balls, no breaks"
    for e in driven + settled:
        if e["name"] == "ball":
            assert abs(e["x"]) < 7 and -3 < e["y"] < 13, f"ball in bounds: {e}"
    for args in (["--replay", "missing.clk"], ["--unknown"]):
        result = subprocess.run([binary, "--headless", *args], cwd=root, capture_output=True)
        assert result.returncode != 0, f"must fail: {args}"
print("breakout: bricks, replay determinism, bounds and errors PASS")
