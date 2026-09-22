# sound2vital-gui (Audial Synth) — design

Date: 2026-09-16
Status: approved for planning (business model confirmed by the user in conversation)

## 1. Goal

Fork the open-source Vital synthesizer (github.com/mtytel/vital, GPLv3) into a
free, renamed synth that Audial distributes from its website with full synth
functionality, plus one new section: drop a one-shot sample, the sample is sent
to the hosted `sound2vital` engine with the user's Audial API key, and the
returned preset loads into the running instance.

The same fork is the engine's render backend: its Linux VST3 build is what the
microservice loads to score candidates, so the preset the server optimises is
the sound the user hears.

## 2. Non-goals

- No change to Vital's DSP. The fork's synth engine stays upstream 1.0.6.
- No factory preset or wavetable content from vital.audio (separate licence).
- No contact with any vital.audio service.
- No Windows build, no code signing or notarization in this iteration.
- No in-DAW file-system sandbox handling beyond what JUCE already does.

## 3. Licensing and naming (hard constraints from upstream terms)

- The fork stays GPLv3; the source is published with every binary.
- Product name is **Audial Synth** (working name, user may change). "Vital",
  "Vital Audio", "Tytel" never appear in product names, bundle ids, window
  titles, or the logo. Plugin codes: manufacturer `Audi`, plugin `AuSy`, bundle
  id `ai.audialmusic.synth`.
- The Vital logo paths (`Paths::vitalV`, `Paths::vitalRing`, `vitalWord`,
  `vitalWordRing`) are replaced by the Audial wave mark.
- Authentication, update checking and factory-content download code paths are
  compiled out (`NO_AUTH=1`) or pointed at nothing.

## 4. Upstream facts the design relies on

| Fact | Consequence |
|---|---|
| Upstream is version 1.0.6 (JUCE 6.0.5), Projucer-generated Makefiles and Xcode projects, unity build files in `src/unity_build/` | New sources are added by one `#include` line in a unity file; no Projucer needed |
| `LoadSave::jsonToState` refuses presets whose feature version (major.minor) is newer than the synth's | Engine presets carry the render backend's own version, so they load; historical 1.6.4 presets are accepted by relaxing the gate to a logged warning |
| `LoadSave::loadControls` ignores unknown controls | The 131 controls a 1.6.4 preset has that 1.0.6 lacks (`modulation_N_ramp_up/down`, `osc_N_spectral_morph_phase`) are all at their defaults in engine output, so nothing is lost |
| `SynthBase::loadFromFile(File, std::string& error)` loads a preset and refreshes the GUI; `SynthGuiInterface::externalPresetLoaded(File)` updates the browser | The drop section loads presets exactly like the preset browser does |
| `Overlay` sections (`UpdateCheckSection`, `DownloadSection`) show how a modal panel with OpenGL text, buttons, background threads and `juce::URL` networking is built | The new section follows that pattern |
| `LoadSave::getConfigJson()/saveJsonToConfig()` persist per-user settings outside DAW sessions | API credentials live there |
| `AudioFileDropSource` handles single-file audio drops with `AudioFormatManager` wildcards | Reused for the drop zone |
| JUCE 6.0.5 `URL::createInputStream(bool postLike, cb, ctx, extraHeaders, timeoutMs, responseHeaders, int* statusCode, redirects, String httpRequestCmd)` and `URL::withFileToUpload` | Multipart PUT and JSON POST need no new dependency |
| Linux VST3 target builds through `plugin/builds/linux_vst/Makefile` `VST3`; tests through `tests/builds/linux/Makefile` and `tests/builds/osx/VitalTests.xcodeproj` (target `VitalTests - ConsoleApp`), both already `NO_AUTH=1` | Headless build for the microservice and a unit-test harness exist |

## 5. Repository

`Audial-All Current Code/sound2vital-gui/` is a git repository whose history
starts at upstream `mtytel/vital` `main`, with upstream added as a remote so
later upstream changes can be merged. Planning documents live under `docs/`.

Layout additions:

```
docs/superpowers/{specs,plans}/
docker/Dockerfile.linux-vst3     headless Linux VST3 build + pedalboard load check
docker/verify_vst3.py
tools/parity_check.py            render presets through Vital 1.6.4 and the fork, compare
tools/mock_audial_api.py         local stand-in for the Audial API (files, run, execution)
src/common/audial_client.{h,cpp}
src/interface/editor_sections/resynth_section.{h,cpp}
tests/common/audial_client_test.cpp
```

## 6. Build targets

- **macOS (development and the product download):** `plugin/builds/osx/Vial.xcodeproj`
  targets `Vial - VST3` and `Vial - AU`, plus `standalone/builds/osx/Vial.xcodeproj`
  target `Vial - App` (Vital's real standalone application, which also owns the
  `--render` CLI); all renamed to `AudialSynth - …` by the rename task and built
  with `GCC_PREPROCESSOR_DEFINITIONS='$(inherited) NO_AUTH=1'`, `ARCHS=arm64`.
- **Linux VST3 (engine backend):** `make vst3` at the repo root, with
  `REQUIRE_AUTH=1` replaced by `NO_AUTH=1` and the firebase/libsecret link
  flags removed in `plugin/builds/linux_vst/Makefile` and
  `standalone/builds/linux/Makefile`. Output
  `plugin/builds/linux_vst/build/AudialSynth.vst3/Contents/x86_64-linux/AudialSynth.so`.
