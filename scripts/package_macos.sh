#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SHA="$(git -C "$ROOT" rev-parse --short HEAD)"
"$ROOT/scripts/build_macos.sh" Release AudialSynth >/dev/null
OUT="$ROOT/dist/AudialSynth-macOS-$SHA"
rm -rf "$OUT" && mkdir -p "$OUT"
cp -R "$ROOT/standalone/builds/osx/build/Release/AudialSynth.app" "$OUT/"
cp -R "$ROOT/plugin/builds/osx/build/Release/AudialSynth.vst3" "$OUT/"
cp -R "$ROOT/plugin/builds/osx/build/Release/AudialSynth.component" "$OUT/"
cp "$ROOT/LICENSE" "$OUT/LICENSE"
printf 'Audial Synth is GPLv3. Source for this build: https://github.com/zfarrell13/sound2vital-gui/tree/%s\n' "$(git -C "$ROOT" rev-parse HEAD)" > "$OUT/SOURCE.txt"
(cd "$ROOT/dist" && rm -f "AudialSynth-macOS-$SHA.zip" && zip -qr "AudialSynth-macOS-$SHA.zip" "AudialSynth-macOS-$SHA")
ls -la "$ROOT/dist/AudialSynth-macOS-$SHA.zip"
