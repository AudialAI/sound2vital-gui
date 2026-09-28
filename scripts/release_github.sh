#!/usr/bin/env bash
# Cut a GitHub release for Audial Synth.
#
# Usage:
#   scripts/release_github.sh <tag> [--dry-run|--publish]
#
# Default (or --dry-run) only prints the `gh release create` command it would run.
# --publish actually builds (prod) and runs it.
#
# Requires: working tree clean, on `main`, `gh` authenticated with access to
# AudialAI/sound2vital-gui, and a docs/release-notes/<tag>.md file.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

TAG="${1:-}"
MODE="dry-run"
for arg in "${@:2}"; do
  case "$arg" in
    --dry-run) MODE="dry-run" ;;
    --publish) MODE="publish" ;;
    *) echo "Unknown argument: $arg" >&2; exit 1 ;;
  esac
done

if [ -z "$TAG" ]; then
  echo "Usage: scripts/release_github.sh <tag> [--dry-run|--publish]" >&2
  exit 1
fi

NOTES="$ROOT/docs/release-notes/$TAG.md"
if [ ! -f "$NOTES" ]; then
  echo "Missing release notes: $NOTES" >&2
  exit 1
fi

BRANCH="$(git -C "$ROOT" rev-parse --abbrev-ref HEAD)"
if [ "$BRANCH" != "main" ]; then
  echo "Refusing to release from branch '$BRANCH' (must be on main)." >&2
  exit 1
fi

if [ -n "$(git -C "$ROOT" status --porcelain)" ]; then
  echo "Working tree is not clean; commit or stash changes first." >&2
  exit 1
fi

echo "Building and packaging (AUDIAL_ENV=prod)..."
AUDIAL_ENV=prod "$ROOT/scripts/package_macos.sh"

SHA="$(git -C "$ROOT" rev-parse --short HEAD)"
SHA_ZIP="$ROOT/dist/AudialSynth-macOS-$SHA.zip"
STABLE_ZIP="$ROOT/dist/AudialSynth-macOS.zip"
STABLE_SHA="$ROOT/dist/AudialSynth-macOS.zip.sha256"

for f in "$SHA_ZIP" "$STABLE_ZIP" "$STABLE_SHA"; do
  if [ ! -f "$f" ]; then
    echo "Expected packaged asset missing: $f" >&2
    exit 1
  fi
done

CMD=(gh release create "$TAG" --repo AudialAI/sound2vital-gui
     --title "Audial Synth $TAG" --notes-file "$NOTES"
     "$STABLE_ZIP" "$STABLE_SHA" "$SHA_ZIP")

if [ "$MODE" = "dry-run" ]; then
  echo "--dry-run (default): would run:"
  printf '  %q' "${CMD[@]}"
  echo
  exit 0
fi

echo "--publish: running:"
printf '  %q' "${CMD[@]}"
echo
"${CMD[@]}"
