#!/usr/bin/env bash
# =============================================================================
# make-gif.sh — convert a screen recording (.mov/.mp4) into an optimized GIF
#               for the README, using ffmpeg's two-pass palette method.
#
# Usage:
#   ./docs/make-gif.sh <input.mov> [output.gif] [fps] [width] [start] [duration]
#
# Examples:
#   ./docs/make-gif.sh ~/Desktop/recording.mov
#   ./docs/make-gif.sh ~/Desktop/recording.mov docs/demo.gif 12 900
#   ./docs/make-gif.sh ~/Desktop/recording.mov docs/demo.gif 12 900 2 10   # trim: start 2s, 10s long
#
# Tips for a small, crisp GIF (GitHub READMEs autoplay GIFs):
#   * Keep it short (8-15s) and record just the app window/region.
#   * Lower fps (10-12) and width (800-900) if the file is too big.
#   * Aim for < 5 MB so the README loads fast.
# =============================================================================
set -euo pipefail

IN="${1:?usage: make-gif.sh <input.mov> [output.gif] [fps] [width] [start] [duration]}"
OUT="${2:-docs/demo.gif}"
FPS="${3:-12}"
WIDTH="${4:-900}"
START="${5:-}"     # optional: seconds to start at (e.g. 2)
DUR="${6:-}"       # optional: clip length in seconds (e.g. 10)

if ! command -v ffmpeg >/dev/null 2>&1; then
  echo "ffmpeg not found. Install it:  brew install ffmpeg" >&2
  exit 1
fi

TRIM=()
[ -n "$START" ] && TRIM+=(-ss "$START")
[ -n "$DUR" ]   && TRIM+=(-t "$DUR")

PALETTE="$(mktemp -t zzapi-palette).png"
FILTERS="fps=${FPS},scale=${WIDTH}:-1:flags=lanczos"

echo "→ pass 1/2: generating color palette …"
ffmpeg -y ${TRIM[@]+"${TRIM[@]}"} -i "$IN" -vf "${FILTERS},palettegen=stats_mode=diff" "$PALETTE"

echo "→ pass 2/2: encoding GIF …"
ffmpeg -y ${TRIM[@]+"${TRIM[@]}"} -i "$IN" -i "$PALETTE" \
  -lavfi "${FILTERS}[x];[x][1:v]paletteuse=dither=bayer:bayer_scale=3" "$OUT"

rm -f "$PALETTE"
echo "✓ wrote $OUT  ($(du -h "$OUT" | cut -f1))"
