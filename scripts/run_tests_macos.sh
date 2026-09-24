#!/usr/bin/env bash
# Build and run the Audial Synth unit test console app (all of Vital's tests plus ours).
# Usage: scripts/run_tests_macos.sh [nongraphical]   (log: /tmp/audial_tests.log)
# Any argument skips the Interface category, whose stress tests open windows on screen.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
# Same extra xcodebuild settings as scripts/build_macos.sh: ad-hoc signing (no Vital Audio
# developer certificate on this machine) and DEPLOYMENT_LOCATION=NO (DSTROOT=/ deadlocks the
# current Xcode build system). See docs/build-notes.md.
xcodebuild -project "$ROOT/tests/builds/osx/AudialSynthTests.xcodeproj" -target "AudialSynthTests - ConsoleApp" \
  -configuration Release ARCHS=arm64 ONLY_ACTIVE_ARCH=YES GCC_TREAT_WARNINGS_AS_ERRORS=NO \
  GCC_PREPROCESSOR_DEFINITIONS='$(inherited) NO_AUTH=1' \
  CODE_SIGN_IDENTITY=- CODE_SIGN_STYLE=Manual DEVELOPMENT_TEAM= \
  PROVISIONING_PROFILE_SPECIFIER= \
  DEPLOYMENT_LOCATION=NO build | tail -3
"$ROOT/tests/builds/osx/build/Release/AudialSynthTests" "${1:-}" 2>&1 | tee /tmp/audial_tests.log | grep -i "audial client\|failed\|passed" | head -20
