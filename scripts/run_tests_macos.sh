#!/usr/bin/env bash
# Build and run the Audial Synth unit test console app (all of Vital's tests plus ours).
# Usage: scripts/run_tests_macos.sh [nongraphical]   (log: /tmp/audial_tests.log)
# Any argument skips the Interface category, whose stress tests open windows on screen.
# AUDIAL_ENV (env var, default "prod") selects the baked-in Audial API base URL; see
# scripts/build_macos.sh for the accepted values (prod, dev, mock).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

AUDIAL_ENV="${AUDIAL_ENV:-prod}"
case "$AUDIAL_ENV" in
  prod)
    EXTRA_DEFS=""
    ;;
  dev)
    EXTRA_DEFS=' AUDIAL_API_BASE_URL=\"https://starfish-app-2x28e.ondigitalocean.app\"'
    ;;
  mock)
    EXTRA_DEFS=' AUDIAL_API_BASE_URL=\"http://localhost:8766\"'
    ;;
  *)
    echo "Unknown AUDIAL_ENV: $AUDIAL_ENV (expected prod, dev, or mock)" >&2
    exit 1
    ;;
esac
echo "Testing for AUDIAL_ENV=$AUDIAL_ENV"

# Same extra xcodebuild settings as scripts/build_macos.sh: ad-hoc signing (no Vital Audio
# developer certificate on this machine) and DEPLOYMENT_LOCATION=NO (DSTROOT=/ deadlocks the
# current Xcode build system). See docs/build-notes.md.
xcodebuild -project "$ROOT/tests/builds/osx/AudialSynthTests.xcodeproj" -target "AudialSynthTests - ConsoleApp" \
  -configuration Release ARCHS=arm64 ONLY_ACTIVE_ARCH=YES GCC_TREAT_WARNINGS_AS_ERRORS=NO \
  GCC_PREPROCESSOR_DEFINITIONS="\$(inherited) NO_AUTH=1$EXTRA_DEFS" \
  CODE_SIGN_IDENTITY=- CODE_SIGN_STYLE=Manual DEVELOPMENT_TEAM= \
  PROVISIONING_PROFILE_SPECIFIER= \
  DEPLOYMENT_LOCATION=NO build | tail -3
"$ROOT/tests/builds/osx/build/Release/AudialSynthTests" "${1:-}" 2>&1 | tee /tmp/audial_tests.log | grep -i "audial client\|failed\|passed" | head -20
