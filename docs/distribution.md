# Distributing Audial Synth

- `scripts/package_macos.sh` builds Release and zips the app, VST3 and AU with
  the licence and a SOURCE.txt link to the exact commit (GPLv3 requires it).
- The build is unsigned in this iteration: users must right-click → Open the
  app once, and DAWs on macOS 15+ will refuse the unsigned VST3/AU until the
  bundle is signed and notarized. Signing is the next packaging task.
- Never include factory presets, wavetables or skins from the upstream
  project's free or paid content. They are under separate licences that do not
  allow redistribution.
- `backend-v1` tags the commit the hosted render backend is built from, and is
  re-pointed at each newly qualified backend commit rather than a new tag being
  cut each time. Move it and push with:
  `git tag -f backend-v1 <sha> && git push origin main --tags`. Its sha is the
  `PLUGIN_COMMIT` pin in the render backend repo (named in
  `docs/build-notes.md`); confirm it with `git rev-parse backend-v1` before
  pinning.
