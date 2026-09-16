"""Verify the core CLI's drive channel changes the dumped scene."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile


binary, replay = map(lambda p: str(Path(p).resolve()), sys.argv[1:])
with tempfile.TemporaryDirectory() as temp:
    root = Path(temp)

    def run(name, driven):
        command = [binary, "--headless", "--dump-scene", name, "--seed", "42"]
        if driven:
            command += ["--replay", replay]
        result = subprocess.run(command, cwd=root, capture_output=True, text=True, check=True)
        assert result.stdout.splitlines() == [name]
        return json.loads((root / name).read_text())

    plain = run("plain.json", False)
    driven = run("driven.json", True)
    assert plain != driven, "replay must affect the core scene"
    assert abs(driven["entities"][0]["x"] - 0.5) < 0.001
    assert abs(driven["entities"][0]["y"]) < 0.001

print("clank: core replay PASS")
