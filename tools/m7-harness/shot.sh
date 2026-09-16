#!/usr/bin/env bash
# WHY: agents need dump/see/drive artifacts without display or extra deps.
set -u
if [ $# -ne 3 ]; then
  echo "usage: shot.sh <binary> <frames> <outdir>" >&2
  exit 2
fi
BIN="$1"
FRAMES="$2"
OUT="$3"
if [ ! -x "$BIN" ]; then
  echo "shot.sh: not executable: $BIN" >&2
  exit 1
fi
mkdir -p "$OUT"
SCENE="$OUT/scene.json"
"$BIN" --headless --shot-after "$FRAMES" --dump-scene "$SCENE" --seed 42 >"$OUT/stdout.txt" 2>"$OUT/stderr.txt"
CODE=$?
echo "exit=$CODE"
echo "--- stdout ---"
cat "$OUT/stdout.txt"
echo "--- artifacts ---"
ls -l "$OUT"
for f in $(cat "$OUT/stdout.txt"); do
  [ -e "$f" ] && ls -l "$f"
done
exit $CODE
