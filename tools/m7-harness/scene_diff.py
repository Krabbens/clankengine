#!/usr/bin/env python3
"""Semantic diff of two clank scene dumps (shape in spec/scene.schema.json).

Byte compare proves determinism; this proves behavioral sameness instead:
same seeds, same entity ids/names, numeric fields within --eps. Exit 0 when
identical, 1 with a report on stdout when different, 2 on usage/load errors.
"""
import json
import math
import sys

NUMERIC = ("x", "y", "angle", "sx", "sy", "z")


def load(path):
    try:
        with open(path) as handle:
            scene = json.load(handle)
    except (OSError, ValueError) as err:
        return None, f"{path}: cannot load ({err})"
    if not isinstance(scene, dict) or not isinstance(scene.get("entities"), list):
        return None, f"{path}: not a scene dump"
    return scene, ""


def number(value):
    return isinstance(value, (int, float)) and not isinstance(value, bool) \
        and math.isfinite(value)


def compare_entities(old, new, eps):
    lines = []
    for key in ("id", "name"):
        if old.get(key) != new.get(key):
            lines.append(f"id {old.get('id')}: {key} {old.get(key)!r} -> {new.get(key)!r}")
    changes = []
    for key in NUMERIC:
        aval, bval = old.get(key, 0.0), new.get(key, 0.0)
        if not number(aval) or not number(bval) or abs(aval - bval) > eps:
            changes.append(f"{key}: {aval!r}->{bval!r}")
    if changes:
        lines.append(f"id {old.get('id')}: moved " + ", ".join(changes))
    return lines


def compare(a_path, b_path, eps):
    a_scene, a_err = load(a_path)
    b_scene, b_err = load(b_path)
    if a_err or b_err:
        return [a_err or b_err], 2
    lines = []
    if a_scene.get("version") != b_scene.get("version"):
        lines.append(f"version {a_scene.get('version')!r} -> {b_scene.get('version')!r}")
    if a_scene.get("seed") != b_scene.get("seed"):
        lines.append(f"seed {a_scene.get('seed')!r} -> {b_scene.get('seed')!r}")
    a_map = {e["id"]: e for e in a_scene["entities"] if isinstance(e, dict) and "id" in e}
    b_map = {e["id"]: e for e in b_scene["entities"] if isinstance(e, dict) and "id" in e}
    for gone in sorted(set(a_map) - set(b_map)):
        lines.append(f"removed id {gone} ({a_map[gone].get('name')})")
    for fresh in sorted(set(b_map) - set(a_map)):
        lines.append(f"added id {fresh} ({b_map[fresh].get('name')})")
    for same in sorted(set(a_map) & set(b_map)):
        lines.extend(compare_entities(a_map[same], b_map[same], eps))
    return lines, 1 if lines else 0


def selftest():
    def check(a, b, want_code, want_text="", eps=1e-6):
        import tempfile
        from pathlib import Path
        with tempfile.TemporaryDirectory() as temp:
            pa, pb = Path(temp) / "a.json", Path(temp) / "b.json"
            pa.write_text(a if isinstance(a, str) else json.dumps(a))
            pb.write_text(b if isinstance(b, str) else json.dumps(b))
            lines, code = compare(str(pa), str(pb), eps)
            assert code == want_code, f"{a} vs {b}: code {code}"
            assert want_text in "\n".join(lines), f"{a} vs {b}: missing {want_text!r}"

    base = {"version": 0, "seed": 42, "entities": [
        {"id": 0, "name": "player", "x": 1.0, "y": 2.0, "angle": 0, "sx": 1, "sy": 1}]}
    check(base, base, 0)
    moved = json.loads(json.dumps(base))
    moved["entities"][0]["x"] = 1.01
    check(base, moved, 1, "moved")
    added = json.loads(json.dumps(base))
    added["entities"].append({"id": 1, "name": "rupee", "x": 0, "y": 0, "angle": 0,
                              "sx": 1, "sy": 1})
    check(base, added, 1, "added id 1")
    check(added, base, 1, "removed id 1")
    reseeded = json.loads(json.dumps(base))
    reseeded["seed"] = 43
    check(base, reseeded, 1, "seed")
    with_zero = json.loads(json.dumps(base))
    with_zero["entities"][0]["z"] = 0.0
    check(base, with_zero, 0)
    check("nope", base, 2)
    print("scene-diff selftest PASS")
    return 0


def main(argv):
    if argv == ["--selftest"]:
        return selftest()
    args = [a for a in argv if not a.startswith("--eps=")]
    eps_args = [a for a in argv if a.startswith("--eps=")]
    if len(args) != 2:
        print("usage: scene_diff.py A.json B.json [--eps=1e-6] | --selftest", file=sys.stderr)
        return 2
    try:
        eps = float(eps_args[0].split("=", 1)[1]) if eps_args else 1e-6
    except ValueError:
        print("scene_diff.py: bad --eps value", file=sys.stderr)
        return 2
    lines, code = compare(args[0], args[1], eps)
    for line in lines:
        print(line)
    return code


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
