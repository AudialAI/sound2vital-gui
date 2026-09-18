# Audial Synth

Audial Synth is a polyphonic wavetable synthesizer: spectral warping oscillators, a wavetable
editor, a modulation matrix with real-time graphical feedback, and a full effects chain. It builds
as a VST3, an Audio Unit, an LV2 and a standalone application.

It is a GPLv3 fork of the synthesizer written by Matt Tytel, whose source is at
[github.com/mtytel/vital](https://github.com/mtytel/vital). Audial Synth is not affiliated with or
endorsed by that project; it is a separate product, separately named, and it talks to no service
belonging to it. The upstream copyright notices are kept in every file that carries them, and in
`LICENSE` and `debian/copyright`.

## Resynth

The RESYNTH button in the header opens a panel where you drop a one-shot sample of up to 20
seconds. The sample is uploaded to the Audial API, which analyses it and returns a patch; the patch
is saved under your user preset folder and loaded into the synth, where every oscillator, envelope
and effect stays editable like any other preset. The panel holds your Audial base URL, user id and
API key; nothing is sent anywhere until you drop a file.

## Building

### macOS (VST3, AU, standalone)

```bash
scripts/build_macos.sh Release AudialSynth
```

Builds with Xcode into `plugin/builds/osx/build/Release/` (`AudialSynth.vst3`,
`AudialSynth.component`) and `standalone/builds/osx/build/Release/AudialSynth.app`. The products are
ad-hoc signed, so no developer certificate is needed, and macOS will ask you to approve the app and
the plug-ins the first time.

### Linux VST3

```bash
scripts/build_linux_vst3.sh
```

Builds inside an Ubuntu 22.04 Docker image and exports the bundle to
`docker/out/AudialSynth.vst3/`. On Apple Silicon this runs emulated and takes 30-60 minutes. A
native Linux build needs no Docker: `make vst3 CONFIG=Release` from the repository root, with the
build dependencies listed in `debian/control`.

### Tests

```bash
scripts/run_tests_macos.sh
```

Builds and runs the JUCE unit tests (parameter tables, preset load/save, the Audial API client's
pure helpers, and the interface sections). The script's own summary is truncated; the full log is
written to `/tmp/audial_tests.log`.

### Packaging

```bash
scripts/package_macos.sh
```

Produces `dist/AudialSynth-macOS-<sha>.zip` containing the app, the VST3, the AU, the licence and a
`SOURCE.txt` pointing at the exact commit the binaries were built from, which GPLv3 requires.

## Documentation

- `docs/build-notes.md` - build environment, every non-obvious fix, and the `backend-v1` tag that
  pins the commit used by the hosted render backend.
- `docs/distribution.md` - what ships, what must never ship, and the tagging procedure.
- `docs/parity.md` - behaviour parity notes against the upstream synthesizer.
- `docs/manual-checklist.md` - the checks that still need a human at a screen.

## Licence

Audial Synth is licensed under the GNU General Public License v3 or later; see `LICENSE`. If you
distribute a binary you must make the corresponding source available under the same licence.
Presets, wavetables and other content distributed by the upstream project are under separate
licences and are not redistributed here.
