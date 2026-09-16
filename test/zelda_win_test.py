"""Prove the zelda sample is beatable: a fixed replay collects 5 rupees and exits."""
import json
import subprocess
import sys
import tempfile
from pathlib import Path

binary, replay = map(lambda p: str(Path(p).resolve()), sys.argv[1:])
name = Path(binary).name
assert name == "zelda"
with tempfile.TemporaryDirectory() as temp:
    root = Path(temp)

    def run(frames=600, seed=42):
        out = root / "scene.json"
        command = [binary, "--headless", "--shot-after", str(frames),
                   "--dump-scene", "scene.json", "--seed", str(seed),
                   "--replay", replay]
        result = subprocess.run(command, cwd=root, capture_output=True, text=True, check=True)
        assert result.stdout.splitlines() == [f"zelda_frame{frames}.png", "scene.json"]
        return out.read_text()

    first = run()
    assert run() == first, "win replay must be byte-identical"
    scene = json.loads(first)
    state = next(e for e in scene["entities"] if e["name"] == "state")
    assert state["y"] == 5, f"all rupees collected, got {state['y']}"
    assert state["x"] >= 1, f"player alive, got hp {state['x']}"
    assert state["angle"] == 1, f"exit reached, got status {state['angle']}"
    assert not [e for e in scene["entities"] if e["name"] == "rupee"], "no rupees left"
print("zelda-win: 5 rupees, exit reached, deterministic PASS")
