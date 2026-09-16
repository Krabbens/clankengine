#!/usr/bin/env bash
set -eu
if [ -z "${CLANK_WINDOWED_TEST:-}" ]; then
  echo "zelda-windowed-golden: SKIP (set CLANK_WINDOWED_TEST=1 to run)"
  exit 0
fi
binary=$1
compare=$2
golden=$3
replay=$4
frame=$5
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
cd "$tmp"
if command -v xvfb-run >/dev/null 2>&1; then
  xvfb-run -a "$binary" --shot-after "$frame" --dump-scene scene.json \
    --replay "$replay" --seed 42
else
  "$binary" --shot-after "$frame" --dump-scene scene.json --replay "$replay" --seed 42
fi
cp "zelda_frame${frame}.png" "/tmp/zelda_windowed_frame${frame}.png"
[ -f "$golden" ] || { echo "zelda-windowed-golden: missing $golden"; exit 1; }
"$compare" "zelda_frame${frame}.png" "$golden"
"$binary" --headless --shot-after "$frame" --dump-scene headless.json \
  --replay "$replay" --seed 42
cmp scene.json headless.json
