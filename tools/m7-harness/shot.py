#!/usr/bin/env python3
"""Run a deterministic headless shot and collect its observability artifacts.

The child process writes result paths to stdout.  Informational lines are
allowed there too, so only standalone-looking paths are collected.  A child
may use an absolute path outside the output directory (the current clank
binary uses /tmp for screenshots), so reported files are copied into it.
"""

from __future__ import annotations

import ntpath
import os
import shutil
import subprocess
import sys
from pathlib import Path


def _usage() -> str:
    return "usage: shot.py <binary> <frames> <outdir>"


def _child_path(line: str, outdir: Path) -> Path:
    path = Path(line)
    return path if path.is_absolute() else outdir / path


def _looks_like_artifact(line: str) -> bool:
    """Recognize a missing path without treating child log lines as paths."""
    if line.startswith(("INFO:", "WARNING:", "ERROR:")):
        return False
    if ntpath.isabs(line) or "/" in line or "\\" in line:
        return True
    return Path(line).suffix.lower() in {
        ".bmp",
        ".clk",
        ".jpeg",
        ".jpg",
        ".json",
        ".png",
    }


def _reported_paths(stdout: str, outdir: Path) -> list[Path]:
    paths = []
    for raw_line in stdout.splitlines():
        line = raw_line.strip()
        if not line:
            continue
        path = _child_path(line, outdir)
        if path.is_file() or _looks_like_artifact(line):
            paths.append(path)
    return paths


def _same_path(left: Path, right: Path) -> bool:
    return left.resolve(strict=False) == right.resolve(strict=False)


def _collect_artifacts(
    stdout: str, scene: Path, outdir: Path
) -> tuple[list[Path], list[str]]:
    """Copy child-reported files into outdir and return (copied, missing)."""
    copied: list[Path] = []
    missing: list[str] = []
    reported = [scene, *_reported_paths(stdout, outdir)]
    seen: set[Path] = set()
    for source in reported:
        key = source.resolve(strict=False)
        if key in seen:
            continue
        seen.add(key)
        if not source.is_file():
            missing.append(str(source))
            continue
        destination = outdir / source.name
        if not _same_path(source, destination):
            try:
                shutil.copy2(source, destination)
            except OSError as error:
                missing.append(f"{source} ({error})")
        copied.append(destination)
    return copied, missing


def run(binary: str, frames: str, outdir: str) -> int:
    """Run the child and return its exit code."""
    binary_path = Path(binary).expanduser().resolve(strict=False)
    output_dir = Path(outdir).expanduser().resolve(strict=False)
    if not binary_path.is_file() or (
        os.name != "nt" and not os.access(binary_path, os.X_OK)
    ):
        print(f"shot.py: not executable: {binary}", file=sys.stderr)
        return 1
    try:
        output_dir.mkdir(parents=True, exist_ok=True)
    except OSError as error:
        print(
            f"shot.py: cannot create output directory {outdir}: {error}",
            file=sys.stderr,
        )
        return 1

    scene = output_dir / "scene.json"
    command = [
        str(binary_path),
        "--headless",
        "--shot-after",
        str(frames),
        "--dump-scene",
        str(scene),
        "--seed",
        "42",
    ]
    try:
        result = subprocess.run(
            command,
            cwd=output_dir,
            capture_output=True,
            check=False,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
    except OSError as error:
        print(f"shot.py: cannot run {binary}: {error}", file=sys.stderr)
        return 1

    (output_dir / "stdout.txt").write_text(result.stdout, encoding="utf-8")
    (output_dir / "stderr.txt").write_text(result.stderr, encoding="utf-8")
    copied, missing = _collect_artifacts(result.stdout, scene, output_dir)

    print(f"exit={result.returncode}")
    print("--- stdout ---")
    sys.stdout.write(result.stdout)
    if result.stdout and not result.stdout.endswith("\n"):
        print()
    print("--- artifacts ---")
    for path in copied:
        print(path)
    for path in missing:
        print(f"missing artifact: {path}")
    return result.returncode


def main(argv: list[str] | None = None) -> int:
    args = sys.argv[1:] if argv is None else argv
    if len(args) != 3:
        print(_usage(), file=sys.stderr)
        return 2
    return run(*args)


if __name__ == "__main__":
    sys.exit(main())
