# Distributing Audial Synth

- `scripts/package_macos.sh` builds Release and zips the app, VST3 and AU with
  the licence and a SOURCE.txt link to the exact commit (GPLv3 requires it).
- The build is unsigned in this iteration: users must right-click → Open the
  app once, and DAWs on macOS 15+ will refuse the unsigned VST3/AU until the
  bundle is signed and notarized. Signing is the next packaging task.
- Never include factory presets or wavetables from vital.audio.
- Tag the commit used for the hosted engine's render backend:
  `git tag backend-v1 && git push --tags`. The tag's sha is `PLUGIN_COMMIT`
  in sound2vital-runpod.
