#!/usr/bin/env bash
# AUDIAL_ENV (env var, default "prod") is passed through to scripts/build_macos.sh; the zip
# name keeps its existing form for prod and gets "-<env>" appended otherwise (e.g. "-dev").
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SHA="$(git -C "$ROOT" rev-parse --short HEAD)"
AUDIAL_ENV="${AUDIAL_ENV:-prod}"
AUDIAL_ENV="$AUDIAL_ENV" "$ROOT/scripts/build_macos.sh" Release AudialSynth >/dev/null
SUFFIX=""
[ "$AUDIAL_ENV" != "prod" ] && SUFFIX="-$AUDIAL_ENV"
NAME="AudialSynth-macOS-$SHA$SUFFIX"
OUT="$ROOT/dist/$NAME"
rm -rf "$OUT" && mkdir -p "$OUT"
cp -R "$ROOT/standalone/builds/osx/build/Release/AudialSynth.app" "$OUT/"
cp -R "$ROOT/plugin/builds/osx/build/Release/AudialSynth.vst3" "$OUT/"
cp -R "$ROOT/plugin/builds/osx/build/Release/AudialSynth.component" "$OUT/"
cp "$ROOT/LICENSE" "$OUT/LICENSE"
printf 'Audial Synth is GPLv3. Source for this build: https://github.com/AudialAI/sound2vital-gui/tree/%s\n' "$(git -C "$ROOT" rev-parse HEAD)" > "$OUT/SOURCE.txt"
(cd "$ROOT/dist" && rm -f "$NAME.zip" && zip -qr "$NAME.zip" "$NAME")
ls -la "$ROOT/dist/$NAME.zip"
