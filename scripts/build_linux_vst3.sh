#!/usr/bin/env bash
# Build and verify the Linux VST3 in Docker; export the bundle to docker/out/.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
docker build --platform linux/amd64 -f "$ROOT/docker/Dockerfile.linux-vst3" --target verify -t audialsynth-vst3-verify "$ROOT"
docker build --platform linux/amd64 -f "$ROOT/docker/Dockerfile.linux-vst3" --target artifact -o "$ROOT/docker/out" "$ROOT"
ls "$ROOT/docker/out/AudialSynth.vst3/Contents/x86_64-linux/"
