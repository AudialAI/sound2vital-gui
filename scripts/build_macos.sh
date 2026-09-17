#!/usr/bin/env bash
# Build the VST3 and AU (plugin project) and the standalone app (standalone project)
# with authentication compiled out.
# Usage: scripts/build_macos.sh [Debug|Release] [target-prefix]   (defaults: Release, Vial)
set -euo pipefail
CONFIG="${1:-Release}"
PREFIX="${2:-Vial}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
# Upstream signs with Vital Audio's Apple team ID; build locally with an ad-hoc
# signature instead so no developer certificate is required.
# JUCE_VST3_CAN_REPLACE_VST2=0 because the VST2 SDK is not vendored (only
# third_party/VST_SDK/VST3_SDK is present) and no VST2 build is shipped.
# DEPLOYMENT_LOCATION=NO keeps the products in build/$CONFIG (upstream installs
# them into ~/Library/Audio/Plug-Ins via DSTROOT=/, which the current Xcode build
# system rejects with a dependency cycle on "/").
FLAGS=(ARCHS=arm64 ONLY_ACTIVE_ARCH=YES GCC_TREAT_WARNINGS_AS_ERRORS=NO
       GCC_PREPROCESSOR_DEFINITIONS='$(inherited) NO_AUTH=1 JUCE_VST3_CAN_REPLACE_VST2=0'
       CODE_SIGN_IDENTITY=- CODE_SIGN_STYLE=Manual DEVELOPMENT_TEAM=
       PROVISIONING_PROFILE_SPECIFIER=
       DEPLOYMENT_LOCATION=NO)
for TARGET in "VST3" "AU"; do
  xcodebuild -project "$ROOT/plugin/builds/osx/$PREFIX.xcodeproj" \
    -target "$PREFIX - $TARGET" -configuration "$CONFIG" "${FLAGS[@]}" build
done
xcodebuild -project "$ROOT/standalone/builds/osx/$PREFIX.xcodeproj" \
  -target "$PREFIX - App" -configuration "$CONFIG" "${FLAGS[@]}" build
ls "$ROOT/plugin/builds/osx/build/$CONFIG/" "$ROOT/standalone/builds/osx/build/$CONFIG/"
