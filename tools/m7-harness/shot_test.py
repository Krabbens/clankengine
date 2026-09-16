#!/usr/bin/env python3
"""Stdlib-only tests for the portable shot runner."""

from __future__ import annotations

import os
import stat
import subprocess
import sys
import tempfile
import textwrap
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent
SHOT = ROOT / "shot.py"


FAKE_BINARY = textwrap.dedent(
    r"""
    import argparse
    import os
    from pathlib import Path
    import sys

    parser = argparse.ArgumentParser()
    parser.add_argument("--headless", action="store_true")
    parser.add_argument("--shot-after", required=True)
    parser.add_argument("--dump-scene", required=True)
    parser.add_argument("--seed", required=True)
    args = parser.parse_args()
    dump = Path(args.dump_scene)
    (Path.cwd() / "received-args.txt").write_text(
        " ".join(sys.argv[1:]), encoding="utf-8")
    if os.environ.get("FAKE_WRITE_SCENE") == "1":
        dump.write_text('{"version": 0, "seed": 42, "entities": []}', encoding="utf-8")
    if os.environ.get("FAKE_WRITE_ARTIFACT") == "1":
        source = dump.parent.parent / "child artifact.txt"
        source.write_text("artifact", encoding="utf-8")
        print(source)
    if os.environ.get("FAKE_MISSING_ARTIFACT") == "1":
        print(dump.parent / "missing artifact.png")
    print("INFO: FILEIO: [/tmp/not-a-path.png] Image exported successfully")
    print(dump)
    print("fake stderr", file=sys.stderr)
    sys.exit(int(os.environ.get("FAKE_EXIT", "0")))
    """
).lstrip()


class ShotTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory(prefix="m7-shot-")
        self.root = Path(self.temp.name) / "Windows path with spaces"
        self.root.mkdir()
        self.binary = self._make_binary()

    def tearDown(self) -> None:
        self.temp.cleanup()

    def _make_binary(self) -> Path:
        script = self.root / "fake binary.py"
        script.write_text(FAKE_BINARY, encoding="utf-8")
        if os.name == "nt":
            binary = self.root / "fake binary.cmd"
            binary.write_text(
                f'@echo off\r\n"{sys.executable}" "{script}" %*\r\n', encoding="utf-8"
            )
        else:
            binary = self.root / "fake binary"
            binary.write_text(
                f'#!/bin/sh\nexec "{sys.executable}" "{script}" "$@"\n',
                encoding="utf-8",
            )
            binary.chmod(binary.stat().st_mode | stat.S_IXUSR)
        return binary

    def _run(self, outdir: Path, **env: str) -> subprocess.CompletedProcess[str]:
        child_env = os.environ.copy()
        child_env.update(env)
        return subprocess.run(
            [sys.executable, str(SHOT), str(self.binary), "17", str(outdir)],
            capture_output=True,
            check=False,
            text=True,
            encoding="utf-8",
            env=child_env,
        )

    def test_arguments(self) -> None:
        result = subprocess.run(
            [sys.executable, str(SHOT)], capture_output=True, check=False, text=True
        )
        self.assertEqual(result.returncode, 2)
        self.assertIn("usage: shot.py", result.stderr)

    def test_windows_paths_and_artifacts(self) -> None:
        outdir = self.root / "out dir"
        result = self._run(outdir, FAKE_WRITE_SCENE="1", FAKE_WRITE_ARTIFACT="1")
        self.assertEqual(result.returncode, 0)
        self.assertTrue((outdir / "scene.json").is_file())
        self.assertTrue((outdir / "child artifact.txt").is_file())
        self.assertTrue((outdir / "stdout.txt").is_file())
        self.assertTrue((outdir / "stderr.txt").is_file())
        self.assertNotIn("missing artifact: " + str(outdir / "INFO:"), result.stdout)
        args = (outdir / "received-args.txt").read_text(encoding="utf-8")
        self.assertIn("--headless", args)
        self.assertIn("--shot-after 17", args)
        self.assertIn("--seed 42", args)
        self.assertIn(str(outdir / "scene.json"), args)

    def test_exit_code_and_missing_artifacts(self) -> None:
        outdir = self.root / "missing out dir"
        result = self._run(outdir, FAKE_EXIT="7", FAKE_MISSING_ARTIFACT="1")
        self.assertEqual(result.returncode, 7)
        self.assertIn("missing artifact:", result.stdout)
        self.assertEqual(result.stdout.count("missing artifact:"), 2)
        self.assertTrue((outdir / "stdout.txt").is_file())
        self.assertTrue((outdir / "stderr.txt").is_file())
        self.assertIn(
            "fake stderr", (outdir / "stderr.txt").read_text(encoding="utf-8")
        )


if __name__ == "__main__":
    unittest.main()