- **Tests:** `tests/builds/osx/VitalTests.xcodeproj` on macOS, `make test` on Linux.

## 7. The Resynth section

A full-window `Overlay` opened from a `RESYNTH` button in the header, next to
the tab selector.

Contents, top to bottom:

1. Title "Resynth a sample" and one line of help: "Drop a one-shot (20 s max).
   Audial builds an editable patch from it."
2. Drop zone (also click to browse). Accepts one file matching the JUCE
   audio format wildcards. Files longer than 20.0 s are refused with the
   message "Sample is 24.3 s; the limit is 20 s" before any upload.
3. Status line and a progress hint: Uploading, Submitted, Processing (with
   elapsed seconds), Downloading, Done, or the error text.
4. Credentials: three text fields (API base URL, user id, API key shown as
   password characters) and a Save button. Stored through `LoadSave` config
   keys `audial_base_url`, `audial_user_id`, `audial_api_key`. Default base URL
   `https://api.audialmusic.ai`.
5. Buttons: Cancel (visible during a job), Close.

Job state machine, run on a `juce::Thread` owned by the section:

```
Idle -> Uploading -> Submitting -> Processing -> Downloading -> Done
                                  \-> Error (any step)          \-> Idle (Close)
```

- Uploading: `PUT {base}/api/files/{user}/execution/{uuid}/reference/{name}`
  (multipart, headers `x-api-key`, `x-user-id`). The filename is sanitised to
  `[A-Za-z0-9_.-]`, because the API uses it as a Firebase key.
- Submitting: `POST {base}/api/functions/run/sound2vital` with body
  `{"userId": ..., "original": {"filename": ..., "url": ...}}`. Response is the
  execution; `exeId` is kept.
- Processing: `GET {base}/api/db/{user}/execution/{exeId}` every 2 s until
  `state` is `completed` or `failed`; client cap 300 s. `preset` is a map of
  file entries; the first entry's `url` is the preset.
- Downloading: the preset is saved to
  `LoadSave::getUserPresetDirectory()/Resynth/<sanitised sample name>_<yyyymmdd-hhmmss>.vital`
  so it appears in the preset browser afterwards.
- Done: on the message thread, `SynthBase::loadFromFile` then
  `SynthGuiInterface::externalPresetLoaded`; the overlay closes. A load failure
  shows the loader's error string.

Cancel stops the thread; a job already submitted keeps running server-side and
its preset stays available in the user's Audial executions.

## 8. Networking client

`AudialClient` (in `src/common/`) wraps JUCE `URL`:

- `HttpResult uploadReference(const File&, const String& exeId, const String& filename)`
- `HttpResult runSound2Vital(const String& filename, const String& fileUrl)`
- `HttpResult getExecution(const String& exeId)`
- `bool downloadToFile(const String& url, const File& dest)`
- Pure helpers, unit-tested with JUCE `UnitTest`: `sanitizeFilename`,
  `buildRunBody`, `parseUrl`, `parseExecution` (returns `state`, `preset_url`,
  `error`).

All network calls run off the message thread. Status updates are posted with
`MessageManager::callAsync` guarded by `Component::SafePointer`.

## 9. Parity verification against Vital 1.6.4

Before the microservice pins the fork, `tools/parity_check.py` renders every
preset in a directory (default: the 207 native results under the genetic repo's
`editable_corpus_parallel_v8/run/native/*/work/preset.vital`) through both the
installed Vital 1.6.4 VST3 and the fork's macOS VST3, using the engine's own
`apply_preset_state`/`render_midi` helpers, and writes `parity.json` with per
preset residual RMS in dB relative to the 1.6.4 render plus the engine's
`compare_audio` metrics. The ten worst are listened to and their spectrograms
reviewed; the decision (proceed, or fix a DSP gap first) is recorded in
`docs/parity.md`. Bit-exact parity is not expected: the goal is that the
server-side objective and the user's synth agree, which the microservice's
requalification measures directly.

## 10. Testing

- JUCE unit tests for `AudialClient` helpers, included through
  `tests/interface_tests.cpp` and run with the existing test targets.
- `tools/mock_audial_api.py`: a stdlib HTTP server that accepts the upload,
  returns an execution, flips it to `completed` after a delay and serves a
  fixed preset file. Used to exercise the whole GUI flow without the real
  service.
- Manual checklist per build: app launches without a login prompt, header
  shows no Vital branding, drop zone refuses a 21 s file, mock flow ends with
  the preset loaded and visible in the browser, credentials survive restart.

## 11. Decisions and assumptions recorded here

- Product name "Audial Synth" is a working name chosen so the plan has
  concrete identifiers; every occurrence is in the rename task.
- The header button and section are called "Resynth"; also a working name.
- Upstream remote URL `https://github.com/mtytel/vital.git`; the fork's GitHub
  URL is `https://github.com/AudialAI/sound2vital-gui.git` (confirmed 2026-09-22)
  (the microservice Dockerfile's default build argument).
- The microservice loads the Linux VST3, not the standalone `--render` CLI:
  the project's own notes record that CLI renders ignore wavetable and sample
  data, which the engine relies on.
