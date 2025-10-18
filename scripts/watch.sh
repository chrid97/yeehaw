#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"
cd "$PROJECT_ROOT"

if ! command -v watchexec >/dev/null 2>&1; then
  echo "❌ Error: watchexec not found. Please install it"
  exit 1
fi

scripts/run.sh &
GAME_PID=$!

watchexec -r -e c,h -- bash scripts/build_game.sh &
WATCH_PID=$!

cleanup() {
  if [[ "${CLEANED_UP:-false}" == true ]]; then
    return
  fi
  CLEANED_UP=true

  echo "🛑 Stopping game and watcher..."
  kill "$GAME_PID" 2>/dev/null || true
  kill "$WATCH_PID" 2>/dev/null || true
  wait "$GAME_PID" 2>/dev/null || true
  wait "$WATCH_PID" 2>/dev/null || true
}

trap cleanup EXIT INT

if wait -n 2>/dev/null; then
  wait -n "$GAME_PID" "$WATCH_PID"
else
  # macOS fallback
  while true; do
    if ! kill -0 "$GAME_PID" 2>/dev/null; then
      break
    fi
    if ! kill -0 "$WATCH_PID" 2>/dev/null; then
      break
    fi
    sleep 0.5
  done
fi

cleanup
