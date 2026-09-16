#!/usr/bin/env bash
set -eu
if [ -z "${CLANK_WINDOWED_TEST:-}" ]; then
  echo "showcase-golden: SKIP (set CLANK_WINDOWED_TEST=1 to run)"
  exit 0
fi
binary=$1
compare=$2
golden=$3
replay=$4
frame=$5
name=$(basename "$binary")
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
cd "$tmp"
if command -v xvfb-run >/dev/null 2>&1; then
  xvfb-run -a "$binary" --shot-after "$frame" --dump-scene scene.json --replay "$replay" --seed 42
else
  "$binary" --shot-after "$frame" --dump-scene scene.json --replay "$replay" --seed 42
fi
"$compare" "${name}_frame${frame}.png" "$golden"
cp scene.json windowed.json
"$binary" --headless --shot-after "$frame" --dump-scene scene.json --replay "$replay" --seed 42
cmp scene.json windowed.json
