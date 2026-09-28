#!/usr/bin/env bash
# AUDIAL_ENV (env var, default "prod") is passed through to scripts/build_macos.sh; the zip
# name keeps its existing form for prod and gets "-<env>" appended otherwise (e.g. "-dev").
#
# Signing / notarization (optional; without these, bundles stay ad-hoc signed, same as
# scripts/build_macos.sh produces today):
#   AUDIAL_SIGN_IDENTITY   - a "Developer ID Application: ..." identity. When set, each
#                            bundle (.app, .vst3, .component) is codesigned with a hardened
#                            runtime and secure timestamp before zipping.
#   AUDIAL_NOTARY_PROFILE  - a `notarytool store-credentials` profile name. When set (and
#                            AUDIAL_SIGN_IDENTITY is also set), the zip is submitted to Apple
#                            notary service and waited on, then each bundle is stapled and the
#                            zip is rebuilt.
#
# AUDIAL_SKIP_BUILD=1 skips scripts/build_macos.sh and packages the build products already
# sitting in standalone/builds/osx/build/Release and plugin/builds/osx/build/Release.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SHA="$(git -C "$ROOT" rev-parse --short HEAD)"
AUDIAL_ENV="${AUDIAL_ENV:-prod}"

if [ "${AUDIAL_SKIP_BUILD:-}" = "1" ]; then
  echo "AUDIAL_SKIP_BUILD=1: skipping build_macos.sh, packaging existing build products"
else
  AUDIAL_ENV="$AUDIAL_ENV" "$ROOT/scripts/build_macos.sh" Release AudialSynth >/dev/null
fi

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

BUNDLES=("$OUT/AudialSynth.app" "$OUT/AudialSynth.vst3" "$OUT/AudialSynth.component")

if [ -n "${AUDIAL_SIGN_IDENTITY:-}" ]; then
  echo "Signing bundles with identity: $AUDIAL_SIGN_IDENTITY"
  for b in "${BUNDLES[@]}"; do
    codesign --force --deep --options runtime --timestamp --sign "$AUDIAL_SIGN_IDENTITY" "$b"
  done
else
  echo "AUDIAL_SIGN_IDENTITY not set: bundles are ad-hoc signed only (not suitable for notarization/distribution outside this Mac)."
fi

ZIP="$ROOT/dist/$NAME.zip"
(cd "$ROOT/dist" && rm -f "$NAME.zip" && zip -qr "$NAME.zip" "$NAME")

if [ -n "${AUDIAL_SIGN_IDENTITY:-}" ] && [ -n "${AUDIAL_NOTARY_PROFILE:-}" ]; then
  echo "Submitting $NAME.zip for notarization with profile: $AUDIAL_NOTARY_PROFILE"
  xcrun notarytool submit "$ZIP" --keychain-profile "$AUDIAL_NOTARY_PROFILE" --wait
  for b in "${BUNDLES[@]}"; do
    xcrun stapler staple "$b"
  done
  (cd "$ROOT/dist" && rm -f "$NAME.zip" && zip -qr "$NAME.zip" "$NAME")
fi

size_of() { stat -f%z "$1" 2>/dev/null || stat -c%s "$1"; }

echo "$ZIP  size=$(size_of "$ZIP") bytes  sha256=$(shasum -a 256 "$ZIP" | cut -d' ' -f1)"

if [ "$AUDIAL_ENV" = "prod" ]; then
  STABLE="$ROOT/dist/AudialSynth-macOS.zip"
  cp "$ZIP" "$STABLE"
  (cd "$ROOT/dist" && shasum -a 256 "AudialSynth-macOS.zip" > "AudialSynth-macOS.zip.sha256")
  echo "$STABLE  size=$(size_of "$STABLE") bytes  sha256=$(cut -d' ' -f1 "$ROOT/dist/AudialSynth-macOS.zip.sha256")"
fi
