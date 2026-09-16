#!/usr/bin/env bash
# WHY: real-pixel golden with no physical display (CI runs this under xvfb-run).
# Skips silently unless CLANK_WINDOWED_TEST=1, so plain `ctest` never pops windows.
set -u
if [ -z "${CLANK_WINDOWED_TEST:-}" ]; then
  echo "windowed-golden: SKIP (set CLANK_WINDOWED_TEST=1 to run)"
  exit 0
fi
BIN="${1:?usage: windowed_golden.sh <clank-bin> <golden-png>}"
GOLDEN="${2:?usage: windowed_golden.sh <clank-bin> <golden-png>}"
OUT="/tmp/clank_frame60.png"
rm -f "$OUT"
if command -v xvfb-run >/dev/null 2>&1; then
  RUN="xvfb-run -a"
else
  RUN=""
fi
# shellcheck disable=SC2086
$RUN "$BIN" --shot-after 60 --dump-scene /tmp/wscene.json --seed 42 || exit 1
[ -f "$OUT" ] || { echo "windowed-golden: no screenshot at $OUT"; exit 1; }
[ -f "$GOLDEN" ] || { echo "windowed-golden: missing golden $GOLDEN (inspect $OUT and commit it)"; exit 1; }
cmake -E compare_files "$OUT" "$GOLDEN"
