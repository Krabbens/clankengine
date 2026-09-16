"""Verify the hierarchy sample's dump, screenshot, and replay channels."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile


binary, replay = map(lambda p: str(Path(p).resolve()), sys.argv[1:])
name = Path(binary).stem
assert name == "hierarchy"

with tempfile.TemporaryDirectory() as temp:
    root = Path(temp)

    def run(driven):
        command = [
            binary,
            "--headless",
            "--shot-after",
            "240",
            "--dump-scene",
            "scene.json",
            "--seed",
            "42",
        ]
        if driven:
            command += ["--replay", replay]
        result = subprocess.run(command, cwd=root, capture_output=True, text=True, check=True)
        assert result.stdout.splitlines() == ["hierarchy_frame240.png", "scene.json"]
        png = (root / "hierarchy_frame240.png").read_bytes()
        assert png.startswith(b"\x89PNG\r\n\x1a\n")
        assert int.from_bytes(png[16:20], "big") == 1280
        assert int.from_bytes(png[20:24], "big") == 720
        scene_text = (root / "scene.json").read_text()
        scene = json.loads(scene_text)
        assert {entity["id"]: entity.get("parent", -1) for entity in scene["entities"]} == {
            10: -1,
            11: 10,
            12: 11,
        }
        return scene_text, png, scene

    plain_text, plain_png, plain = run(False)
    driven_text, driven_png, driven = run(True)
    driven_text_2, driven_png_2, _ = run(True)

    assert driven_text == driven_text_2
    assert driven_png == driven_png_2
    assert plain_text != driven_text
    assert plain_png != driven_png
    assert abs(plain["entities"][0]["x"]) < 0.001
    assert abs(driven["entities"][0]["x"] + 1.0) < 0.001

print("hierarchy: dump, affine hierarchy, screenshot and replay PASS")
